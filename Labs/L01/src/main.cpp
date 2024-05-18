#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "Image.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;
struct myStructure {
    int xmin;
    int xmax;
    int ymin;
    int ymax;
} ;

int main(int argc, char **argv)
{
	if(argc < 4) {
		cout << "Usage: L01 filename width height" << endl;
		return 0;
	}
	// Output filename
	string filename(argv[2]);
	// Width of image
	int width = atoi(argv[3]);
	// Height of image
	int height = atoi(argv[4]);
    cout << filename << " " << width << " " << height << endl;

	// Vertex 1 x-coord
	int v1x = atoi(argv[5]);

	// Vertex 1 y-coord
	int v1y = atoi(argv[6]);

	// Vertex 2 x-coord
	int v2x = atoi(argv[7]);

	// Vertex 2 y-coord
	int v2y = atoi(argv[8]);

	// Vertex 3 x-coord
	int v3x = atoi(argv[9]);

	// Vertex 3 y-coord
	int v3y = atoi(argv[10]);
    
    vector<int> xVert{ v1x, v2x, v3x };
    vector<int> yVert{ v1y, v2y, v3y };
    struct myStructure tri;
    tri.xmax = v1x;
    tri.xmin = v1x;
    tri.ymax = v1y;
    tri.ymin = v1y;
    for (int i = 1; i < 3; i++) {
        if (xVert[i] >= tri.xmax) {
            tri.xmax = xVert[i];
        }
        if (yVert[i] >= tri.ymax) {
            tri.ymax = yVert[i];
        }
        if (xVert[i] <= tri.xmin) {
            tri.xmin = xVert[i] + 1;
        }
        if (yVert[i] <= tri.ymin) {
            tri.ymin = yVert[i] + 1;
        }

    }
	// Create the image. We're using a `shared_ptr`, a C++11 feature.
	
	auto image = make_shared<Image>(width, height);
    
    unsigned char red = 255;
    unsigned char green = 255;
    unsigned char blue = 255;
	image->setPixel(v1x, v1y, red, green, blue);
	image->setPixel(v2x, v2y, red, green, blue);
	image->setPixel(v3x, v3y, red, green, blue);

	// Draw a rectangle
    unsigned char r = 255;
    unsigned char g = 0;
    unsigned char b = 0;
	for(int y = tri.ymin; y < tri.ymax; ++y) {
        
		for(int x = tri.xmin; x < tri.xmax; x+=3) {
            if (x %2 == 0) {
                r = 255;
                g = 0;
                b = 0;
            }
            else{
                r = 0;
                g = 255;
                b = 0;
            }
			image->setPixel(x, y, r, g, b);
		}
	}
	
	// Write image to file
	image->writeToFile(filename);
	return 0;
}
