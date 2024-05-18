#include <iostream>
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Image.h"
#include "Structures.h"
#include <cfloat>
#include <vector>
#include <limits>
#include <cmath>

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;


double RANDOM_COLORS[7][3] = {
	{0.0000,    0.4470,    0.7410},
	{0.8500,    0.3250,    0.0980},
	{0.9290,    0.6940,    0.1250},
	{0.4940,    0.1840,    0.5560},
	{0.4660,    0.6740,    0.1880},
	{0.3010,    0.7450,    0.9330},
	{0.6350,    0.0780,    0.1840},
};


float area(Vertex V0, Vertex V1, Vertex V2)
{
    return (0.5 * ((V1.x - V0.x)*(V2.y - V0.y) - (V2.x - V0.x) * (V1.y - V0.y)));
}


Color interpolateColor(const Color& color1, const Color& color2, double t) {
    Color result;
    result.r = static_cast<int>((1 - t) * color1.r + t * color2.r);
    result.g = static_cast<int>((1 - t) * color1.g + t * color2.g); // this is just 0
    result.b = static_cast<int>((1 - t) * color1.b + t * color2.b);
    return result;
}


Color calculateInterpolatedColor(float ymin, float ymax, float y) {
    // Normalize y-value within the range [0, 1]
    double t = (y - ymin) / (ymax - ymin);

    Color blue = {0, 0, 255};
    Color red = {255, 0, 0};

    // Interpolate between blue and red
    return interpolateColor(blue, red, t);
}


