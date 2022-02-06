#ifndef KMEANS_H
#define KMEANS_H

#include "Image.h"
#include <algorithm>

class KDTree
{
public:
	ColorRGB* color;
	Group* group;
	
	KDTree* left;
	KDTree* right;
	int d;

	KDTree() {}
	
	void Reset(ColorRGB* color, Group* group) 
	{
		this->color = color;
		this->group = group;
		this->left = 0;
		this->right = 0;
		this->d = 0; 
	}

	KDTree* Nearest(const ColorRGB& color)
	{
		KDTree* nearest = 0;
		int min_dist = INT_MAX;
		NearestR(color, nearest, min_dist);
		return nearest;
	}

	class node_cmp 
	{
	public:
		size_t d;
		node_cmp(size_t d) : d(d) {}

		bool operator()(const KDTree& n1, const KDTree& n2) const {
			return (*n1.color)[d] < (*n2.color)[d];
		}
        
    };

	static KDTree* Build(KDTree* kd_tree, KDTree* end, int d)
	{
		if(end <= kd_tree)
			return 0;

		std::nth_element(kd_tree, kd_tree + (end - kd_tree) / 2, end, node_cmp(d));

		KDTree* ret =  kd_tree + (end - kd_tree) / 2;
		ret->d = d;
		ret->left  = Build(kd_tree, ret, (d + 1) % 3);
		ret->right = Build(ret + 1, end, (d + 1) % 3);

		return ret;
	}

private:
	void NearestR(const ColorRGB& color, KDTree*& nearest, int& min_dist)
	{
		int dist = this->color->Dist(color);
		if(dist < min_dist)
		{
			nearest = this;
			min_dist = dist;
		}

		if(min_dist == 0)
			return; 

		KDTree* ordered_nodes[2];
		int bb_dist = color[d] - (*this->color)[d];
		if(bb_dist < 0)
		{
			ordered_nodes[0] = left;
			ordered_nodes[1] = right;
		}
		else
		{
			ordered_nodes[0] = right;
			ordered_nodes[1] = left;
		}

		if(ordered_nodes[0]) //First node is always mandatory (if this is not a leaf)
			ordered_nodes[0]->NearestR(color, nearest, min_dist);
		
		if(ordered_nodes[1])
		{
			if((bb_dist * bb_dist) < min_dist) //Second node only required is the distance to its bounding box is less than min_dist
				ordered_nodes[1]->NearestR(color, nearest, min_dist);
		}
	}
};

ColorRGB* KMeans(const Image& img, int k);

#endif