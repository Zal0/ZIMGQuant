#include "Octree.h"

Octree::Octree() : num_leaves(0)
{
	root = new OctreeNode(this, 0);
}

Octree::~Octree()
{
	delete root;
}

ColorRGB* Octree::GetPalette(size_t num_colors)
{
	//Locate the level where we should start reducing nodes
	int current_level = 0;
	while(current_level < 8 && nodes_by_level[current_level + 1].size() < num_colors)
	{
		current_level ++;
	}

	//Sort this level
	std::sort(nodes_by_level[current_level].begin(), nodes_by_level[current_level].end(), OctreeNode::comp);

	ColorRGB* ret = new ColorRGB[num_colors];
	int ret_size = 0;

	std::vector< OctreeNode* >& nodes = nodes_by_level[current_level];
	std::vector< OctreeNode* > children(8);
	for(size_t i = 0; i < nodes.size(); ++i)
	{
		OctreeNode* node = nodes[i];
		size_t max_children = num_colors - (nodes.size() - i) - ret_size + 1;
		if(max_children > 1)
		{
			//Pick the maximum amount of children on this node
			children.clear();
			for(int j = 0; j < 8; ++j)
			{
				if(node->nodes[j])
				{
					children.push_back(node->nodes[j]);
				}
			}

			if(children.size() > max_children)
			{
				//Add the nodes with less colors into the last position
				std::sort(children.begin(), children.end(), OctreeNode::comp);

				Group& group = (*(children.begin() + max_children - 1))->group;
				for(std::vector< OctreeNode* >::iterator it = children.begin() + max_children; it != children.end(); ++ it)
				{
					group.color[0] += (*it)->group.color[0];
					group.color[1] += (*it)->group.color[1];
					group.color[2] += (*it)->group.color[2];
					group.n += (*it)->group.n;
				}

				//Remove the colors added together
				children.erase(children.begin() + max_children, children.end());
			}

			for(std::vector< OctreeNode* >::iterator it = children.begin(); it != children.end(); ++ it)
			{
				Group& group = (*it)->group;
				ret[ret_size ++] = ColorRGB((unsigned char)(group.color[0] / group.n), (unsigned char)(group.color[1] / group.n), (unsigned char)(group.color[2] / group.n));
			}
		}
		else
		{
			//Reduce this node (ignore children)
			Group& group = nodes[i]->group;
			ret[ret_size ++] = ColorRGB((unsigned char)(group.color[0] / group.n), (unsigned char)(group.color[1] / group.n), (unsigned char)(group.color[2] / group.n));
		}
	}

	return ret;
}

ColorRGB* OctreePalette(const Image& image, int num_colors)
{
	Octree octree;
	for(int y = 0; y < image.h; ++ y)
	{
		for(int x = 0; x < image.h; ++ x)
		{
			octree.root->Add(image.Get(x, y), &octree);
		}
	}

	return octree.GetPalette(num_colors);
}