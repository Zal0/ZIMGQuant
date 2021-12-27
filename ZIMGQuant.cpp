#include "Image.h"
#include "KMeans.h"
#include "Octree.h"
#include <windows.h>

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

int main(int argc, char* argv[])
{
	Image img("licensed-image.jpg");
	int k = 32;
	bool dithering = true;

	//img.Resize(160, 144);
	long long start = milliseconds_now();
	ColorRGB* palette = KMeans(img, k);
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

