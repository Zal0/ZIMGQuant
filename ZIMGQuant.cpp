#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#include <math.h>
#include <float.h>
#include <set>

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

int FindClosest(const ColorRGB color, ColorRGB* pal, int pal_size)
{
	//Locate the closest centroid
	int dist = INT_MAX;
	int best_k = 0;
	for(int c = 0; c < pal_size ; ++ c)
	{
		int d = color.Dist(pal[c]);
		if(d < dist)
		{
			best_k = c;
			dist = d;
		}
	}
	return best_k;
}

ColorRGB* KMeans(int w, int h, int depth, unsigned char* data, int k)
{
	//Get k different colors in the image
	std::set<ColorRGB> colors;
	for(int i = 0; i < (w * h * depth) && colors.size() < k; i += depth)
	{
		colors.insert(ColorRGB(data[i], data[i + 1], data[i + 2]));
	}
	//Check that there are at least k colors
	if(colors.size() < k)
		k = colors.size();
	
	//ret initialization
	ColorRGB* ret = new ColorRGB[k];
	int i = 0;
	for(std::set<ColorRGB>::iterator it = colors.begin(); it != colors.end(); ++it)
	{
		ret[i ++] = *it;
	}

	Group* groups = new Group[k];

	
	while(true)
	{
		for(int c = 0; c < k; ++c) 
		{
			groups[c].Clear();
		}

		//Group pixels by their closest centroid
		for(int y = 0; y < h; ++ y)
		{
			for(int x = 0; x < w; ++ x)
			{
				int idx = (w * y + x) * depth;
				ColorRGB color(data[idx], data[idx + 1], data[idx + 2]);

				//Locate the closest centroid
				int best_k = FindClosest(color, ret, k);
				groups[best_k].Add(color);
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

int main(int argc, char* argv[])
{
	Image img("fOcIL.png");
	int k = 32;
	bool dithering = true;

	//img.Resize(160, 144);
	ColorRGB* palette = KMeans(img.w, img.h, img.depth, img.data, k);
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

    return 0;
}

