#pragma  once
#ifndef CAMERA_H
#define CAMERA_H

#include <memory>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class MatrixStack;

class Camera
{
public:
	enum {
		ROTATE = 0,
		TRANSLATE,
		SCALE
	};
	
	Camera();
	virtual ~Camera();
	void setInitDistance(float z) { translations.z = -std::abs(z); }
	void setAspect(float a) { aspect = a; };
	void setRotationFactor(float f) { rfactor = f; };
	void setTranslationFactor(float f) { tfactor = f; };
	void setScaleFactor(float f) { sfactor = f; };
	void mouseClicked(float x, float y, bool shift, bool ctrl, bool alt);
	void mouseMoved(float x, float y);
	void applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const;
	void applyViewMatrix(std::shared_ptr<MatrixStack> MV) const;
    void processKeyboardInput(GLFWwindow* window);
    void setFOV(float a) 
    {
        if (fovy < float(4 * M_PI/180.0)) {
            fovy = float(4 * M_PI/180.0);
        } else if (fovy > float(114 * M_PI/180.0))  {
            fovy = float(114 * M_PI/180.0);
        }
        else{
            fovy += float(a*M_PI/180.0);
        }
        
    };
    glm::vec3 getPosition(){
        return position;
    };
    glm::mat4 inverseLookAt(){
        glm::vec3 cameraFront = glm::vec3(sin(glm::radians(yaw)), sin(glm::radians(pitch)), cos(glm::radians(yaw)));
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 forward = position + cameraFront;
        
        return glm::inverse((glm::lookAt(position, forward, up)));
    };
    float getFOVy(){
        return fovy;
    };
	
private:
	float aspect;
	float fovy;
	float znear;
	float zfar;
	glm::vec2 rotations;
	glm::vec3 translations;
	glm::vec2 mousePrev;
	int state;
	float rfactor;
	float tfactor;
	float sfactor;
    
    glm::vec3 position;
    float yaw;
    float pitch;
    

};

#endif
