#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>


#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>


#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Image.h"

#include <math.h>

#define EPSILON 0.000001
#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2];

using namespace std;
using namespace glm;

string RESOURCE_DIR = "../../resources" +  string("/"); // Where the resources are loaded from

shared_ptr<Shape> shape;
std::vector<glm::vec3> rays;
float smallestT = std::numeric_limits<int>::max();

struct Light {
    glm::vec3 position;
    float intensity;
};

class Material {
public:
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 ambient;
    float exponent;
};

class ManualCamera {
private:
    glm::vec3 position;
    float fov;

public:
    ManualCamera(glm::vec3 _position, float _fov)
            : position(_position), fov(_fov) {}

    glm::vec3 getPosition() {
            return position;
        }
    void changePosition(vec3 newPosition)
    {
        position = newPosition;
    }
    void changeFOV(float newFOV)
    {
        fov = newFOV;
    }
    float getFOV()
    {
        return fov;
    }
};


class Object {
public:
    virtual vec3 findHit(float t, vec3 pw, vec3 vw) = 0;
    virtual vec3 computeRayColor(vec3 cameraPosition, vec3 rayDirection, vector<Light>& lights, vector<Object*>& objects, Object& object, vec3 hitPoint, int depth) = 0;
    virtual float intersectTest(vec3 cameraPos, vec3 rayDirection) = 0;
    virtual bool doHit(vec3 cameraPos, vec3 rayDirection) = 0;
};

class Sphere : public Object {
private:
    vec3 center;
    float radius;
    vec3 diffuseColor;
    vec3 specularColor;
    vec3 ambientColor;
    float exponent;
    
public:
    Sphere(vec3 _center, float _radius, vec3 _diffuseColor, vec3 _specularColor, vec3 _ambientColor, float _exponent)
        : center(_center), radius(_radius), diffuseColor(_diffuseColor), specularColor(_specularColor), ambientColor(_ambientColor), exponent(_exponent) {}

    vec3 findHit(float t, vec3 pw, vec3 vw) override
    {
        vec3 hitPoint = pw + t * vw;
        return hitPoint;
        
    }
    vec3 computeRayColor(vec3 cameraPosition, vec3 rayDirection, vector<Light>& lights, vector<Object*>& objects, Object& object, vec3 hitPoint, int depth) override {
        vec3 ca = ambientColor;
        vec3 color = ca;
        
        for(int i = 0; i < lights.size(); i ++){
            Light light = lights[i];
            // Calculate the direction from the intersection point to the lights
            vec3 lightDirection = normalize(light.position - hitPoint);
            
            vec3 normal = (hitPoint - center) / radius;
            
            
            // Calculate the diffuse and specular components
            float diffuseFactor = std::max(0.0f, dot(normal, lightDirection));
            vec3 cd = diffuseColor * diffuseFactor;
            
            glm::vec3 viewDirection = normalize(-rayDirection);
            
            glm::vec3 halfwayDirection = glm::normalize(lightDirection + viewDirection);
            float specularFactor = std::max(0.0f, pow(dot(halfwayDirection, normal), exponent));
            vec3 cs = specularColor * specularFactor;
           
            
            float epsilon = 0.01f;
            bool inShadow = false;
            for (auto& object : objects) {
                //                          shadow ray is this
                float distToLight = distance(hitPoint, light.position);
                float t = object->intersectTest((hitPoint + lightDirection * epsilon) , lightDirection);
                if(t <= distToLight){
                    inShadow = object->doHit((hitPoint + lightDirection * epsilon) , lightDirection);
                    if(inShadow == true)
                    {
                        
                        break;
                    }
                }
            }
            if(!inShadow)
            {
                color += (light.intensity * ( cd + cs));
            }
            else{
                color += (0.0f * (cd + cs));
            }
            
        }
        return color;
    }
    
