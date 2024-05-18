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
#include "Texture.h"

using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Texture> texture0;
map<string,GLuint> bufIDs;
int indCount;

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

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	//
	// General setup
	//
	
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	// prog->addAttribute("aNor");
	prog->addAttribute("aTex");
	prog->addUniform("MV");
	prog->addUniform("P");
    prog->addUniform("t");
    prog->addUniform("invTransposeMV");
	prog->addUniform("texture0");
	prog->setVerbose(false);
	
	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f);
	
	texture0 = make_shared<Texture>();
	texture0->setFilename(RESOURCE_DIR + "tamu.jpg");
	texture0->init();
	texture0->setUnit(0);
	texture0->setWrapModes(GL_REPEAT, GL_REPEAT);
	
	//
	// Vertex buffer setup
	//
	
	vector<float> posBuf;
	vector<float> norBuf;
	vector<float> texBuf;
	vector<unsigned int> indBuf;

	//
	// IMPLEMENT ME
	//
	// Instead of the hard coded square below, you need to draw a sphere.
	// You need to use one or more for-loops to fill in the position buffer,
	// normal buffer, texture buffer, and the index buffer.
	//
	
    int numXPoints = 50;
    float maxX = 10.0f; // Maximum value of x
    float maxTheta = 2.0f * M_PI; // Maximum value of theta

    int numThetaPoints = 50;
    int numPoints = 50;
    for (int i = 0; i < numXPoints; i++) {
        float x = maxX * static_cast<float>(i) / (numXPoints - 1);
        for (int j = 0; j < numThetaPoints; j++) {
            
            float theta = maxTheta * static_cast<float>(j) / (numThetaPoints - 1);
            
            float fx = cos(x) + 2;
            float y = fx * cos(theta);
            // float z = fx * sin(theta);
            
            // Position (send x and theta)
            posBuf.push_back(x);
            posBuf.push_back(theta);
            posBuf.push_back(0.0f);
            
            /*
            float dfdx = -sin(x);
            
            float dp_dx_x = 1.0f;
            float dp_dx_y = dfdx * cos(theta);
            float dp_dx_z = dfdx * sin(theta);
            
            float dfdtheta = -sin(x);
            float dp_dtheta_x = 0.0f;
            float dp_dtheta_y = -(dfdtheta * sin(theta));
            float dp_dtheta_z = dfdtheta * cos(theta);
            
            // Compute the normal vector
            float nx = dp_dx_y * dp_dtheta_z - dp_dx_z * dp_dtheta_y;
            float ny = dp_dx_z * dp_dtheta_x - dp_dx_x * dp_dtheta_z;
            float nz = dp_dx_x * dp_dtheta_y - dp_dx_y * dp_dtheta_x;
            // Normalize the normal vector
            float length = sqrt(nx * nx + ny * ny + nz * nz);
            nx /= length;
            ny /= length;
            nz /= length;
            
            */
            // Normal (send in zeros)
            norBuf.push_back(0.0f);
            norBuf.push_back(0.0f);
            norBuf.push_back(0.0f);
            
            // Compute the texture coordinates
            
            texBuf.push_back(x + 0.5f);
            texBuf.push_back(y + 0.5f);
        }
    }
    for (int i = 0; i < numPoints - 1; ++i) {
        for (int j = 0; j < numPoints - 1; ++j) {
            int index = i * numPoints + j;
            indBuf.push_back(index);
            indBuf.push_back(index + 1);
            indBuf.push_back(index + 1 + numPoints);

            indBuf.push_back(index);
            indBuf.push_back(index + numPoints + 1);
            indBuf.push_back(index + numPoints);
        }
    }
	// Total number of indices
	indCount = (int)indBuf.size();
		
	// Generate buffer IDs and put them in the bufIDs map.
	GLuint tmp[4];
	glGenBuffers(4, tmp);
	bufIDs["bPos"] = tmp[0];
	bufIDs["bNor"] = tmp[1];
	bufIDs["bTex"] = tmp[2];
	bufIDs["bInd"] = tmp[3];
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bNor"]);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bTex"]);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size()*sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	assert(norBuf.size() == posBuf.size());
	
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
    double t = glfwGetTime();
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
    
	prog->bind();
    
    MV->pushMatrix();
    //float y = 1.3f * ( (1/2.0f) * sin( (2*M_PI/1.7f) * (t + 0.9f)) + (1/2.0f));
    //float s = -0.5f * ( (1/2.0f) * cos( (4*M_PI/1.7f) * (t + 0.9f)) + (1/2.0f)) + 1.0f;
    
    MV->translate(0.0f, -0.5f, 0.0f);
    
    MV->scale(glm::vec3( 0.1f, 0.1f,  0.1f ));
    float angle = 90.0f * M_PI/180.0f;
    MV->rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
    
    
    glUniform1f(prog->getUniform("t"), t);
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
    glUniformMatrix4fv(prog->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
	glEnableVertexAttribArray(prog->getAttribute("aPos"));
	GLSL::checkError(GET_FILE_LINE);
	// glEnableVertexAttribArray(prog->getAttribute("aNor"));
	GLSL::checkError(GET_FILE_LINE);
	glEnableVertexAttribArray(prog->getAttribute("aTex"));
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
	glVertexAttribPointer(prog->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bNor"]);
	// glVertexAttribPointer(prog->getAttribute("aNor"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bTex"]);
	glVertexAttribPointer(prog->getAttribute("aTex"), 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glDrawElements(GL_TRIANGLES, indCount, GL_UNSIGNED_INT, (void *)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(prog->getAttribute("aTex"));
	// glDisableVertexAttribArray(prog->getAttribute("aNor"));
	glDisableVertexAttribArray(prog->getAttribute("aPos"));
    MV->popMatrix();
	prog->unbind();
	
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
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
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