int main(int argc, char **argv)
{
	if(argc < 5) {
		cout << "Not enough arguments given" << endl;
        cout << "Required Arguments:" << endl;
        cout << "Input filename of .obj file to rasterize" << endl;
        cout << "Output image filename (should be png)" << endl;
        cout << "Image width" << endl;
        cout << "Image height" << endl;
        cout << "Task number (1 through 7)" << endl;
        
		return 0;
	}
	string meshName(argv[2]);
    string output_filename(argv[3]);
    float width = atoi(argv[4]);
    float height = atoi(argv[5]);
    int task = atoi(argv[6]);

	// Load geometry
	vector<float> posBuf; // list of vertex positions
	vector<float> norBuf; // list of vertex normals
	vector<float> texBuf; // list of vertex texture coords
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		// Some OBJ files have different indices for vertex positions, normals,
		// and texture coordinates. For example, a cube corner vertex may have
		// three different normals. Here, we are going to duplicate all such
		// vertices.
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			size_t index_offset = 0;
			for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				size_t fv = shapes[s].mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);
					if(!attrib.normals.empty()) {
						norBuf.push_back(attrib.normals[3*idx.normal_index+0]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+1]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+2]);
					}
					if(!attrib.texcoords.empty()) {
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
					}
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}
	cout << "Number of vertices: " << posBuf.size()/3 << endl;
    
    auto image = make_shared<Image>(width, height);
    
    // Task 1
    if (task == 1) {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }

            // Creating Bounding Box
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    image->setPixel(x, y, RANDOM_COLORS[(i/9)%7][0] * 255, RANDOM_COLORS[(i/9)%7][1] * 255, RANDOM_COLORS[(i/9)%7][2] * 255) ;
                }
            }
            
        }
       
    }
    
    // Task 2
    else if (task == 2)
    {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }
            
            float areaTotal = area(someTriangle.v1, someTriangle.v2, someTriangle.v3);
            // Creating Triangle
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    Vertex P;
                    P.x = float(x);
                    P.y = float(y);
                    
                    
                    float areaA  = area(P, someTriangle.v2, someTriangle.v3);
                    float areaB  = area(P, someTriangle.v3, someTriangle.v1);
                    float areaC  = area(P, someTriangle.v1, someTriangle.v2);
                    
                    float a = areaA/areaTotal;
                    float b = areaB/areaTotal;
                    float c = areaC/areaTotal;
                    
                    if ( (a > -FLT_EPSILON && a <=1) && (b > -FLT_EPSILON && b <=1) && (c > -FLT_EPSILON && c <=1)) {
                        image->setPixel(P.x, P.y, RANDOM_COLORS[(i/9)%7][0] * 255, RANDOM_COLORS[(i/9)%7][1] * 255, RANDOM_COLORS[(i/9)%7][2] * 255) ;
                    }
                    
                }
            }
            
        }
    }
    // Task 3
    else if (task == 3)
    {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }
            
            float areaTotal = area(someTriangle.v1, someTriangle.v2, someTriangle.v3);
            // Creating Triangle
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    Vertex P;
                    P.x = float(x);
                    P.y = float(y);
                    
                    
                    float areaA  = area(P, someTriangle.v2, someTriangle.v3);
                    float areaB  = area(P, someTriangle.v3, someTriangle.v1);
                    float areaC  = area(P, someTriangle.v1, someTriangle.v2);
                    
                    float a = areaA/areaTotal;
                    float b = areaB/areaTotal;
                    float c = areaC/areaTotal;
                    
                    if ( (a > -FLT_EPSILON && a <=1) && (b > -FLT_EPSILON && b <=1) && (c > -FLT_EPSILON && c <=1)) {
                        // interpolate colors
                        
                        float newR = 255 * ((a*RANDOM_COLORS[ ((i/9)*3)%7][0]) + (b*RANDOM_COLORS[((i/9)*3+ 1)%7][0]) + (c*RANDOM_COLORS[((i/9)*3 + 2)%7][0]));
                        
                        float newG = 255 * ((a*RANDOM_COLORS[ ((i/9)*3)%7][1]) + (b*RANDOM_COLORS[((i/9)*3+ 1)%7][1]) + (c*RANDOM_COLORS[((i/9)*3 + 2)%7][1]));
                        
                        float newB = 255 * ((a*RANDOM_COLORS[ ((i/9)*3)%7][2]) + (b*RANDOM_COLORS[((i/9)*3+ 1)%7][2]) + (c*RANDOM_COLORS[((i/9)*3 + 2)%7][2]));
                        
                        
                        if (newR < 0) {
                            newR = 0;
                        }
                        else if (newR > 255)
                        {
                            newR = 255;
                        }
                        
                        if (newG < 0) {
                            newG = 0;
                        }
                        else if (newG > 255)
                        {
                            newG = 255;
                        }
                        
                        if (newB < 0) {
                            newB = 0;
                        }
                        else if (newB > 255)
                        {
                            newB = 255;
                        }
                        
                        
                        image->setPixel(x, y, newR, newG, newB) ;
                    }
                    
                }
            }
            
        }
    }
    
    
    // Task 4
    else if (task == 4)
    {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
            
            
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        
        
        yminPB = yminPB * scaleFactor;
        ymaxPB = ymaxPB * scaleFactor;
        
        yminPB += yShift;
        ymaxPB += yShift;
        
        
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }
            
            float areaTotal = area(someTriangle.v1, someTriangle.v2, someTriangle.v3);
            // Creating Triangle
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    Vertex P;
                    P.x = float(x);
                    P.y = float(y);
                    
                    
                    float areaA  = area(P, someTriangle.v2, someTriangle.v3);
                    float areaB  = area(P, someTriangle.v3, someTriangle.v1);
                    float areaC  = area(P, someTriangle.v1, someTriangle.v2);
                    
                    float a = areaA/areaTotal;
                    float b = areaB/areaTotal;
                    float c = areaC/areaTotal;
                    
                    if ( (a > -FLT_EPSILON && a <=1) && (b > -FLT_EPSILON && b <=1) && (c > -FLT_EPSILON && c <=1)) {
                        // interpolate colors
                        
                        Color interpolatedColor = calculateInterpolatedColor(yminPB, ymaxPB, y);
                        
                        image->setPixel(x, y, interpolatedColor.r, interpolatedColor.g, interpolatedColor.b);
                    }
                    
                }
            }
            
        }
    }
    
    // Task 5
    else if (task == 5)
    {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        float zminPB = posBuf[2];
        float zmaxPB = posBuf[2];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
            if (posBuf[i+2] > zmaxPB) {
                zmaxPB = posBuf[i+2];
            }
            else if (posBuf[i+2] < zminPB) {
                zminPB = posBuf[i+2];
            }
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        yminPB = yminPB * scaleFactor;
        ymaxPB = ymaxPB * scaleFactor;
        
        yminPB += yShift;
        ymaxPB += yShift;
        
        
        zminPB = zminPB * scaleFactor;
        zmaxPB = zmaxPB * scaleFactor;
        
        
        vector<vector<float>> zBuffer(height, vector<float>(width, static_cast<float>(numeric_limits<int>::min())));
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }
            
            float areaTotal = area(someTriangle.v1, someTriangle.v2, someTriangle.v3);
            // Creating Triangle
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    Vertex P;
                    P.x = float(x);
                    P.y = float(y);
                    
                    
                    float areaA  = area(P, someTriangle.v2, someTriangle.v3);
                    float areaB  = area(P, someTriangle.v3, someTriangle.v1);
                    float areaC  = area(P, someTriangle.v1, someTriangle.v2);
                    
                    float a = areaA/areaTotal;
                    float b = areaB/areaTotal;
                    float c = areaC/areaTotal;
                    
                    if ( (a > -FLT_EPSILON && a <=1) && (b > -FLT_EPSILON && b <=1) && (c > -FLT_EPSILON && c <=1)) {
                        // interpolate colors
                        
                        float interpolatedZ = (a * someTriangle.v1.z) + (b * someTriangle.v2.z) + (c * someTriangle.v3.z);
                        
                        if (interpolatedZ > zBuffer[x][y]) {
                            double t = (interpolatedZ - zminPB) / (zmaxPB - zminPB);
                            // Map normalized y-value to the range [0, 255]
                            int red = static_cast<int>(t * 255);
                            
                            image->setPixel(x, y,  red , 0, 0);
                            zBuffer[x][y] = interpolatedZ;
                        }
                        
                        
                    }
                    
                }
            }
            
        }
    }
    // Task 6
    else if (task == 6)
    {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        float zminPB = posBuf[2];
        float zmaxPB = posBuf[2];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
            if (posBuf[i+2] > zmaxPB) {
                zmaxPB = posBuf[i+2];
            }
            else if (posBuf[i+2] < zminPB) {
                zminPB = posBuf[i+2];
            }
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        yminPB = yminPB * scaleFactor;
        ymaxPB = ymaxPB * scaleFactor;
        
        yminPB += yShift;
        ymaxPB += yShift;
        
        
        zminPB = zminPB * scaleFactor;
        zmaxPB = zmaxPB * scaleFactor;
        
        
        vector<vector<float>> zBuffer(height, vector<float>(width, static_cast<float>(numeric_limits<int>::min())));
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            // Normal Vectors
            someTriangle.v1.nx = norBuf[i-8];
            someTriangle.v1.ny = norBuf[i-7];
            someTriangle.v1.nz = norBuf[i-6];

            someTriangle.v2.nx = norBuf[i-5];
            someTriangle.v2.ny = norBuf[i-4];
            someTriangle.v2.nz = norBuf[i-3];

            someTriangle.v3.nx = norBuf[i-2];
            someTriangle.v3.ny = norBuf[i-1];
            someTriangle.v3.nz = norBuf[i];
            
            
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }
            
            float areaTotal = area(someTriangle.v1, someTriangle.v2, someTriangle.v3);
            // Creating Triangle
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    Vertex P;
                    P.x = float(x);
                    P.y = float(y);
                    
                    
                    float areaA  = area(P, someTriangle.v2, someTriangle.v3);
                    float areaB  = area(P, someTriangle.v3, someTriangle.v1);
                    float areaC  = area(P, someTriangle.v1, someTriangle.v2);
                    
                    float a = areaA/areaTotal;
                    float b = areaB/areaTotal;
                    float c = areaC/areaTotal;
                    
                    if ( (a > -FLT_EPSILON && a <=1) && (b > -FLT_EPSILON && b <=1) && (c > -FLT_EPSILON && c <=1)) 
                    {
                        float interpolatedZ = (a * someTriangle.v1.z) + (b * someTriangle.v2.z) + (c * someTriangle.v3.z);
                        
                        if (interpolatedZ > zBuffer[x][y]) {
                            Vertex interpolatedNormals;
                            interpolatedNormals.nx = (a * someTriangle.v1.nx) + (b * someTriangle.v2.nx) + (c * someTriangle.v3.nx);
                            interpolatedNormals.ny = (a * someTriangle.v1.ny) + (b * someTriangle.v2.ny) + (c * someTriangle.v3.ny);
                            interpolatedNormals.nz = (a * someTriangle.v1.nz) + (b * someTriangle.v2.nz) + (c * someTriangle.v3.nz);
                            
                            float r = 255 * (0.5 * interpolatedNormals.nx + 0.5);
                            float g = 255 * (0.5 * interpolatedNormals.ny + 0.5);
                            float b = 255 * (0.5 * interpolatedNormals.nz + 0.5);

                            // interpolate colors
                            
                            image->setPixel(x, y,  r , g, b);
                            zBuffer[x][y] = interpolatedZ;
                        }
                        
                        
                    }
                    
                }
            }
            
        }
    }
    
    // Task 7
    else if (task == 7)
    {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        float zminPB = posBuf[2];
        float zmaxPB = posBuf[2];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
            if (posBuf[i+2] > zmaxPB) {
                zmaxPB = posBuf[i+2];
            }
            else if (posBuf[i+2] < zminPB) {
                zminPB = posBuf[i+2];
            }
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        yminPB = yminPB * scaleFactor;
        ymaxPB = ymaxPB * scaleFactor;
        
        yminPB += yShift;
        ymaxPB += yShift;
        
        
        zminPB = zminPB * scaleFactor;
        zmaxPB = zmaxPB * scaleFactor;
        
        
        vector<vector<float>> zBuffer(height, vector<float>(width, static_cast<float>(numeric_limits<int>::min())));
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            // Normal Vectors
            someTriangle.v1.nx = norBuf[i-8];
            someTriangle.v1.ny = norBuf[i-7];
            someTriangle.v1.nz = norBuf[i-6];

            someTriangle.v2.nx = norBuf[i-5];
            someTriangle.v2.ny = norBuf[i-4];
            someTriangle.v2.nz = norBuf[i-3];

            someTriangle.v3.nx = norBuf[i-2];
            someTriangle.v3.ny = norBuf[i-1];
            someTriangle.v3.nz = norBuf[i];
            
            
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }
            
            float areaTotal = area(someTriangle.v1, someTriangle.v2, someTriangle.v3);
            // Creating Triangle
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    Vertex P;
                    P.x = float(x);
                    P.y = float(y);
                    
                    
                    float areaA  = area(P, someTriangle.v2, someTriangle.v3);
                    float areaB  = area(P, someTriangle.v3, someTriangle.v1);
                    float areaC  = area(P, someTriangle.v1, someTriangle.v2);
                    
                    float a = areaA/areaTotal;
                    float b = areaB/areaTotal;
                    float c = areaC/areaTotal;
                    
                    if ( (a > -FLT_EPSILON && a <=1) && (b > -FLT_EPSILON && b <=1) && (c > -FLT_EPSILON && c <=1))
                    {
                        float interpolatedZ = (a * someTriangle.v1.z) + (b * someTriangle.v2.z) + (c * someTriangle.v3.z);
                        
                        if (interpolatedZ > zBuffer[x][y]) {
                            
                            Vertex interpolatedNormals;
                            interpolatedNormals.nx = (a * someTriangle.v1.nx) + (b * someTriangle.v2.nx) + (c * someTriangle.v3.nx);
                            interpolatedNormals.ny = (a * someTriangle.v1.ny) + (b * someTriangle.v2.ny) + (c * someTriangle.v3.ny);
                            interpolatedNormals.nz = (a * someTriangle.v1.nz) + (b * someTriangle.v2.nz) + (c * someTriangle.v3.nz);
                            
                            // Matrix dot product l * n
                            float l0 = (1/sqrt(3));
                            float l1 = (1/sqrt(3));
                            float l2 = (1/sqrt(3));
                            
                            float scalar_c = (l0 * interpolatedNormals.nx) + (l1 * interpolatedNormals.ny) + (l2 * interpolatedNormals.nz);
                            
                            if (scalar_c < 0) {
                                scalar_c = 0;
                            }
                            
                            image->setPixel(x, y, scalar_c * 255 , scalar_c * 255  , scalar_c * 255);
                            zBuffer[x][y] = interpolatedZ;
                        }
                        
                        
                    }
                    
                }
            }
            
        }
    }
    
    // Task 8
    else if (task == 8)
    {
        
        float xminPB = posBuf[0];
        float xmaxPB = posBuf[0];
        float yminPB = posBuf[1];
        float ymaxPB = posBuf[1];
        float zminPB = posBuf[2];
        float zmaxPB = posBuf[2];
        
        for (int i = 3; i < posBuf.size(); i += 3) {
            if (posBuf[i] > xmaxPB) {
                xmaxPB = posBuf[i];
            }
            
            else if (posBuf[i] < xminPB) {
                xminPB = posBuf[i];
            }
            
            if (posBuf[i+1] > ymaxPB) {
                ymaxPB = posBuf[i+1];
            }
            else if (posBuf[i+1] < yminPB) {
                yminPB = posBuf[i+1];
            }
            
            if (posBuf[i+2] > zmaxPB) {
                zmaxPB = posBuf[i+2];
            }
            else if (posBuf[i+2] < zminPB) {
                zminPB = posBuf[i+2];
            }
        }
        
        
        float objHeight = ymaxPB - yminPB;
        float objWidth = xmaxPB - xminPB;
        
        
        
        float scaleFactor = width/objWidth;
        
        if (height/objHeight < scaleFactor) {
            scaleFactor = height/objHeight;
        }
        
        
        float xShift = ((width - (objWidth * scaleFactor)) / 2.0) - (xminPB * scaleFactor);
        float yShift = ((height - (objHeight * scaleFactor)) / 2.0) - (yminPB * scaleFactor);
        
        yminPB = yminPB * scaleFactor;
        ymaxPB = ymaxPB * scaleFactor;
        
        yminPB += yShift;
        ymaxPB += yShift;
        
        
        zminPB = zminPB * scaleFactor;
        zmaxPB = zmaxPB * scaleFactor;
        
        
        vector<vector<float>> zBuffer(height, vector<float>(width, static_cast<float>(numeric_limits<int>::min())));
        
        for (int i = 8; i < posBuf.size(); i+=9)
        {
            Triangle someTriangle;

            someTriangle.v1.x = posBuf[i-8];
            someTriangle.v1.y = posBuf[i-7];
            someTriangle.v1.z = posBuf[i-6];

            someTriangle.v2.x = posBuf[i-5];
            someTriangle.v2.y = posBuf[i-4];
            someTriangle.v2.z = posBuf[i-3];

            someTriangle.v3.x = posBuf[i-2];
            someTriangle.v3.y = posBuf[i-1];
            someTriangle.v3.z = posBuf[i];
            
            // ROTATING POSITIONS
            // cos(pi/4), sin(pi/4) == sqrt2 / 2, sqrt2 / 2
            
            float Rotated_v1_x = ((cos(sqrt(2)/ 2) * someTriangle.v1.x) + (sin(sqrt(2)/ 2) * someTriangle.v1.z));
            float Rotated_v1_y = (someTriangle.v1.y);
            float Rotated_v1_z = ((-sin(sqrt(2)/ 2) * someTriangle.v1.x) + (cos(sqrt(2)/ 2) * someTriangle.v1.z));
            
            float Rotated_v2_x = ((cos(sqrt(2)/ 2) * someTriangle.v2.x) + (sin(sqrt(2)/ 2) * someTriangle.v2.z));
            float Rotated_v2_y = (someTriangle.v2.y);
            float Rotated_v2_z = ((-sin(sqrt(2)/ 2) * someTriangle.v2.x) + (cos(sqrt(2)/ 2) * someTriangle.v2.z));
            
            float Rotated_v3_x = ((cos(sqrt(2)/ 2) * someTriangle.v3.x) + (sin(sqrt(2)/ 2) * someTriangle.v3.z));
            float Rotated_v3_y = (someTriangle.v3.y);
            float Rotated_v3_z = ((-sin(sqrt(2)/ 2) * someTriangle.v3.x) + (cos(sqrt(2)/ 2) * someTriangle.v3.z));
            
            posBuf[i-8] = Rotated_v1_x;
            posBuf[i-7] = Rotated_v1_y;
            posBuf[i-6] = Rotated_v1_z;

            posBuf[i-5] = Rotated_v2_x;
            posBuf[i-4] = Rotated_v2_y;
            posBuf[i-3] = Rotated_v2_z;

            posBuf[i-2] = Rotated_v3_x;
            posBuf[i-1] = Rotated_v3_y;
            posBuf[i] = Rotated_v3_z;
            
            someTriangle.v1.x = Rotated_v1_x;
            someTriangle.v1.y = Rotated_v1_y;
            someTriangle.v1.z = Rotated_v1_z;

            someTriangle.v2.x = Rotated_v2_x;
            someTriangle.v2.y = Rotated_v2_y;
            someTriangle.v2.z = Rotated_v2_z;

            someTriangle.v3.x = Rotated_v3_x;
            someTriangle.v3.y = Rotated_v3_y;
            someTriangle.v3.z = Rotated_v3_z;
            
            someTriangle.v1.z = someTriangle.v1.z * scaleFactor;
            someTriangle.v1.y = someTriangle.v1.y * scaleFactor;
            someTriangle.v1.x = someTriangle.v1.x * scaleFactor;

            someTriangle.v2.z = someTriangle.v2.z * scaleFactor;
            someTriangle.v2.y = someTriangle.v2.y * scaleFactor;
            someTriangle.v2.x = someTriangle.v2.x * scaleFactor;

            someTriangle.v3.z = someTriangle.v3.z * scaleFactor;
            someTriangle.v3.y = someTriangle.v3.y * scaleFactor;
            someTriangle.v3.x = someTriangle.v3.x * scaleFactor;
            
            someTriangle.v1.y = someTriangle.v1.y + yShift;
            someTriangle.v1.x = someTriangle.v1.x + xShift;
            
            someTriangle.v2.y = someTriangle.v2.y + yShift;
            someTriangle.v2.x = someTriangle.v2.x + xShift;
            
            someTriangle.v3.y = someTriangle.v3.y + yShift;
            someTriangle.v3.x = someTriangle.v3.x + xShift;
            
            
            someTriangle.xmin = someTriangle.v1.x;
            someTriangle.xmax = someTriangle.v1.x;
            
            someTriangle.ymin = someTriangle.v1.y;
            someTriangle.ymax = someTriangle.v1.y;
            
            // Normal Vectors
            someTriangle.v1.nx = norBuf[i-8];
            someTriangle.v1.ny = norBuf[i-7];
            someTriangle.v1.nz = norBuf[i-6];

            someTriangle.v2.nx = norBuf[i-5];
            someTriangle.v2.ny = norBuf[i-4];
            someTriangle.v2.nz = norBuf[i-3];

            someTriangle.v3.nx = norBuf[i-2];
            someTriangle.v3.ny = norBuf[i-1];
            someTriangle.v3.nz = norBuf[i];
            
            float Rotated_v1_nx = ((cos(sqrt(2)/ 2) * someTriangle.v1.nx) + (sin(sqrt(2)/ 2) * someTriangle.v1.nz));
            float Rotated_v1_ny = (someTriangle.v1.ny);
            float Rotated_v1_nz = ((-sin(sqrt(2)/ 2) * someTriangle.v1.nx) + (cos(sqrt(2)/ 2) * someTriangle.v1.nz));
            
            float Rotated_v2_nx = ((cos(sqrt(2)/ 2) * someTriangle.v2.nx) + (sin(sqrt(2)/ 2) * someTriangle.v2.nz));
            float Rotated_v2_ny = (someTriangle.v2.ny);
            float Rotated_v2_nz = ((-sin(sqrt(2)/ 2) * someTriangle.v2.nx) + (cos(sqrt(2)/ 2) * someTriangle.v2.nz));
            
            float Rotated_v3_nx = ((cos(sqrt(2)/ 2) * someTriangle.v3.nx) + (sin(sqrt(2)/ 2) * someTriangle.v3.nz));
            float Rotated_v3_ny = (someTriangle.v3.ny);
            float Rotated_v3_nz = ((-sin(sqrt(2)/ 2) * someTriangle.v3.nx) + (cos(sqrt(2)/ 2) * someTriangle.v3.nz));
            
            norBuf[i-8] = Rotated_v1_nx;
            norBuf[i-7] = Rotated_v1_ny;
            norBuf[i-6] = Rotated_v1_nz;

            norBuf[i-5] = Rotated_v2_nx;
            norBuf[i-4] = Rotated_v2_ny;
            norBuf[i-3] = Rotated_v2_nz;

            norBuf[i-2] = Rotated_v3_nx;
            norBuf[i-1] = Rotated_v3_ny;
            norBuf[i] = Rotated_v3_nz;
            
            someTriangle.v1.nx = Rotated_v1_nx;
            someTriangle.v1.ny = Rotated_v1_ny;
            someTriangle.v1.nz = Rotated_v1_nz;

            someTriangle.v2.nx = Rotated_v2_nx;
            someTriangle.v2.ny = Rotated_v2_ny;
            someTriangle.v2.nz = Rotated_v2_nz;

            someTriangle.v3.nx = Rotated_v3_nx;
            someTriangle.v3.ny = Rotated_v3_ny;
            someTriangle.v3.nz = Rotated_v3_nz;
            
            //  comparison for max
            if (someTriangle.v2.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v2.x;
            }
            if (someTriangle.v3.x >= someTriangle.xmax)
            {
                someTriangle.xmax = someTriangle.v3.x;
            }

            if (someTriangle.v2.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v2.y;
            }
            if (someTriangle.v3.y >= someTriangle.ymax)
            {
                someTriangle.ymax = someTriangle.v3.y;
            }
            
            if (someTriangle.v2.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v2.x;
            }
            if (someTriangle.v3.x <= someTriangle.xmin)
            {
                someTriangle.xmin = someTriangle.v3.x;
            }
            
            if (someTriangle.v2.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v2.y;
            }
            if (someTriangle.v3.y <= someTriangle.ymin)
            {
                someTriangle.ymin = someTriangle.v3.y;
            }
            
            float areaTotal = area(someTriangle.v1, someTriangle.v2, someTriangle.v3);
            // Creating Triangle
            for(int y = someTriangle.ymin; y < someTriangle.ymax; ++y)
            {
                for(int x = someTriangle.xmin; x < someTriangle.xmax; x++)
                {
                    Vertex P;
                    P.x = float(x);
                    P.y = float(y);
                    
                    
                    float areaA  = area(P, someTriangle.v2, someTriangle.v3);
                    float areaB  = area(P, someTriangle.v3, someTriangle.v1);
                    float areaC  = area(P, someTriangle.v1, someTriangle.v2);
                    
                    float a = areaA/areaTotal;
                    float b = areaB/areaTotal;
                    float c = areaC/areaTotal;
                    
                    if ( (a > -FLT_EPSILON && a <=1) && (b > -FLT_EPSILON && b <=1) && (c > -FLT_EPSILON && c <=1))
                    {
                        float interpolatedZ = (a * someTriangle.v1.z) + (b * someTriangle.v2.z) + (c * someTriangle.v3.z);
                        
                        if (interpolatedZ > zBuffer[x][y]) {
                            
                            Vertex interpolatedNormals;
                            interpolatedNormals.nx = (a * someTriangle.v1.nx) + (b * someTriangle.v2.nx) + (c * someTriangle.v3.nx);
                            interpolatedNormals.ny = (a * someTriangle.v1.ny) + (b * someTriangle.v2.ny) + (c * someTriangle.v3.ny);
                            interpolatedNormals.nz = (a * someTriangle.v1.nz) + (b * someTriangle.v2.nz) + (c * someTriangle.v3.nz);
                            
                            // Matrix dot product l * n
                            float l0 = (1/sqrt(3));
                            float l1 = (1/sqrt(3));
                            float l2 = (1/sqrt(3));
                            
                            float scalar_c = (l0 * interpolatedNormals.nx) + (l1 * interpolatedNormals.ny) + (l2 * interpolatedNormals.nz);
                            
                            if (scalar_c < 0) {
                                scalar_c = 0;
                            }
                            
                            image->setPixel(x, y, scalar_c * 255 , scalar_c * 255  , scalar_c * 255);
                            zBuffer[x][y] = interpolatedZ;
                        }
                    }
                }
            }
            
        }
    }
    
    

	//write image to file
	image->writeToFile(output_filename);
	
}