    float intersectTest(vec3 cameraPos, vec3 rayDirection) override {
        vec3 pw = cameraPos; // ray position
        vec3 vw = rayDirection; // ray direction
        vec3 newPC = vec3(pw) - center;
        float a = dot(vw, vw);
        float b = 2.0f * dot(vw, newPC);
        float newC = dot(newPC, newPC) - pow(radius, 2.0f);
        float d = pow(b, 2) - (4 * a * newC);
        
        if (d > 0){
            float t1 = (-b + sqrt(d)) / (2.0f * a);
            float t2 = (-b - sqrt(d)) / (2.0f * a);
            
            if (t1 > 0.0f && t2 > 0.0f){
                return std::min(t1, t2);
            }
            if (t1 < 0.0f && t2 > 0.0f)
            {
                return t2;
            }
            if (t1 > 0.0f && t2 < 0.0f)
            {
                return t1;
            }
        }
        return -1.0f;
    }
    bool doHit(vec3 cameraPos, vec3 rayDirection) override
    {
        
        float t = intersectTest(cameraPos, rayDirection);
        
        if (t == -1.0f)
        {
            return false;
        }
        if (t < smallestT)
        {
            smallestT = t;
        }
        return true;
    }

};

class Plane : public Object {
private:
    vec3 pos;
    vec3 rotation;
    vec3 diffuseColor;
    vec3 specularColor;
    vec3 ambientColor;
    float exponent;
    
public:
    Plane(vec3 _position, vec3 _rotation, vec3 _diffuseColor, vec3 _specularColor, vec3 _ambientColor, float _exponent)
        : pos(_position), rotation(_rotation), diffuseColor(_diffuseColor), specularColor(_specularColor), ambientColor(_ambientColor), exponent(_exponent) {}

    vec3 findHit(float t, vec3 pw, vec3 vw) override
    {
        vec3 hitPoint = pw + t * vw;
        return hitPoint;
        
    }

    vec3 computeRayColor(vec3 cameraPosition, vec3 rayDirection, vector<Light>& lights, vector<Object*>& objects, Object& object, vec3 hitPoint, int depth) override {
        
        vec3 ca = ambientColor;
        vec3 color = ca;
        for(int i = 0; i < lights.size(); i ++){
            Light light = lights[i];
            // Calculate the direction from the intersection point to the lights
            vec3 lightDirection = normalize(light.position - hitPoint);
            vec3 normal = rotation;
            
            
            // Calculate the diffuse and specular components
            float diffuseFactor = std::max(0.0f, dot(normal, lightDirection));
            vec3 cd = diffuseColor * diffuseFactor;
            
            glm::vec3 viewDirection = normalize(-rayDirection);
            
            glm::vec3 halfwayDirection = glm::normalize(lightDirection + viewDirection);
            float specularFactor = std::max(0.0f, pow(dot(halfwayDirection, normal), exponent));
            vec3 cs = specularColor * specularFactor;
            
            
            float epsilon = 0.01f;
            bool inShadow = false;
            for (auto& object : objects) {
                //                          shadow ray is this
                float distToLight = distance(hitPoint, light.position);
                float t = object->intersectTest((hitPoint + lightDirection * epsilon) , lightDirection);
                if(t <= distToLight){
                    inShadow = object->doHit((hitPoint + lightDirection * epsilon) , lightDirection);
                    if(inShadow == true)
                    {
                        
                        break;
                    }
                }
            }
            if(!inShadow)
            {
                color += (light.intensity * ( cd + cs));
            }
            else{
                color += (0.0f * ( cd + cs));
            }
        }
        return color;
    }
    float intersectTest(vec3 cameraPos, vec3 rayDirection) override{
        // Task 2: Find plane intersection
        vec3 c = pos;
        vec3 normal = rotation;
        vec3 pw = cameraPos; // ray position
        vec3 vw = rayDirection; // ray direction
        float t = dot(normal, (c - pw)) / dot(normal, vw);
        if (t < 0.0f)
        {
            return -1.0f;
        }
        return t;
    }
    bool doHit(vec3 cameraPos, vec3 rayDirection) override
    {
        
        float t = intersectTest(cameraPos, rayDirection);
        
        if (t == -1.0f)
        {
            return false;
        }
        if (t < smallestT)
        {
            smallestT = t;
        }
        return true;
    }
};


class Ellipsoid : public Object {
private:
    vec3 center;
    vec3 scale;
    vec3 diffuseColor;
    vec3 specularColor;
    vec3 ambientColor;
    float exponent;
    glm::mat4 E;
    vec3 ellipseHP = vec3(0.0f);
    vec3 ellipseNor = vec3(0.0f);

public:
    Ellipsoid(vec3 _center, vec3 _scale, vec3 _diffuseColor, vec3 _specularColor, vec3 _ambientColor, float _exponent, glm::mat4 _E)
        : center(_center), scale(_scale), diffuseColor(_diffuseColor), specularColor(_specularColor), ambientColor(_ambientColor), exponent(_exponent), E(_E) {}

