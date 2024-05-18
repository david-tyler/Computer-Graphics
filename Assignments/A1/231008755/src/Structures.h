//
//  Structures.h
//  A1
//
//  Created by David-Tyler Ighedosa on 2/7/24.
//

#ifndef Structures_h
#define Structures_h

#include <stdio.h>
#include <string>
#include <vector>
#include "Image.h"

using namespace std;

struct Color {
    float r;
    float g;
    float b;
};
struct Vertex{
    float x;
    float y;
    float z;

    float nx;
    float ny;
    float nz;
};

struct Triangle {
    float xmin;
    float xmax;
    float ymin;
    float ymax;
    Vertex v1;
    Vertex v2;
    Vertex v3;


};



#endif /* Structures_h */
