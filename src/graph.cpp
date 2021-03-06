#include "graph.h"

//Checks if the requested pixel is in range of the image
bool isValid(int x,int y, int w, int h)
{
	return (x>=0 && x<w && y>=0 && y<h);
}


Graph::Graph(Image& imageI)
{
	//Innitializing variables from Image
	this->image = &imageI;
	int h = image->getHeight();
	int w = image->getWidth();

	//Semantic
	// edges[i][j][k] -> denotes whether there is a an edge from (i,j) in kth direction in the graph
	//Preallocating structures
	for(int i = 0; i < w; i++) 
	{
		vector<vector<bool>> v1;
		vector<vector<int>> v2;
		for(int j = 0; j < h; j ++)
		{
			v1.push_back(vector<bool>(8,false));
			v2.push_back(vector<int>(8,0));
		}
		edges.push_back(v1);
		weights.push_back(v2);
	}

	//Add edge in kth direction of (x,y) of (x,y)+k is valid cell and has similar color
	for(int i = 0 ; i < w ; i++) for(int j = 0; j < h; j++) for(int k = 0 ; k < 8; k ++) {
		if((*image)(i+direction[k][0],j+direction[k][1]) != nullptr) edges[i][j][k] = (*image)(i,j)->isSimilar(*(*image)(i+direction[k][0],j+direction[k][1]));
	}
}

void Graph::removeCross()
{
	//Take current pixel as top-left of a 4 pixel square and check whether the colors are same in all
	for(int i = 0 ; i < this->image->getWidth() - 1; i++) for(int j = 0 ; j < this->image->getHeight() - 1; j++)
	{
		_pixel* topLeft = (*this->image)(i,j);
		_pixel* topRight = (*this->image)(i+direction[RIGHT][0],j+direction[RIGHT][1]);
		_pixel* bottomLeft = (*this->image)(i+direction[BOTTOM][0],j+direction[BOTTOM][1]);
		_pixel* bottomRight = (*this->image)(i+direction[BOTTOM_RIGHT][0],j+direction[BOTTOM_RIGHT][1]);
		if(topLeft->isSimilar(*topRight) && topLeft->isSimilar(*bottomLeft) && topLeft->isSimilar(*bottomRight) && bottomLeft->isSimilar(*topRight))
		{
			//All colors are same in the square, remove diagonal edges
			this->edges[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] = false;
			this->edges[bottomRight->getX()][bottomRight->getY()][TOP_LEFT] = false;
			this->edges[topRight->getX()][topRight->getY()][BOTTOM_LEFT] = false;
			this->edges[bottomLeft->getX()][bottomLeft->getY()][TOP_RIGHT] = false;
		}
	}
	 
}


int Graph::valence(int x,int y)
{
	//Checks for out of range input
	if(x < 0 || x >= this->image->getWidth()) return -1;
	if(y < 0 || y >= this->image->getHeight()) return -1;

	//Count the edges around the pixel
	int cnt = 0;
	for(int i = 0; i < 8 ; i++) if(edges[x][y][i] == true) cnt ++;
	return cnt;
}

//Checks if (x,y) are inclusively inside the given cell
bool insideBounds(int x, int y, int row_st, int row_end, int col_st, int col_end)
{
	return (x>=row_st && x <= row_end && y >= col_st && y <= col_end);
}