    vec3 findHit(float t, vec3 pw, vec3 vw) override
    {
        return ellipseHP;
    }
    vec3 computeRayColor(vec3 cameraPosition, vec3 rayDirection, vector<Light>& lights, vector<Object*>& objects, Object& object, vec3 hitPoint, int depth) override {
        vec3 ca = ambientColor;
        vec3 color = ca;
        for(int i = 0; i < lights.size(); i ++){
            Light light = lights[i];
            // Calculate the direction from the intersection point to the lights
            vec3 lightDirection = normalize(light.position - hitPoint);
           
            
            vec3 normal = ellipseNor;
            // Calculate the diffuse and specular components
            float diffuseFactor = std::max(0.0f, dot(normal, lightDirection));
            vec3 cd = diffuseColor * diffuseFactor;
            
            glm::vec3 viewDirection = normalize(-rayDirection);
            
            glm::vec3 halfwayDirection = glm::normalize(lightDirection + viewDirection);
            float specularFactor = std::max(0.0f, pow(dot(halfwayDirection, normal), exponent));
            vec3 cs = specularColor * specularFactor;
           
            
            float epsilon = 0.01f;
            bool inShadow = false;
            for (auto& object : objects) {
                //                          shadow ray is this
                float distToLight = distance(hitPoint, light.position);
                float t = object->intersectTest((hitPoint + lightDirection * epsilon) , lightDirection);
                if(t <= distToLight){
                    inShadow = object->doHit((hitPoint + lightDirection * epsilon) , lightDirection);
                    if(inShadow == true)
                    {
                        
                        break;
                    }
                }
            }
            if(!inShadow)
            {
                color += (light.intensity * ( cd + cs));            }
            else{
                color += (0.0f * ( cd + cs));            }
            
        }
        return color;
    }
    
    float intersectTest(vec3 cameraPos, vec3 rayDirection) override {
        // Task 5: Find ellipsoid intersection(s)
        vec3 pw = cameraPos; // ray position
        vec3 vw = rayDirection; // ray direction
        
        vec4 pLocalSpace = inverse(E) * vec4(pw.x, pw.y, pw.z, 1.0f);
        vec4 vLocalSpace = inverse(E) * vec4(vw, 0.0f);
        vec3 pLS = vec3(pLocalSpace);
        vec3 vLS = vec3(vLocalSpace);
        
        vLS = normalize(vLS);
        
        float a = dot(vLS, vLS);
        float b = 2.0f * dot(vLS, pLS);
        float newC = dot(pLS, pLS) - 1.0f;
        float d = pow(b, 2) - (4 * a * newC);
        
        
        if (d > 0){
            float t1 = (-b + sqrt(d)) / (2.0f * a); // in Local Space
            float t2 = (-b - sqrt(d)) / (2.0f * a); // in Local Space
            
           
            vec3 x1LS = pLS + t1 * vLS;
            vec3 x2LS = pLS + t2 * vLS;
            
            // Finally, we transform the hit position, normal, and distance into world coordinates:
            vec4 worldX1 = E * vec4(x1LS, 1.0f);
            vec4 worldX2 = E * vec4(x2LS, 1.0f);
            
            vec4 worldN1 = transpose(inverse(E)) * vec4(x1LS, 0.0f);
            vec4 worldN2 = transpose(inverse(E)) * vec4(x2LS, 0.0f);
            
            // convert back to vec 3
            vec3 worldNOne = vec3(worldN1);
            vec3 worldNTwo = vec3(worldN2);
            vec3 worldXOne = vec3(worldX1);
            vec3 worldXTwo = vec3(worldX2);
            
            worldNOne = normalize(worldNOne);
            worldNTwo = normalize(worldNTwo);
            
            t1 = length(worldX1 - vec4(pw, 1.0f));
            t2 = length(worldX2 - vec4(pw, 1.0f));
            
            if (dot(vw, vec3(worldX1 - vec4(pw, 1.0f))) < 0){
                t1 = -t1;
            }
            if (dot(vw, vec3(worldX2 - vec4(pw, 1.0f))) < 0){
                t2 = -t2;
            }
            
            if (t1 > 0.0f && t2 > 0.0f){
                if (t2 < t1)
                {
                    ellipseHP = worldXTwo;
                    ellipseNor = worldNTwo;
                }
                else{
                    ellipseHP = worldXOne;
                    ellipseNor = worldNOne;

                }
                return std::min(t1, t2);
            }
            if (t1 < 0.0f && t2 > 0.0f)
            {
                ellipseHP = worldXTwo;
                ellipseNor = worldNTwo;

                return t2;
            }
            if (t1 > 0.0f && t2 < 0.0f)
            {
                ellipseHP = worldXOne;
                ellipseNor = worldNOne;

                return t1;
            }
        }
        return -1.0f;
            
    }
    bool doHit(vec3 cameraPos, vec3 rayDirection) override
    {
        
        float t = intersectTest(cameraPos, rayDirection);
        
        if (t == -1.0f)
        {
            return false;
        }
        if (t < smallestT)
        {
            smallestT = t;
        }
        return true;
    }

};

