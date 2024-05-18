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
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"

using namespace std;

class Material {
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

class Light {
public:
    glm::vec3 position;
    glm::vec3 color;
    
    void move(float dx, float dy) {
        position.x += dx;
        position.y += dy;
    }
};

// me
vector<Material> materials;
vector<Light> lights;
vector<bool> whichShader;

int materialsIndex = 0;
int lightsIndex = 0;
int shaderIndex = 0;

bool swapShaders = false;
bool isSilhouette = false;
bool isBlinnPhong = false;
bool isNormalShader = true;
bool isCelShader = false;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Program> prog2; // <-- Blinn - Phong
shared_ptr<Program> prog3; // <-- Silhoette Shader
shared_ptr<Program> prog4; // <-- Cell Shader
shared_ptr<Shape> shape;
shared_ptr<Shape> teapot;

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
    switch(key) {
        case 's': // cycle shaders forward
        {
            whichShader[shaderIndex] = false;
            if(shaderIndex == whichShader.size() - 1)
            {
                shaderIndex = 0;
            }
            else
            {
                shaderIndex += 1;
            }
            
            whichShader[shaderIndex] = true;
            
            break;
        }
        case 'S': // cycle shaders backwards
        {
            whichShader[shaderIndex] = false;
            if(shaderIndex == 0)
            {
                shaderIndex = whichShader.size() - 1;
            }
            else
            {
                shaderIndex -= 1;
            }
            
            whichShader[shaderIndex] = true;
            break;
        }
        case 'm': // cycle materials forward
        {
            
            if(materialsIndex == materials.size() - 1)
            {
                materialsIndex = 0;
                
            }
            else{
                materialsIndex += 1;
            }
            
            break;
        }
        case 'M': // cycle materials backwards
        {
            if(materialsIndex == 0)
            {
                materialsIndex = materials.size() - 1;
                
            }
            else{
                materialsIndex -= 1;
            }
            break;
        }
        case 'l': // swap light
        {
            
            if(whichShader[1] or whichShader[3])
            {
                if(lightsIndex == lights.size() - 1)
                {
                    lightsIndex = 0;
                    
                }
                else
                {
                    lightsIndex += 1;
                }
            }
            
            break;
        }
        case 'L': // swap light
        {
            if(whichShader[1] or whichShader[3])
            {
                if(lightsIndex == lights.size() - 1)
                {
                    lightsIndex = 0;
                    
                }
                else
                {
                    lightsIndex += 1;
                }
            }
            
            
            break;
        }
        case 'x': // move light -x direction
        {
            if(whichShader[1] or whichShader[3])
            {
                lights[lightsIndex].position.x -= 0.4;
            }
            
            break;
        }
        case 'X': // move light +x direction
        {
            if(whichShader[1] or whichShader[3])
            {
                lights[lightsIndex].position.x += 0.4;
            }
            
            break;
        }
        case 'y': // move light -y direction
        {
            if(whichShader[1] or whichShader[3])
            {
                lights[lightsIndex].position.y -= 0.4;
            }
            
            break;
        }
        case 'Y': // move light +y direction
        {
            if(whichShader[1] or whichShader[3])
            {
                lights[lightsIndex].position.y += 0.4;
            }
            
            break;
        }
            
    }

}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
    // prob.bind to call pid
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);
    
    
    Material material1 = {glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.8f, 0.7f, 0.7f), glm::vec3(1.0f, 0.9f, 0.8f), 200.0f}; // Pinkish with strong highlights
    Material material2 = {glm::vec3(0.0f, 0.0f, 0.2f), glm::vec3(0.0f, 0.0f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), 150.0f}; // Blue with green highlights
    Material material3 = {glm::vec3(0.3f, 0.3f, 0.4f), glm::vec3(0.3f, 0.3f, 0.4f), glm::vec3(0.3f, 0.3f, 0.3f), 50.0f}; // Grayish with low shininess
    
    Light light1 = {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.8f, 0.8f, 0.8f)};
    Light light2 = {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.2f, 0.2f, 0.0f)};
    lights.push_back(light1);
    lights.push_back(light2);
    
    
    materials.push_back(material1);
    materials.push_back(material2);
    materials.push_back(material3);
    
    whichShader.push_back(isNormalShader);
    whichShader.push_back(isBlinnPhong);
    whichShader.push_back(isSilhouette);
    whichShader.push_back(isCelShader);
    
    // NORMAL
    prog = make_shared<Program>();
    prog->setShaderNames(RESOURCE_DIR + "normal_vert.glsl", RESOURCE_DIR + "normal_frag.glsl");
    prog->setVerbose(true);
    prog->init();
    prog->addAttribute("aPos");
    prog->addAttribute("aNor");
    prog->addUniform("MV");
    prog->addUniform("P");
    prog->setVerbose(false);
    
    // BLINN PHONG
    prog2 = make_shared<Program>();
    prog2->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
    prog2->setVerbose(true);
    prog2->init();
    prog2->addAttribute("aPos");
    prog2->addAttribute("aNor");
    prog2->addUniform("MV");
    prog2->addUniform("P");
    
    // lights
    prog2->addUniform("light1position");
    prog2->addUniform("light1color");
    prog2->addUniform("light2position");
    prog2->addUniform("light2color");
    
    prog2->addUniform("invTransposeMV");
    prog2->addUniform("lightPos");
    prog2->addUniform("ka");
    prog2->addUniform("kd");
    prog2->addUniform("ks");
    prog2->addUniform("s");
    prog2->setVerbose(false);
    
    // SILHOUETTE
    prog3 = make_shared<Program>();
    prog3->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "sil_frag.glsl");
    prog3->setVerbose(true);
    prog3->init();
    prog3->addAttribute("aPos");
    prog3->addAttribute("aNor");
    prog3->addUniform("MV");
    prog3->addUniform("P");
    prog3->addUniform("invTransposeMV");
    prog3->setVerbose(false);
	
    
    
    // Cell Shader
    prog4 = make_shared<Program>();
    prog4->setShaderNames(RESOURCE_DIR + "cell_vert.glsl", RESOURCE_DIR + "cell_frag.glsl");
    prog4->setVerbose(true);
    prog4->init();
    prog4->addAttribute("aPos");
    prog4->addAttribute("aNor");
    prog4->addUniform("MV");
    prog4->addUniform("P");
    
    // lights
    prog4->addUniform("light1position");
    prog4->addUniform("light1color");
    prog4->addUniform("light2position");
    prog4->addUniform("light2color");
    
    prog4->addUniform("invTransposeMV");
    prog4->addUniform("lightPos");
    prog4->addUniform("ka");
    prog4->addUniform("kd");
    prog4->addUniform("ks");
    prog4->addUniform("s");
    prog4->setVerbose(false);
    
    
	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation
	
	shape = make_shared<Shape>();
    teapot = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();
    
    teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
    teapot->init();
    
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
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	double t = glfwGetTime();
	if(!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}
   
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
    
    camera->applyViewMatrix(MV); // view matrix on left
    
    
    
   
    
    // pass inverse tranaspose to just blin phong not normal shaders
    if(whichShader[1]) // Blinn-Phong
    {
        prog2->bind();
        
        // draw bunny
        MV->pushMatrix();
        
        MV->translate(-0.5f, -0.5f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        
        MV->rotate(t, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        
        glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        
        shape->draw(prog2);
        MV->popMatrix();
        // end draw bunny
        
        
        // draw teapot
        // ORDER: translate -> scale -> shear -> rotate 
        MV->pushMatrix();
        MV->translate(0.5f, 0.0f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        glm::mat4 S(1.0f);
        S[0][1] = 0.5f*cos(t);
        MV->multMatrix(S);
        
        MV->rotate(M_PI, glm::vec3(0.0f, 1.0f, 0.0f));
            
        invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        
        teapot->draw(prog2);
        MV->popMatrix();
        
        // end draw teapot
        
        glUniform3f(prog2->getUniform("lightPos"), 1.0f, 1.0f, 1.0f);
        // Lights
        
        glUniform3f(prog2->getUniform("light1position"), lights[0].position.x,  lights[0].position.y,  lights[0].position.z);
        glUniform3f(prog2->getUniform("light1color"), lights[0].color.r,  lights[0].color.g,  lights[0].color.b);
        
        glUniform3f(prog2->getUniform("light2position"), lights[1].position.x,  lights[1].position.y,  lights[1].position.z);
        glUniform3f(prog2->getUniform("light2color"), lights[1].color.r,  lights[1].color.g,  lights[1].color.b);
        
        glUniform3f(prog2->getUniform("ka"), materials[materialsIndex].ambient.r, materials[materialsIndex].ambient.g, materials[materialsIndex].ambient.b);
        glUniform3f(prog2->getUniform("kd"), materials[materialsIndex].diffuse.r, materials[materialsIndex].diffuse.g, materials[materialsIndex].diffuse.b);
        glUniform3f(prog2->getUniform("ks"), materials[materialsIndex].specular.r, materials[materialsIndex].specular.g, materials[materialsIndex].specular.b);
        glUniform1f(prog2->getUniform("s"), materials[materialsIndex].shininess);
        
        prog2->unbind();
    }
    else if(whichShader[0])
    {
        
        prog->bind();
        
        // draw bunny
        MV->pushMatrix();
        
        MV->translate(-0.5f, -0.5f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        
        MV->rotate(t, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        shape->draw(prog);
        MV->popMatrix();
        // end draw bunny
        
        
        // draw teapot
        // ORDER: translate -> scale -> shear -> rotate
        MV->pushMatrix();
        MV->translate(0.5f, 0.0f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        glm::mat4 S(1.0f);
        S[0][1] = 0.5f*cos(t);
        MV->multMatrix(S);
        
        MV->rotate(M_PI, glm::vec3(0.0f, 1.0f, 0.0f));
            
        invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        
        teapot->draw(prog);
        MV->popMatrix();
        
        prog->unbind();
    
    }
    else if(whichShader[2])
    {
        
        prog3->bind();
        
        // draw bunny
        MV->pushMatrix();
        
        MV->translate(-0.5f, -0.5f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        
        MV->rotate(t, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        
        glUniformMatrix4fv(prog3->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog3->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog3->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        
        shape->draw(prog3);
        MV->popMatrix();
        // end draw bunny
        
        
        // draw teapot
        // ORDER: translate -> scale -> shear -> rotate
        MV->pushMatrix();
        MV->translate(0.5f, 0.0f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        // SHEARING
        glm::mat4 S(1.0f);
        S[0][1] = 0.5f*cos(t);
        MV->multMatrix(S);
        
        MV->rotate(M_PI, glm::vec3(0.0f, 1.0f, 0.0f));
            
        invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        glUniformMatrix4fv(prog3->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog3->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog3->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        
        teapot->draw(prog3);
        MV->popMatrix();
        
        // end draw teapot
        prog3->unbind();
    
    }
    else if(whichShader[3])
    {
        prog4->bind();
        
        // draw bunny
        MV->pushMatrix();
        
        MV->translate(-0.5f, -0.5f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        
        MV->rotate(t, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        
        glUniformMatrix4fv(prog4->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog4->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog4->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        
        shape->draw(prog4);
        MV->popMatrix();
        // end draw bunny
        
        
        // draw teapot
        // ORDER: translate -> scale -> shear -> rotate
        MV->pushMatrix();
        MV->translate(0.5f, 0.0f, 0.0f);
        
        MV->scale(glm::vec3(0.5f, 0.5f, 0.5f));
        
        // SHEARING
        glm::mat4 S(1.0f);
        S[0][1] = 0.5f*cos(t);
        MV->multMatrix(S);
        
        MV->rotate(M_PI, glm::vec3(0.0f, 1.0f, 0.0f));
            
        invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        glUniformMatrix4fv(prog4->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog4->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog4->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        
        teapot->draw(prog4);
        MV->popMatrix();
        
        // end draw teapot
        
        glUniform3f(prog4->getUniform("lightPos"), 1.0f, 1.0f, 1.0f);
        // Lights
        
        glUniform3f(prog4->getUniform("light1position"), lights[0].position.x,  lights[0].position.y,  lights[0].position.z);
        glUniform3f(prog4->getUniform("light1color"), lights[0].color.r,  lights[0].color.g,  lights[0].color.b);
        
        glUniform3f(prog4->getUniform("light2position"), lights[1].position.x,  lights[1].position.y,  lights[1].position.z);
        glUniform3f(prog4->getUniform("light2color"), lights[1].color.r,  lights[1].color.g,  lights[1].color.b);
        
        glUniform3f(prog4->getUniform("ka"), materials[materialsIndex].ambient.r, materials[materialsIndex].ambient.g, materials[materialsIndex].ambient.b);
        glUniform3f(prog4->getUniform("kd"), materials[materialsIndex].diffuse.r, materials[materialsIndex].diffuse.g, materials[materialsIndex].diffuse.b);
        glUniform3f(prog4->getUniform("ks"), materials[materialsIndex].specular.r, materials[materialsIndex].specular.g, materials[materialsIndex].specular.b);
        glUniform1f(prog4->getUniform("s"), materials[materialsIndex].shininess);
        
        prog4->unbind();
    }
	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
	
	if(OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

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
