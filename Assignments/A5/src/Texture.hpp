//
//  Texture.hpp
//  A4
//
//  Created by David-Tyler Ighedosa on 3/29/24.
//
#pragma once
#ifndef Texture_hpp
#define Texture_hpp


#define GLEW_STATIC
#include <GL/glew.h>

#include <string>

class Texture
{
public:
    Texture();
    virtual ~Texture();
    void setFilename(const std::string &f) { filename = f; }
    void init();
    void setUnit(GLint u) { unit = u; }
    GLint getUnit() const { return unit; }
    void bind(GLint handle);
    void unbind();
    void setWrapModes(GLint wrapS, GLint wrapT); // Must be called after init()
    
private:
    std::string filename;
    int width;
    int height;
    GLuint tid;
    GLint unit;
    
};

#endif /* Texture_hpp */