class ReflectiveSphere : public Object {
private:
    vec3 center;
    float radius;
    
   


public:
    ReflectiveSphere(vec3 _center, float _radius)
        : center(_center), radius(_radius) {}

    vec3 findHit(float t, vec3 pw, vec3 vw) override
    {
        vec3 hitPoint = pw + t * vw;
        return hitPoint;
        
    }
    vec3 computeRayColor(vec3 cameraPosition, vec3 rayDirection, vector<Light>& lights, vector<Object*>& objects, Object& object, vec3 hitPoint, int depth) override {
        // Compute the color of the object at the hit point
        vec3 finalColor = vec3(0.0f);
        
        // Compute the reflection ray
        vec3 normal = (hitPoint - center) / radius;
        
        // find reflected ray
        vec3 reflectedRayDirection = glm::reflect(rayDirection, normal);
        
        // Find closest object hit by the reflected ray
        Object* closestObject = nullptr;
        vec3 reflectionHitPoint;
        bool hitObject = false;
        float smallestFoundT = std::numeric_limits<int>::max();
        for (Object* obj : objects) {
            float epsilon = 0.01f;
            // inShadow = object->doHit((hitPoint + lightDirection * epsilon) , lightDirection);
            
            hitObject = obj->doHit(hitPoint + reflectedRayDirection * epsilon , reflectedRayDirection);
            float t = obj->intersectTest(hitPoint + reflectedRayDirection * epsilon, reflectedRayDirection);
            
            if (hitObject == true && t < smallestFoundT)
            {
                closestObject = obj;
                vec3 newHit = obj->findHit(t, hitPoint + reflectedRayDirection * epsilon , reflectedRayDirection);
                reflectionHitPoint = newHit;
                smallestFoundT = t;
                
            }
        }
        if (closestObject and depth > 0) {
            // add parameter to crc to stop depth
            depth -= 1;
            vec3 reflectedColor = closestObject->computeRayColor(cameraPosition, reflectedRayDirection, lights, objects, *closestObject, reflectionHitPoint , depth);
            finalColor =  reflectedColor;
        }
        
        
        return finalColor;
    }
    
    float intersectTest(vec3 cameraPos, vec3 rayDirection) override {
        vec3 pw = cameraPos; // ray position
        vec3 vw = rayDirection; // ray direction
        vec3 newPC = vec3(pw) - center;
        float a = dot(vw, vw);
        float b = 2.0f * dot(vw, newPC);
        float newC = dot(newPC, newPC) - pow(radius, 2.0f);
        float d = pow(b, 2) - (4 * a * newC);
        
        if (d > 0){
            float t1 = (-b + sqrt(d)) / (2.0f * a);
            float t2 = (-b - sqrt(d)) / (2.0f * a);
            
            if (t1 > 0.0f && t2 > 0.0f){
                return std::min(t1, t2);
            }
            if (t1 < 0.0f && t2 > 0.0f)
            {
                return t2;
            }
            if (t1 > 0.0f && t2 < 0.0f)
            {
                return t1;
            }
        }
        return -1.0f;
    }
    bool doHit(vec3 cameraPos, vec3 rayDirection) override
    {
        
        float t = intersectTest(cameraPos, rayDirection);
        
        if (t == -1.0f)
        {
            return false;
        }
        if (t < smallestT)
        {
            smallestT = t;
        }
        return true;
    }

};

