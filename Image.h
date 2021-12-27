#ifndef IMAGE_H
#define IMAGE_H

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

int Clamp(int v, int min, int max);

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

int FindClosest(const ColorRGB color, ColorRGB* pal, int pal_size);

#endif