#include "graph.h"

int direction[8][2] = 
{
	{-1,-1},
	{-1,0},
	{-1,1},
	{0,-1},
	{0,1},
	{1,-1},
	{1,0},
	{1,1}
};

bool isValid(int x,int y, int w, int h)
{
	return (x>=0 && x<w && y>=0 && y<h);
}

Graph::Graph(Image& imageI)
{
	this->image = &imageI;
	int h = image->getHeight();
	int w = image->getWidth();

	//Semantic
	// edges[i][j][k] -> denotes whether there is a an edge from (i,j) in kth direction in the graph

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

	//Add edge if similar color
	for(int i = 1 ; i < w ; i++) for(int j = 0; j < h; j++) for(int k = 0 ; k < 8; k ++) {
		if((*image)(i+direction[k][0],j+direction[k][1]) != nullptr)
			edges[i][j][k] = (*image)(i,j)->isSimilar(*(*image)(i+direction[k][0],j+direction[k][1]));
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
	if(x < 0 || x >= this->image->getWidth()) return -1;
	if(y < 0 || y >= this->image->getHeight()) return -1;

	int cnt = 0;
	for(int i = 0; i < 8 ; i++) if(edges[x][y][i] == true) cnt ++ ;
	return cnt;
}

void Graph::curves_heuristic(int x, int y, int DIRECTION)
{
	
}

void Graph::islands_heuristic(int x,int y)
{
	for(int i = 0; i < 8; i++) if(edges[x][y][i] == true) weights[x][y][i] = 5 * ((valence(x,y) == 1) + (valence(x+direction[BOTTOM_RIGHT][0],y+direction[BOTTOM_RIGHT][1]) == 1));
}

void Graph::sparse_pixels_heuristic(int x, int y, int DIRECTION)
{
	
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
		
		if(this->edges[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] == true && this->edges[topRight->getX()][topRight->getY()][BOTTOM_LEFT] == true)
		{
			//Edges are crossing and the pixels are dissimilar, need to discard atleast one.
			//Check if there are horizontal/vertical connections
			if(this->edges[topLeft->getX()][topLeft->getY()][BOTTOM]) continue;
			if(this->edges[topLeft->getX()][topLeft->getY()][RIGHT]) continue;
			if(this->edges[bottomRight->getX()][bottomRight->getY()][TOP]) continue;
			if(this->edges[bottomRight->getX()][bottomRight->getY()][LEFT]) continue;

			this->islands_heuristic(topLeft->getX(),topLeft->getY());
			this->islands_heuristic(topRight->getX(),topRight->getY());
			this->curves_heuristic(topLeft->getX(),topLeft->getY(),BOTTOM_RIGHT);
			this->curves_heuristic(topRight->getX(),topRight->getY(),BOTTOM_LEFT);
			this->sparse_pixels_heuristic(topLeft->getX(),topLeft->getY(),BOTTOM_RIGHT);
			this->sparse_pixels_heuristic(topRight->getX(),topRight->getY(),BOTTOM_LEFT);

			if(this->weights[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] >= this->weights[topRight->getX()][topRight->getY()][BOTTOM_LEFT])
			{
				this->weights[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] = false;
				this->weights[bottomRight->getX()][bottomRight->getY()][TOP_LEFT] = false;	
			}
			if(this->weights[topLeft->getX()][topLeft->getY()][BOTTOM_RIGHT] <= this->weights[topRight->getX()][topRight->getY()][BOTTOM_LEFT])
			{
				this->weights[topRight->getX()][topRight->getY()][BOTTOM_LEFT] = false;
				this->weights[bottomLeft->getX()][bottomLeft->getY()][TOP_RIGHT] = false;	
			}
		}	
	}
}