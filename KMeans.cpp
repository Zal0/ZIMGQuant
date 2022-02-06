#include "KMeans.h"
#include "Octree.h"

ColorRGB* KMeans(const Image& img, int k)
{
	//Initialize palette using octree method
	ColorRGB* ret = OctreePalette(img, k);

	Group* groups = new Group[k];
	KDTree* kd_tree_nodes = new KDTree[k];

	while(true)
	{
		for(int c = 0; c < k; ++c) 
		{
			groups[c].Clear();
			kd_tree_nodes[c].Reset(&ret[c], &groups[c]);
		}

		KDTree* kd_tree = KDTree::Build(kd_tree_nodes, kd_tree_nodes + k, 0);

		//Group pixels by their closest centroid
		for(int y = 0; y < img.h; ++ y)
		{
			for(int x = 0; x < img.w; ++ x)
			{
				int idx = (img.w * y + x) * img.depth;
				ColorRGB color(img.data[idx], img.data[idx + 1], img.data[idx + 2]);

				//Locate the closest centroid
				//int best_k = FindClosest(color, ret, k);
				//groups[best_k].Add(color);

				KDTree* nearest = kd_tree->Nearest(color);
				//if(color.Dist(*nearest->color) != color.Dist(ret[best_k]))
				//	printf("ERROR!!");
				nearest->group->Add(color);
			}
		}

		int dist = 0;
		//Recalculate centroids
		for(int c = 0; c < k; ++c) 
		{
			Group& group = groups[c];

			if(group.n == 0)
				continue; //No colors near this one, skip

			group.color[0] /= group.n;
			group.color[1] /= group.n;
			group.color[2] /= group.n;

			ColorRGB new_color((unsigned char)group.color[0], (unsigned char)group.color[1], (unsigned char)group.color[2]);
			int d = ret[c].Dist(new_color);
			if(d > dist)
				dist = d;

			ret[c] = new_color;
		}

		//printf("%d\n", dist);
		if(dist == 0)
			break;
	}

	delete[] kd_tree_nodes;
	delete[] groups;

	return ret;
}