//Curves Heuristic
void Graph::curves_heuristic(int x, int y)
{
	//Directions are (x,y) BOTTOM_RIGHT, and Right(x,y) BOTTOM_LEFT
	//A : feature for (x,y) BOTTOM_RIGHT and B : (x,y) + Right BOTTOM_LEFT
	int featureA = 0;
	int featureB = 0;

	//Find curve lengths in both directions for A
	int p = x, q = y;
	int dir = BOTTOM_RIGHT;
	while(true)
	{
		featureA++;
		if(valence(p,q) != 2) break;
		int i;
		
		for(i = dir+1; edges[p][q][i] == false; i = (i+1)%8);
		if((i+dir)==7) break;
		dir = i;
		p = p + direction[dir][0];
		q = q + direction[dir][1];
	}
	p = x+direction[BOTTOM_RIGHT][0];
	q = y+direction[BOTTOM_RIGHT][1];
	dir = TOP_LEFT;	
	while(true)
	{
		featureA++;
		if(valence(p,q) != 2) break;

		int i;
		for(i = dir+1; edges[p][q][i] == false; i = (i+1)%8);
		if((i+dir)==7) break;
		dir = i; 
		p = p + direction[dir][0];
		q = q + direction[dir][1];
	}

	//Find curve lengths in both directions for B
	p = x+direction[RIGHT][0];
	q = y+direction[RIGHT][1];
	dir = BOTTOM_LEFT;	
	while(true)
	{
		featureB++;
		if(valence(p,q) != 2) break;

		int i;
		for(i = dir+1; edges[p][q][i] == false; i = (i+1)%8);
		if((i+dir)==7) break;
		dir = i;
		p = p + direction[dir][0];
		q = q + direction[dir][1];
	}

	p = x+direction[RIGHT][0] + direction[BOTTOM_LEFT][0];
	q = y+direction[RIGHT][1] + direction[BOTTOM_LEFT][1];
	dir = TOP_RIGHT;
	
	while(true)
	{
		featureB++;
		if(valence(p,q) != 2) break;

		int i;
		for(i = dir+1; edges[p][q][i] == false; i = (i+1)%8);
		if((i+dir)==7) break;
		dir = i;
		p = p + direction[dir][0];
		q = q + direction[dir][1];
	}
	
	//Bigger curve is better, add difference to corresponding edge weight
	if(featureA < featureB) {
		weights[x+direction[RIGHT][0]][y+direction[RIGHT][1]][BOTTOM_LEFT] += (featureB - featureA);
		weights[x+direction[BOTTOM][0]][y+direction[BOTTOM][1]][TOP_RIGHT] += (featureB - featureA);
	}
	else 
	{
		weights[x][y][BOTTOM_RIGHT] += (featureA - featureB);
		weights[x+direction[BOTTOM_RIGHT][0]][y+direction[BOTTOM_RIGHT][1]][TOP_LEFT] += (featureA - featureB);
	}
}

//Checks for Isolated pixels
void Graph::islands_heuristic(int x,int y)
{
	for(int i = 0; i < 8; i++) if(edges[x][y][i] == true) 
	{
		weights[x][y][i] = 5 * ((valence(x,y) == 1) + (valence(x+direction[BOTTOM_RIGHT][0],y+direction[BOTTOM_RIGHT][1]) == 1));
		weights[x+direction[i][0]][y+direction[i][1]][7-i] = 5 * ((valence(x,y) == 1) + (valence(x+direction[BOTTOM_RIGHT][0],y+direction[BOTTOM_RIGHT][1]) == 1));
	}
}

