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

void InputError()
{
	printf("Usage: ZIMGQuant <image> -colors <num colors> -dithering <0 or 1> -output <output path> -method <octree or kmeans>\n");
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		InputError();
		return -1;
	}

	int k = -1;
	bool dithering = true;
	char* output_path = 0;
	
	enum Method
	{
		Method_KMeans,
		Method_Octree
	} 
	method = Method_KMeans;

	for(int i = 2; i < argc; ++i)
	{
		if(!strcmp(argv[i], "-colors"))
		{
			k = atoi(argv[++ i]);
		} 
		else if(!strcmp(argv[i], "-dithering"))
		{
			dithering = atoi(argv[++ i]) == 0 ? false : true;
		}
		else if(!strcmp(argv[i], "-output"))
		{
			output_path = argv[++ i];
		}
		else if(!strcmp(argv[i], "-method"))
		{
			const char* method_str = argv[++ i];
			if(!strcmp(method_str, "kmeans"))
				method = Method_KMeans;
			else if(!strcmp(method_str, "octree"))
				method = Method_Octree;
		}
	}

	if(k == -1 || !output_path)
	{
		InputError();
		return -1;
	}

	Image img(argv[1]);

	//img.Resize(160, 144);

	long long start = milliseconds_now();
	ColorRGB* palette = method == Method_KMeans ? KMeans(img, k) : OctreePalette(img, k);
	img.SetPalette(palette, k, dithering);
	long long elapsed = milliseconds_now() - start;
	printf("Done %lldms\n", elapsed);

	img.Save(output_path);
	scanf("");

    return 0;
}

