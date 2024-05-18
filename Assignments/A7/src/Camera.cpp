#define _USE_MATH_DEFINES
#include <cmath> 
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "MatrixStack.h"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

Camera::Camera() :
	aspect(1.0f),
	fovy((float)(100.0*M_PI/180.0)),
	znear(0.1f),
	zfar(1000.0f),
	rotations(0.0, 0.0),
	translations(0.0f, 0.0f, -5.0f),
	rfactor(0.01f),
	tfactor(0.001f),
	sfactor(0.005f),
    position(glm::vec3(0.0f, 0.0f, -0.5f)),
    yaw(0.0f),
    pitch(0.0f)
{
}

Camera::~Camera()
{
}

void Camera::mouseClicked(float x, float y, bool shift, bool ctrl, bool alt)
{
	mousePrev.x = x;
	mousePrev.y = y;
	if(shift) {
		state = Camera::TRANSLATE;
	} else if(ctrl) {
		state = Camera::SCALE;
	} else {
		state = Camera::ROTATE;
	}
}

void Camera::processKeyboardInput(GLFWwindow* window)
{
    float movementSpeed = 0.1f;
    glm::vec3 cameraFront = glm::vec3(sin(glm::radians(yaw)), 0.0f, cos(glm::radians(yaw)));
    //glm::vec3 forward = position + cameraFront;
    
    glm::vec3 r = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position += movementSpeed * cameraFront;
        
    }
    if (glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS) {
        position -= movementSpeed * cameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position += movementSpeed * r;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position -= movementSpeed * r;
    }
    
}

void Camera::mouseMoved(float x, float y)
{
	glm::vec2 mouseCurr(x, y);
	glm::vec2 dv = mouseCurr - mousePrev;
    float mouseSensitivity = 0.1f;
	switch(state) {
		case Camera::ROTATE:
			rotations += rfactor * dv;
			break;
		case Camera::TRANSLATE:
			translations.x -= translations.z * tfactor * dv.x;
			translations.y += translations.z * tfactor * dv.y;
			break;
		case Camera::SCALE:
			translations.z *= (1.0f - sfactor * dv.y);
			break;
	}
    dv.x *= mouseSensitivity;
    dv.y *= mouseSensitivity;

    yaw += dv.x;
    pitch += dv.y;

    // Constrain pitch
    if (pitch > 60.0f) {
        pitch = 60.0f;
    }
    if (pitch < -60.0f) {
        pitch = -60.0f;
    }
	mousePrev = mouseCurr;
}

void Camera::applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const
{
	// Modify provided MatrixStack
	P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
}

void Camera::applyViewMatrix(std::shared_ptr<MatrixStack> MV) const
{
	/*MV->translate(translations);
	MV->rotate(rotations.y, glm::vec3(1.0f, 0.0f, 0.0f));
	MV->rotate(rotations.x, glm::vec3(0.0f, 1.0f, 0.0f));*/
    
    glm::vec3 cameraFront = glm::vec3(sin(glm::radians(yaw)), sin(glm::radians(pitch)), cos(glm::radians(yaw)));
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 forward = position + cameraFront;
    
    MV->multMatrix(glm::lookAt(position, forward, up));

}
