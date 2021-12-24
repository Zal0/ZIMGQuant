#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#include <math.h>
#include <float.h>
#include <set>
#include <algorithm>
#include <vector>

#include <Windows.h>
long long milliseconds_now() {
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    } else {
        return GetTickCount();
    }
}

int Clamp(int v, int min, int max)
{
	int ret = v;
	if(v < min) v = min;
	if(v > max) v = max;
	return v;
}

class Vec3
{
public:
	int X, Y, Z;

	Vec3(int X, int Y, int Z) : X(X), Y(Y), Z(Z) {}

	Vec3 operator*(float v) const
	{
		Vec3 ret(*this);
		ret.X = (int)(X * v);
		ret.Y = (int)(Y * v);
		ret.Z = (int)(Z * v);
		return ret;
	}
};

class ColorRGB
{
public:
	unsigned char R, G, B;

	ColorRGB() {}
	ColorRGB(unsigned char R, unsigned char G, unsigned char B) : R(R), G(G), B(B) {};

	void Set(unsigned char R, unsigned char G, unsigned char B) 
	{
		this->R = R;
		this->G = G;
		this->B = B;
	};

	int Dist(const ColorRGB& c) const
	{
		return (R - c.R) * (R - c.R) + (G - c.G) * (G - c.G) + (B - c.B) * (B - c.B);
	}

	Vec3 operator-(const ColorRGB& other)
	{
		return Vec3(R - other.R, G - other.G, B - other.B);
	}

	bool operator<(const ColorRGB& other) const
	{
		return (R + G + B) < (other.R + other.G + other.B);
	}

	unsigned char operator[](int idx) const
	{
		return *(&R + idx);
	}
};

class Group
{
public:
	float color[3];
	int n;

	Group() 
	{
		Clear();
	}
	
	void Clear()
	{
		color[0] = color[1] = color[2] = 0.0f;
		n = 0;
	}

	void Add(const ColorRGB& color)
	{
		this->color[0] += color.R;
		this->color[1] += color.G;
		this->color[2] += color.B;
		
		n++;
	}
};

class Image
{
public:
	int w, h, depth;
	unsigned char* data;

	Image(const char* path) 
	{
		data = stbi_load(path, &w, &h, &depth, 0);
	}

	~Image()
	{
		delete[] data;
	}

	void Resize(int new_w, int new_h)
	{
		unsigned char* new_data = new unsigned char[new_w * new_h * depth];
		stbir_resize_uint8(data, w, h, 0, new_data, new_w, new_h, 0, depth);

		delete[] data;
		data = new_data;
		w = new_w;
		h = new_h;
	}

	int GetIdx(int x, int y) const
	{
		return (w * y + x) * depth;
	}

	ColorRGB Get(int x, int y) const
	{
		int idx = GetIdx(x, y);
		return ColorRGB(data[idx], data[idx + 1], data[idx + 2]);
	}

	void Set(int x, int y, const ColorRGB& color)
	{
		int idx = GetIdx(x, y);
		data[idx    ] = color.R;
		data[idx + 1] = color.G;
		data[idx + 2] = color.B;
	}

	void Add(int x, int y, const Vec3& error)
	{
		if(x < w && y < h)
		{
			int idx = GetIdx(x, y);
			data[idx    ] = Clamp(data[idx    ] + error.X, 0, 255);
			data[idx + 1] = Clamp(data[idx + 1] + error.Y, 0, 255);
			data[idx + 2] = Clamp(data[idx + 2] + error.Z, 0, 255);
		}
	}

