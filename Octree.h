#ifndef OCTREE_H
#define OCTREE_H

#include "Image.h"
#include <vector>
#include <algorithm>

#define BIT(V, N) (((V) >> (N)) & 0x1)
class OctreeNode;
class Octree
{
public:
	OctreeNode* root;
	std::vector< OctreeNode* > nodes_by_level[8];
	int num_leaves;
	
	Octree();
	~Octree();
	ColorRGB* GetPalette(size_t num_colors);
};

class OctreeNode
{
public:
	Group group;
	OctreeNode* nodes[8];

	OctreeNode(Octree* tree, int level)
	{
		for(int i = 0; i < 8; ++i)
			nodes[i] = 0;

		if(level >= 0 && level < 8)
			tree->nodes_by_level[level].push_back(this);
	}

	~OctreeNode()
	{
		for(int i = 0; i < 8; ++i)
			if(nodes[i]) delete nodes[i];
	}

	void Add(const ColorRGB& color, Octree* tree, int level = 0)
	{
		this->group.Add(color);

		if(level < 8)
		{
			int idx = (BIT(color.R, 7 - level) << 2) | (BIT(color.G, 7 - level) << 1) | BIT(color.B, 7 - level);
			if(nodes[idx] == 0)
			{
				nodes[idx] = new OctreeNode(tree, level + 1);
				if(level == 7)
					tree->num_leaves ++;
			}
			nodes[idx]->Add(color, tree, level + 1);
		}
	}

	static bool comp(OctreeNode* n0, OctreeNode* n1)
	{
		return n0->group.n > n1->group.n;
	}
};

ColorRGB* OctreePalette(const Image& image, int num_colors);

#endif