// NEED TO TAKE INTO ACCOUNT FOV AT SOME POINT
std::vector<glm::vec3> generateRays(int imageSize) {
    // Calculate the width and height of the image
    int width = imageSize;
    int height = imageSize;

    // Calculate the step size for each pixel
    float stepSize = 1.0f / imageSize;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate the coordinates of the center of the pixel
            
            float centerX = x * stepSize + stepSize / 2;
            float centerY = y * stepSize + stepSize / 2;

            // Calculate the direction vector from camera position to pixel center
            glm::vec3 rayDirection = {
                centerX - 0.5f, // Center of pixel is at (0.5, 0.5)
                centerY - 0.5f,
                -1 // Direction along -Z axis
            };
            
            // Normalize the direction vector
            rayDirection = glm::normalize(rayDirection);
            rays.push_back(rayDirection);
        }
    }
    return rays;
}

// Function to calculate the distance between two points
float distance(vec3& v1, vec3& v2) {
    return std::sqrt((v1.x - v2.x) * (v1.x - v2.x) +
                     (v1.y - v2.y) * (v1.y - v2.y) +
                     (v1.z - v2.z) * (v1.z - v2.z));
}

// Function to create a bounding sphere
void createBoundingSphere(std::vector<float>& posBuf, vec3& center, float& radius) {
    // Find the center
    float sumX = 0;
    float sumY = 0;
    float sumZ = 0;
    for (int i = 0; i < posBuf.size(); i += 3) {
        sumX += posBuf[i];
        sumY += posBuf[i + 1];
        sumZ += posBuf[i + 2];
    }
    center.x = sumX / (posBuf.size() / 3);
    center.y = sumY / (posBuf.size() / 3);
    center.z = sumZ / (posBuf.size() / 3);

    // Find the radius
    radius = 0;
    for (int i = 0; i < posBuf.size(); i += 3)  {
        vec3 vertex = vec3(posBuf[i], posBuf[i + 1], posBuf[i + 2]);
        float dist = distance(vertex, center);
        if (dist > radius)
            radius = dist;
    }
}
/* code rewritten to do tests on the sign of the determinant */
/* the division is at the end in the code                    */
int intersect_triangle1(double orig[3], double dir[3],
            double vert0[3], double vert1[3], double vert2[3],
            double *t, double *u, double *v)
{
   double edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
   double det,inv_det;

   /* find vectors for two edges sharing vert0 */
   SUB(edge1, vert1, vert0);
   SUB(edge2, vert2, vert0);

   /* begin calculating determinant - also used to calculate U parameter */
   CROSS(pvec, dir, edge2);

   /* if determinant is near zero, ray lies in plane of triangle */
   det = DOT(edge1, pvec);

   if (det > EPSILON)
   {
      /* calculate distance from vert0 to ray origin */
      SUB(tvec, orig, vert0);

      /* calculate U parameter and test bounds */
      *u = DOT(tvec, pvec);
      if (*u < 0.0 || *u > det)
     return 0;

      /* prepare to test V parameter */
      CROSS(qvec, tvec, edge1);

      /* calculate V parameter and test bounds */
      *v = DOT(dir, qvec);
      if (*v < 0.0 || *u + *v > det)
     return 0;

   }
   else if(det < -EPSILON)
   {
      /* calculate distance from vert0 to ray origin */
      SUB(tvec, orig, vert0);

      /* calculate U parameter and test bounds */
      *u = DOT(tvec, pvec);
/*      printf("*u=%f\n",(float)*u); */
/*      printf("det=%f\n",det); */
      if (*u > 0.0 || *u < det)
     return 0;

      /* prepare to test V parameter */
      CROSS(qvec, tvec, edge1);

      /* calculate V parameter and test bounds */
      *v = DOT(dir, qvec) ;
      if (*v > 0.0 || *u + *v < det)
     return 0;
   }
   else return 0;  /* ray is parallell to the plane of the triangle */


   inv_det = 1.0 / det;

   /* calculate t, ray intersects triangle */
   *t = DOT(edge2, qvec) * inv_det;
   (*u) *= inv_det;
   (*v) *= inv_det;

   return 1;
}

