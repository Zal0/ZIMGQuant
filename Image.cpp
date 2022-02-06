#include "Image.h"
#include "KMeans.h"

int Clamp(int v, int min, int max)
{
	int ret = v;
	if(v < min) v = min;
	if(v > max) v = max;
	return v;
}

int FindClosest(const ColorRGB color, ColorRGB* pal, int pal_size)
{
	//Locate the closest centroid
	int min_dist = INT_MAX;
	int best_k = 0;
	for(int c = 0; c < pal_size ; ++ c)
	{
		int d = color.Dist(pal[c]);
		if(d < min_dist)
		{
			best_k = c;
			min_dist = d;
		}
	}
	return best_k;
}

void Image::SetPalette(ColorRGB* palette, int k, bool dithering)
{
	/*KDTree* kd_tree_nodes = new KDTree[k];
	for(int c = 0; c < k; ++c) 
	{
		kd_tree_nodes[c].Reset(&palette[c], 0);
	}
	KDTree* kd_tree = KDTree::Build(kd_tree_nodes, kd_tree_nodes + k, 0);*/

	for(int y = 0; y < h; ++y)
	{
		for(int x = 0; x < w; ++x)
		{
			ColorRGB& nearest_color = palette[FindClosest(Get(x, y), palette, k)];
			//ColorRGB& nearest_color = *kd_tree->Nearest(Get(x, y))->color;

			//Apply dithering
			if(dithering)
			{
				Vec3 quant_error = Get(x, y) - nearest_color;
				Add(x + 1, y    , quant_error * (7.0f / 16.0f));
				Add(x - 1, y + 1, quant_error * (3.0f / 16.0f));
				Add(x    , y + 1, quant_error * (5.0f / 16.0f));
				Add(x + 1, y + 1, quant_error * (1.0f / 16.0f));
			}

			Set(x, y, nearest_color);
		}
	}

	//delete[] kd_tree_nodes;
}