void Graph::sparse_pixels_heuristic(int x, int y)
{
	//Directions are BOTTOM_RIGHT, and BOTTOM_LEFT
	//Measure the size of the connected component in a 8x8 box
	if(!insideBounds(x+direction[RIGHT][0],y+direction[RIGHT][1],0,image->getWidth()-1,0, image->getHeight()-1)) return;
	int labels[8][8] = {0}; 
	stack<pair<int,int>> st;
	//Do DFS from (x,y) labeling 1 to each connected node.
	st.push(make_pair(x,y));
	labels[3][3] = 1; // Position of (X,Y) in the label array
	while(!st.empty())
	{
		pair<int,int> point = st.top();
		int p = point.first;
		int q = point.second;
		st.pop();
		for(int i = 0 ; i < 8; i++)
		{
			//See in all directions, scan for points that are in the not yet visited, in the 8x8 box, and have an edge from the current point.
			if(!insideBounds(p+direction[i][0],q+direction[i][1],0,image->getWidth()-1,0, image->getHeight()-1) || !insideBounds(p+direction[i][0],q+direction[i][1],x-3,x+4,y-3,y+4)) continue;
			if(labels[3+p+direction[i][0]-x][3+q+direction[i][1]-y] != 0) continue;
			if(edges[p][q][i] == false) continue;
			st.push(make_pair(p+direction[i][0],q+direction[i][1]));
			labels[3+p+direction[i][0]-x][3+q+direction[i][1]-y] = 1;
		}
	}

	st.push(make_pair(x+direction[RIGHT][0],y+direction[RIGHT][1]));
	labels[4][3] = 2;
	
	while(!st.empty())
	{
		pair<int,int> point = st.top();
		int p = point.first;
		int q = point.second;
		st.pop();
		for(int i = 0 ; i < 8; i++)
		{
			//See in all directions, scan for points that are in the not yet visited, in the 8x8 box, and have an edge from the current point.
			if(!insideBounds(p+direction[i][0],q+direction[i][1],0,image->getWidth()-1,0, image->getHeight()-1) || !insideBounds(p+direction[i][0],q+direction[i][1],x-3,x+4,y-3,y+4)) continue;
			if(labels[3+p+direction[i][0]-x][3+q+direction[i][1]-y] != 0) continue;
			if(edges[p][q][i] == false) continue;
			st.push(make_pair(p+direction[i][0],q+direction[i][1]));
			labels[3+p+direction[i][0]-x][3+q+direction[i][1]-y] = 2;
		}
	}

	//Find the size of the connected components
	int componentA = 0, componentB = 0;
	for(int i = 0 ; i < 8 ; i++) for(int j = 0 ; j < 8; j++)
	{
		if(labels[i][j] == 1) componentA++;
		else if(labels[i][j] == 2) componentB++;
	}

	//Smaller component is better
	if(componentA > componentB) 
	{
		weights[x+direction[RIGHT][0]][y+direction[RIGHT][1]][BOTTOM_LEFT] += (componentA-componentB);
		weights[x+direction[BOTTOM][0]][y+direction[BOTTOM][1]][TOP_RIGHT] += (componentA-componentB);
	}
	else 
	{
		weights[x][y][BOTTOM_RIGHT] += (componentB-componentA);
		weights[x+direction[BOTTOM_RIGHT][0]][y+direction[BOTTOM_RIGHT][1]][TOP_LEFT] += (componentB-componentA);
	}
}

void Graph::planarize()
{
	//Remove Crosses for obvious planarization
	this->removeCross();
	//For Internal Pixels, process via heuristic if edges are crossing
	//A Pixel is the topLeft of a 2x2 box
	for(int i = 0 ; i < this->image->getWidth() - 1; i++) for(int j = 0 ; j < this->image->getHeight() - 1; j++)
	{
		_pixel* topLeft = (*this->image)(i,j);
		_pixel* topRight = (*this->image)(i+direction[RIGHT][0],j+direction[RIGHT][1]);
		_pixel* bottomLeft = (*this->image)(i+direction[BOTTOM][0],j+direction[BOTTOM][1]);
		_pixel* bottomRight = (*this->image)(i+direction[BOTTOM_RIGHT][0],j+direction[BOTTOM_RIGHT][1]);
		assert(topLeft != nullptr && topRight != nullptr && bottomLeft != nullptr && bottomRight != nullptr);
		if(this->edges[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] == true && this->edges[topRight->getX()][topRight->getY()][BOTTOM_LEFT] == true)
		{
			//Edges are crossing and the pixels are dissimilar, need to discard atleast one.
			//Check if there are horizontal/vertical connections
			if(this->edges[topLeft->getX()][topLeft->getY()][BOTTOM]) continue;
			if(this->edges[topLeft->getX()][topLeft->getY()][RIGHT]) continue;
			if(this->edges[bottomRight->getX()][bottomRight->getY()][TOP]) continue;
			if(this->edges[bottomRight->getX()][bottomRight->getY()][LEFT]) continue;

			//Run heuristics for weight
			this->islands_heuristic(topLeft->getX(),topLeft->getY());
			this->islands_heuristic(topRight->getX(),topRight->getY());
			this->curves_heuristic(topLeft->getX(),topLeft->getY());
			this->sparse_pixels_heuristic(topLeft->getX(),topLeft->getY());

			//Remove the lighter edge. And both if they are equal
			if(this->weights[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] <= this->weights[topRight->getX()][topRight->getY()][BOTTOM_LEFT])
			{
				this->edges[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] = false;
				this->edges[bottomRight->getX()][bottomRight->getY()][TOP_LEFT] = false;	
			}
			if(this->weights[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] >= this->weights[topRight->getX()][topRight->getY()][BOTTOM_LEFT])
			{
				this->edges[topRight->getX()][topRight->getY()][BOTTOM_LEFT] = false;
				this->edges[bottomLeft->getX()][bottomLeft->getY()][TOP_RIGHT] = false;	
			}
		}	
	}
}