std::vector<glm::vec3> newRays;
std::vector<glm::vec3> newGenerateRays(int imageSize, vec3 cameraPostion) {
    // Calculate the width and height of the image
    int width = imageSize;
    int height = imageSize;

    // Calculate the step size for each pixel
    float stepSize = 1.0f / imageSize;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate the coordinates of the center of the pixel
            
            float centerX = x * stepSize + stepSize / 2;
            float centerY = y * stepSize + stepSize / 2;

            // Calculate the direction vector from camera position to pixel center
            glm::vec3 rayDirection = {
                centerX - 0.5f, // Center of pixel is at (0.5, 0.5)
                centerY - 0.5f,
                -1 // Direction along -Z axis
            };
            rayDirection += cameraPostion;
            // Normalize the direction vector
            rayDirection = glm::normalize(rayDirection);
            rays.push_back(rayDirection);
        }
    }
    return rays;
}



int main(int argc, char **argv)
{
    if(argc < 4) {
        cout << "A6 not enough arguments" << endl;
        return 0;
    }
    int scene = atoi(argv[2]);
    int imageSize = atoi(argv[3]);
    string output_filename(argv[4]);
    
    auto image = make_shared<Image>(imageSize, imageSize);
    
    // Task 1
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    float fov = 45.0f;
    
    ManualCamera camera(position, fov);
    std::vector<glm::vec3> rays = generateRays(imageSize);
        
    // Task 2
    if (scene <= 2) {
        vector<vector<float>> distBuf(imageSize, vector<float>(imageSize, static_cast<float>(numeric_limits<int>::max())));
        // Define the light
        Light light = {{-2.0, 1.0, 1.0}, 1.0};
        vector<Light> lights;
        lights.push_back(light);
        // Define the spheres
        Sphere redSphere(vec3(-0.5f, -1.0f, 1.0f), 1.0f, vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        Sphere greenSphere(vec3(0.5f, -1.0f, -1.0f), 1.0f, vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        Sphere blueSphere(vec3(0.0f, 1.0f, 0.0f), 1.0f, vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        
        vector<Object*> objects;
        objects.push_back(&redSphere);
        objects.push_back(&greenSphere);
        objects.push_back(&blueSphere);

        for (int i = 0; i < imageSize; i++) {
            for (int j = 0; j < imageSize; j++) {
                vec3 colors = {0.0f, 0.0f, 0.0f};
                for (Object* obj : objects)
                {
                    bool hasHit = obj->doHit(camera.getPosition(), rays[i * imageSize + j]);
                    float t = obj->intersectTest(camera.getPosition(), rays[i * imageSize + j]);
                    if (hasHit && t < distBuf[i][j]) {
                        distBuf[i][j] = t;
                        
                        
                        vec3 hitPoint = obj->findHit(t, camera.getPosition(), rays[i * imageSize + j]);
                        
                        colors = obj->computeRayColor(camera.getPosition(), rays[i * imageSize + j], lights, objects, *obj, hitPoint, 30);
                        
                    }
                }
                
                float r = std::min(colors.r , 1.0f);
                float g = std::min(colors.g , 1.0f);
                float b = std::min(colors.b , 1.0f);
                
                image->setPixel(j, i, r * 255, g * 255, b * 255);
            }
        }
    }
    
    // Task 3
    if (scene == 3) {
        Light lightOne = {{1.0f, 2.0f, 2.0f}, 0.5f};
        Light lightTwo = {{-1.0f, 2.0f, -1.0f}, 0.5f};
        vector<Light> lights;
        lights.push_back(lightOne);
        lights.push_back(lightTwo);
        
        Plane infPlane(vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f);
        Sphere greenSphere(vec3(-0.5f, 0.0f, -0.5f), 1.0f, vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        
        
        auto M = make_shared<MatrixStack>();
        M->translate(0.5f, 0.0f, 0.5f);
        M->scale(0.5f, 0.6f, 0.2f);
        glm::mat4 E = M->topMatrix();
        Ellipsoid ellipse(vec3(0.5f, 0.0f, 0.5f), vec3(0.5f, 0.6f, 0.2f), vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f, E);
        vector<Object*> objects;
        objects.push_back(&ellipse);
        objects.push_back(&greenSphere);
        objects.push_back(&infPlane);
       
        
        
        vector<vector<float>> distBuf(imageSize, vector<float>(imageSize, static_cast<float>(numeric_limits<int>::max())));
        for (int i = 0; i < imageSize; i++) {
            for (int j = 0; j < imageSize; j++) {
                vec3 colors = {0.0f, 0.0f, 0.0f};
                for (Object* obj : objects)
                {
                    bool hasHit = obj->doHit(camera.getPosition(), rays[i * imageSize + j]);
                    float t = obj->intersectTest(camera.getPosition(), rays[i * imageSize + j]);
                    
                    if (hasHit && t < distBuf[i][j]) {
                        distBuf[i][j] = t;
                        
                        vec3 hitPoint = obj->findHit(t, camera.getPosition(), rays[i * imageSize + j]);
                        colors = obj->computeRayColor(camera.getPosition(), rays[i * imageSize + j], lights, objects, *obj, hitPoint, 5);
                    }
                }
                
                float r = std::min(colors.r , 1.0f);
                float g = std::min(colors.g , 1.0f);
                float b = std::min(colors.b , 1.0f);
                
                image->setPixel(j, i, r * 255, g * 255, b * 255);
            }
        }
    }
    // Task 4
    if (scene == 4 || scene == 5) {
        Light lightOne = {{-1.0, 2.0, 1.0}, 0.5f};
        Light lightTwo = {{0.5, -0.5, 0.0}, 0.5f};
        vector<Light> lights;
        lights.push_back(lightOne);
        lights.push_back(lightTwo);
        
        Plane backWall(vec3(0.0, 0.0, -3.0), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f);
        Plane floor(vec3(0.0, -1.0, 0.0), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.1f, 0.1f, 0.1f), 0.0f);
        
        Sphere redSphere(vec3(0.5, -0.7, 0.5), 0.3f, vec3(1.0, 0.0, 0.0), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        Sphere blueSphere(vec3(1.0, -0.7, 0.0), 0.3f, vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        
        ReflectiveSphere reflectiveSphere1(vec3(-0.5, 0.0, -0.5), 1.0f);
        ReflectiveSphere reflectiveSphere2(vec3(1.5, 0.0, -1.5), 1.0f);
        
        
       
        vector<Object*> objects;
        objects.push_back(&backWall);
        objects.push_back(&floor);
        objects.push_back(&blueSphere);
        objects.push_back(&redSphere);
        objects.push_back(&reflectiveSphere1);
        objects.push_back(&reflectiveSphere2);
        
        
        vector<vector<float>> distBuf(imageSize, vector<float>(imageSize, static_cast<float>(numeric_limits<int>::max())));
        for (int i = 0; i < imageSize; i++) {
            for (int j = 0; j < imageSize; j++) {
                vec3 colors = {0.0f, 0.0f, 0.0f};
                for (Object* obj : objects)
                {
                    bool hasHit = obj->doHit(camera.getPosition(), rays[i * imageSize + j]);
                    float t = obj->intersectTest(camera.getPosition(), rays[i * imageSize + j]);
                    
                    if (hasHit && t < distBuf[i][j]) {
                        distBuf[i][j] = t;
                        
                        vec3 hitPoint = obj->findHit(t, camera.getPosition(), rays[i * imageSize + j]);
                        colors = obj->computeRayColor(camera.getPosition(), rays[i * imageSize + j], lights, objects, *obj, hitPoint, 3);
                    }
                }
                
                float r = std::min(colors.r , 1.0f);
                float g = std::min(colors.g , 1.0f);
                float b = std::min(colors.b , 1.0f);
                
                image->setPixel(j, i, r * 255, g * 255, b * 255);
            }
        }
        
    }
    // TASK 5 unfinished
    if (scene == 6 or scene == 7)
    {
        Light lightOne = {{-1.0, 1.0, 1.0}, 1.0f};
        Material objMaterial = {glm::vec3(0.0, 0.0, 1.0), glm::vec3(1.0, 1.0, 0.5), glm::vec3(0.1, 0.1, 0.1), 100.0f}; // Blue with green highlights
        
        shape = make_shared<Shape>();
        shape->loadMesh(RESOURCE_DIR + "bunny.obj");
        std::vector<float> posBuf = shape->getPosBuf();
        std::vector<float> norBuf = shape->getNorBuf();
        // every 9 points is a triangle.
        // every 3 of those 9 points is a vertex
        
        vec3 center;
        float radius;
        createBoundingSphere(posBuf, center, radius);
        Sphere boundingSphere(center, radius, vec3(0.0, 0.0, 0.0), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),0.0f);
            
        for (int i = 0; i < imageSize; i++) {
            for (int j = 0; j < imageSize; j++) {
                bool hitSphere = false;
                
                hitSphere = boundingSphere.intersectTest(camera.getPosition(), rays[i * imageSize + j]);
                if (hitSphere)
                {
                    double orig[3];
                    orig[0] = camera.getPosition().x;
                    orig[1] = camera.getPosition().y;
                    orig[2] = camera.getPosition().z;
                    
                    double dir[3];
                    dir[0] = rays[i * imageSize + j].x;
                    dir[1] = rays[i * imageSize + j].y;
                    dir[2] = rays[i * imageSize + j].z;
                    for (int i = 8; i < posBuf.size(); i+=9)
                    {
                        
                        
                        double vert0[3];
                        double vert1[3];
                        double vert2[3];
                        vert0[0] = posBuf[i-8];
                        vert0[1] = posBuf[i-7];
                        vert0[2] = posBuf[i-6];
                        
                        vert1[0] = posBuf[i-5];
                        vert1[1] = posBuf[i-4];
                        vert1[2] = posBuf[i-3];
                        
                        vert2[0] = posBuf[i-2];
                        vert2[1] = posBuf[i-1];
                        vert2[2] = posBuf[i];
                        
                        
                        double u;
                        double v;
                        double t;
                        int res;
                        res = intersect_triangle1(&orig[3], &dir[3], &vert0[3], &vert1[3], &vert2[3], &t, &u,  &v);
                        
                        //float r = std::min(colors.r , 1.0f);
                        //float g = std::min(colors.g , 1.0f);
                        //float b = std::min(colors.b , 1.0f);
                    
                        //image->setPixel(j, i, r * 255, g * 255, b * 255);
                    }
                }
                
                
            }
        }
        
        
    }
    // TASK 6
    if (scene == 8)
    {
       
        //camera.changeFOV(60.0f);
        camera.changePosition(vec3(-3,0,0));
       
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // Look at the positve x axis
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // Y-axis is up

        glm::mat4 view = glm::lookAt(camera.getPosition(), cameraTarget, up);
        glm::mat4 transformMatrix = view;

        vec3 rSpherePos = vec3(transformMatrix *  vec4(vec3(-0.5f, -1.0f, 1.0f), 1.0f));
        vec3 gSpherePos = vec3(transformMatrix *  vec4(vec3(0.5f, -1.0f, -1.0f), 1.0f));
        vec3 bSpherePos = vec3(transformMatrix *  vec4(vec3(0.0f, 1.0f, 0.0f), 1.0f));
        
        
        // Define the light
        Light light = {{-2.0, 1.0, 1.0}, 1.0};
        vector<Light> lights;
        lights.push_back(light);
        lights[0].position =vec3(transformMatrix *  vec4(lights[0].position, 1.0f));
        
        camera.changePosition(vec3(0,0,0));
        // Define the spheres
        Sphere redSphere(rSpherePos, 1.0f, vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        Sphere greenSphere(gSpherePos, 1.0f, vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        Sphere blueSphere(bSpherePos, 1.0f, vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 1.0f, 0.5f), vec3(0.1f, 0.1f, 0.1f), 100.0f);
        
        vector<Object*> objects;
        objects.push_back(&redSphere);
        objects.push_back(&greenSphere);
        objects.push_back(&blueSphere);
        
        vector<vector<float>> distBuf(imageSize, vector<float>(imageSize, static_cast<float>(numeric_limits<int>::max())));
        for (int i = 0; i < imageSize; i++) {
            for (int j = 0; j < imageSize; j++) {
                vec3 colors = {0.0f, 0.0f, 0.0f};
                for (Object* obj : objects)
                {
                    bool hasHit = obj->doHit(camera.getPosition(), rays[i * imageSize + j]);
                    float t = obj->intersectTest(camera.getPosition(), rays[i * imageSize + j]);
                    if (hasHit && t < distBuf[i][j]) {
                        distBuf[i][j] = t;
                        
                        
                        vec3 hitPoint = obj->findHit(t, camera.getPosition(), rays[i * imageSize + j]);
                        
                        colors = obj->computeRayColor(camera.getPosition(), rays[i * imageSize + j], lights, objects, *obj, hitPoint, 30);
                        
                    }
                }
                
                float r = std::min(colors.r , 1.0f);
                float g = std::min(colors.g , 1.0f);
                float b = std::min(colors.b , 1.0f);
                
                image->setPixel(j, i, r * 255, g * 255, b * 255);
            }
        }
    }
    //write image to file
    image->writeToFile(output_filename);
    return 0;
     
}