	void Save(const char* path)
	{
		stbi_write_png(path, w, h, depth, data, 0);
	}
};

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
	
	~KDTree()
	{
		delete left;
		delete right;
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

ColorRGB* KMeans(int w, int h, int depth, unsigned char* data, int k)
{
	//Get k different colors in the image
	std::set<ColorRGB> colors;
	for(int i = 0; i < (w * h * depth) && (int)colors.size() < k; i += depth)
	{
		colors.insert(ColorRGB(data[i], data[i + 1], data[i + 2]));
	}
	//Check that there are at least k colors
	if((int)colors.size() < k)
		k = colors.size();
	
	//ret initialization
	ColorRGB* ret = new ColorRGB[k];
	int i = 0;
	for(std::set<ColorRGB>::iterator it = colors.begin(); it != colors.end(); ++it)
	{
		ret[i ++] = *it;
	}

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
		for(int y = 0; y < h; ++ y)
		{
			for(int x = 0; x < w; ++ x)
			{
				int idx = (w * y + x) * depth;
				ColorRGB color(data[idx], data[idx + 1], data[idx + 2]);

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

		printf("%d\n", dist);
		if(dist == 0)
			break;
	}

	return ret;
}

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
	Octree* tree;
	Group group;
	OctreeNode* nodes[8];
	int level;

	OctreeNode(Octree* tree, int level) : tree(tree), level(level)
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

	void Add(const ColorRGB& color, int level = 0)
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
			nodes[idx]->Add(color, level + 1);
		}
	}

	bool IsLeaf()
	{
		for(int i = 0; i < 8; ++i)
		{
			if(nodes[i])
				return false;
		}
		return true;
	}

	static bool comp(OctreeNode* n0, OctreeNode* n1)
	{
		return n0->group.n > n1->group.n;
	}
};

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
	int current_level = 8;

	while(num_leaves > num_colors)
	{
		if(current_level == 8 || nodes_by_level[current_level].size() == 0)
		{
			current_level --;
			std::sort(nodes_by_level[current_level].begin(), nodes_by_level[current_level].end(), OctreeNode::comp);
		}

		OctreeNode* node_to_reduce = nodes_by_level[current_level].back();
		nodes_by_level[current_level].pop_back();
		
		for(int i = 0; i < 8; ++ i)
		{
			if(node_to_reduce->nodes[i])
			{
				delete node_to_reduce->nodes[i];
				node_to_reduce->nodes[i] = 0;
				num_leaves --;
			}
		}
		num_leaves ++;
	}

	ColorRGB* ret = new ColorRGB[num_leaves];
	int c = 0;
	for(int i = current_level - 1; i <= current_level; ++i)
	{
		for(std::vector< OctreeNode* >::iterator it = nodes_by_level[i].begin(); it != nodes_by_level[i].end(); ++ it)
		{
			for(int j = 0; j < 8; ++j)
			{
				if((*it)->nodes[j] && (*it)->nodes[j]->IsLeaf())
				{
					Group& group = (*it)->nodes[j]->group;
					ret[c ++] = ColorRGB((unsigned char)(group.color[0] / group.n), (unsigned char)(group.color[1] / group.n), (unsigned char)(group.color[2] / group.n));

					if(c > num_leaves)
						bool error = true;
				}
			}
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
			octree.root->Add(image.Get(x, y));
		}
	}

	return octree.GetPalette(num_colors);
}

int main(int argc, char* argv[])
{
	Image img("licensed-image.jpg");
	int k = 32;
	bool dithering = true;

	//img.Resize(160, 144);
	long long start = milliseconds_now();
	ColorRGB* palette = KMeans(img.w, img.h, img.depth, img.data, k);
	//ColorRGB* palette = OctreePalette(img, k);
	long long elapsed = milliseconds_now() - start;
	for(int y = 0; y < img.h; ++y)
	{
		for(int x = 0; x < img.w; ++x)
		{
			int best_k = FindClosest(img.Get(x, y), palette, k);

			//Apply dithering
			if(dithering)
			{
				Vec3 quant_error = img.Get(x, y) - palette[best_k];
				img.Add(x + 1, y    , quant_error * (7.0f / 16.0f));
				img.Add(x - 1, y + 1, quant_error * (3.0f / 16.0f));
				img.Add(x    , y + 1, quant_error * (5.0f / 16.0f));
				img.Add(x + 1, y + 1, quant_error * (1.0f / 16.0f));
			}

			img.Set(x, y, palette[best_k]);
		}
	}
	
	img.Save("output.png");

	printf("Done %lldms", elapsed);
	scanf("");

    return 0;
}

