#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <map>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"

using namespace std;
using namespace glm;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from

shared_ptr<Camera> camera;
shared_ptr<Program> progPass1;
shared_ptr<Program> progPass2;
shared_ptr<Shape> shape1;
shared_ptr<Shape> shape2;

int textureWidth = 640;
int textureHeight = 480;
GLuint framebufferID;
GLuint textureA;
GLuint textureB;

bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// See how many draw buffers are supported
	int count;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &count);
	cout << "GL_MAX_DRAW_BUFFERS: " << count << endl;
	
	// Initialize time
	glfwSetTime(0.0);
	
	// Initial key toggles
	keyToggles[(unsigned)'l'] = true;
	keyToggles[(unsigned)'s'] = true;
	
	camera = make_shared<Camera>();
	camera->setInitDistance(5.0f);

	progPass1 = make_shared<Program>();
	progPass1->setShaderNames(RESOURCE_DIR + "pass1_vert.glsl", RESOURCE_DIR + "pass1_frag.glsl");
	progPass1->setVerbose(false);
	progPass1->init();
	progPass1->addAttribute("aPos");
	progPass1->addAttribute("aNor");
	progPass1->addUniform("P");
	progPass1->addUniform("MV");
    progPass1->addUniform("MVit");
    

	progPass2 = make_shared<Program>();
	progPass2->setShaderNames(RESOURCE_DIR + "pass2_vert.glsl", RESOURCE_DIR + "pass2_frag.glsl");
	progPass2->setVerbose(false);
	progPass2->init();
	progPass2->addAttribute("aPos");
	progPass2->addAttribute("aTex");
	progPass2->addUniform("P");
	progPass2->addUniform("MV");
    progPass2->addUniform("textureA");
    progPass2->addUniform("textureB");
    progPass2->addUniform("t");

    progPass2->bind();
    glUniform1i(progPass2->getUniform("textureA"), 0);
    glUniform1i(progPass2->getUniform("textureB"), 1);
    progPass2->unbind();


	// IMPLEMENT ME
	
	shape1 = make_shared<Shape>();
	shape1->loadMesh(RESOURCE_DIR + "teapot.obj");
	shape1->fitToUnitBox();
	shape1->init();
	
	shape2 = make_shared<Shape>();
	shape2->loadMesh(RESOURCE_DIR + "cube.obj");
	shape2->init();

	// IMPLEMENT ME
    glGenFramebuffers(1, &framebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    
    // set up textureA
    glGenTextures(1, &textureA);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, textureHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureA, 0);
    
    // set up textureB
    glGenTextures(1, &textureB);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, textureHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textureB, 0);
    
    // set up depth tests to happen in pass 1
    GLuint depthrenderbuffer;
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, textureWidth, textureHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    // We now tell OpenGL that we want two textures as the output of this framebuffer.
    GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "Framebuffer is not ok" << endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	double t = glfwGetTime();
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	//////////////////////////////////////////////////////
	// Render to the framebuffer
	//////////////////////////////////////////////////////
	// IMPLEMENT ME
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    glViewport(0, 0, textureWidth, textureHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    progPass1->bind();
    camera->setAspect(1.0f);
    P->pushMatrix();
    camera->applyProjectionMatrix(P);
    MV->pushMatrix();
    MV->translate(0.0, 0.0, -1.0);
    float angle = 180.0f * M_PI/180.0f ;
    MV->rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
    glUniformMatrix4fv(progPass1->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
    glUniformMatrix4fv(progPass1->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
    glUniformMatrix4fv(progPass1->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
    
    shape1->draw(progPass1);
    progPass1->unbind();
    MV->popMatrix();
    P->popMatrix();

	//////////////////////////////////////////////////////
	// Render to the screen
	//////////////////////////////////////////////////////
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	progPass2->bind();
	camera->setAspect((float)width/(float)height);
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
	glUniformMatrix4fv(progPass2->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progPass2->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
    glUniform1f(progPass2->getUniform("t"), t);

	// IMPLEMENT ME
    /*
	shape2->draw(progPass2);
	glActiveTexture(GL_TEXTURE0);
    */
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureB);
    shape2->draw(progPass2);
    glActiveTexture(GL_TEXTURE0);

	progPass2->unbind();
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
