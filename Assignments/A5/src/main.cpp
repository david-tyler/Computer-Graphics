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
#include "Texture.hpp"


using namespace std;
shared_ptr<Program> prog; // <-- Blinn - Phong <-- progPass 1
glm::mat3 T(1.0f);
// shared_ptr<Program> progPass1;
shared_ptr<Program> progPass2;
shared_ptr<Program> prog3; // <-- for surface of revolution


GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;

shared_ptr<Shape> shape;
shared_ptr<Shape> teapot;
shared_ptr<Shape> cube;
shared_ptr<Shape> sphere;
shared_ptr<Shape> frustum;
shared_ptr<Shape> square;

int textureWidth = 640;
int textureHeight = 480;
GLuint framebufferID;
GLuint textureA;
GLuint textureB;
GLuint textureC;
GLuint textureD;

// STUFF FOR BOUNCING SPHERES
map<string,GLuint> bufIDs;
int indCount;

// STUFF FOR SPIRALY THINGY (SURFACE REVOLUTION)
map<string,GLuint> bufIDs2;
int indCount2;

// QUAD
map<string,GLuint> bufIDs3;
int indCount3;

bool useBlur = false;

bool keyToggles[256] = {false}; // only for English keyboards!

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


class SceneObject {
public:
    shared_ptr<Shape> shape;
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale;
    Material colors;
    float scaleOffset;
    float randRotation;
    bool doScale;
    bool doRotate;
    bool doShear;

    SceneObject(shared_ptr<Shape> _shape, glm::vec3 _translation, glm::vec3 _rotation, glm::vec3 _scale, Material _material, float _scaleOffset, float _randRotation, bool _doScale, bool _doRotate, bool _doShear)
        : shape(_shape), translation(_translation), rotation(_rotation), scale(_scale), colors(_material), scaleOffset(_scaleOffset), randRotation(_randRotation), doScale(_doScale), doRotate(_doRotate), doShear(_doShear){}

    void draw(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, double t) {
        MV->pushMatrix();
        // ORDER: translate -> scale -> shear -> rotate
        
        MV->translate(translation);
        if (doScale)
        {
            float scaleChange = 1.0 + float(0.15/2) + (float(0.15/2)*(sin(2 * M_PI * 0.2f * (t + scaleOffset))));
            glm::vec3 dynamicScale = scale * (scaleChange);
            MV->scale(dynamicScale);
        }
        else
        {
            MV->scale(scale);
        }
         
        // SHEARING
        if (doShear)
        {
            glm::mat4 S(1.0f);
            S[1][0] = 0.5f*cos(t * 2.0f);
            MV->multMatrix(S);
        }
        
        MV->rotate(randRotation, rotation);
        if (doRotate) {
            MV->rotate(t, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        glUniform3f(prog->getUniform("ka"), colors.ambient.r, colors.ambient.g, colors.ambient.b);
        glUniform3f(prog->getUniform("kd"), colors.diffuse.r, colors.diffuse.g, colors.diffuse.b);
        glUniform3f(prog->getUniform("ks"), colors.specular.r, colors.specular.g, colors.specular.b);
        glUniform1f(prog->getUniform("s"), colors.shininess);
        
        shape->draw(prog);
        MV->popMatrix();
    }
    void drawSphere(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, double t)
    {
        
        MV->pushMatrix();
        // ORDER: translate -> scale -> shear -> rotate
        
        MV->translate(translation);
        
        MV->scale(scale);
        
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        glUniform3f(prog->getUniform("ka"), colors.ambient.r, colors.ambient.g, colors.ambient.b);
        glUniform3f(prog->getUniform("kd"), colors.diffuse.r, colors.diffuse.g, colors.diffuse.b);
        glUniform3f(prog->getUniform("ks"), colors.specular.r, colors.specular.g, colors.specular.b);
        glUniform1f(prog->getUniform("s"), colors.shininess);
        
        shape->draw(prog);
        MV->popMatrix();
        
    }
    void drawBouncingBall(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, double t)
    {
        MV->pushMatrix();
        
        float y = 1.3f * ( (1/2.0f) * sin( (2*M_PI/1.7f) * (t + 0.9f)) + (1/2.0f));
        float s = -0.5f * ( (1/2.0f) * cos( (4*M_PI/1.7f) * (t + 0.9f)) + (1/2.0f)) + 1.0f;
        MV->translate(translation.x, y + 0.1f , translation.z);
        
        MV->scale(glm::vec3(s*scale.x, scale.y, s*scale.z));
        
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        glUniform3f(prog->getUniform("ka"), colors.ambient.r, colors.ambient.g, colors.ambient.b);
        glUniform3f(prog->getUniform("kd"), colors.diffuse.r, colors.diffuse.g, colors.diffuse.b);
        glUniform3f(prog->getUniform("ks"), colors.specular.r, colors.specular.g, colors.specular.b);
        glUniform1f(prog->getUniform("s"), colors.shininess);
        glEnableVertexAttribArray(prog->getAttribute("aPos"));
        GLSL::checkError(GET_FILE_LINE);
        glEnableVertexAttribArray(prog->getAttribute("aNor"));
        GLSL::checkError(GET_FILE_LINE);
        glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
        glVertexAttribPointer(prog->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bNor"]);
        glVertexAttribPointer(prog->getAttribute("aNor"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
        glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
        glDrawElements(GL_TRIANGLES, indCount, GL_UNSIGNED_INT, (void *)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(prog->getAttribute("aNor"));
        glDisableVertexAttribArray(prog->getAttribute("aPos"));
        MV->popMatrix();
        
        
    }
    void drawSurfaceOfRevolution(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, double t)
    {
        MV->pushMatrix();
        MV->translate(translation);
        
        MV->scale(scale);
        
        float angle = 90.0f * M_PI/180.0f;
        MV->rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        glUniform1f(prog3->getUniform("t"), t);
        glUniformMatrix4fv(prog3->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog3->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        glUniform3f(prog3->getUniform("ka"), colors.ambient.r, colors.ambient.g, colors.ambient.b);
        glUniform3f(prog3->getUniform("kd"), colors.diffuse.r, colors.diffuse.g, colors.diffuse.b);
        glUniform3f(prog3->getUniform("ks"), colors.specular.r, colors.specular.g, colors.specular.b);
        glUniform1f(prog3->getUniform("s"), colors.shininess);
        glEnableVertexAttribArray(prog3->getAttribute("aPos"));
        GLSL::checkError(GET_FILE_LINE);
        
        GLSL::checkError(GET_FILE_LINE);
        
        glBindBuffer(GL_ARRAY_BUFFER, bufIDs2["bPos"]);
        glVertexAttribPointer(prog3->getAttribute("aPos"), 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glBindBuffer(GL_ARRAY_BUFFER, bufIDs2["bNor"]);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs2["bInd"]);
        glUniformMatrix4fv(prog3->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
        glDrawElements(GL_TRIANGLES, indCount2, GL_UNSIGNED_INT, (void *)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(prog3->getAttribute("aPos"));
        MV->popMatrix();
    }
};

vector<Light> newLights;
std::vector<SceneObject> sceneObjects;
glm::vec3 lightColors[70];
glm::vec3 bouncingSphColors[70];


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
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
    {
        // Toggle the useBlur uniform
        
        progPass2->bind();
        useBlur = !useBlur;
        glUniform1i(progPass2->getUniform("useBlur"), useBlur);
        progPass2->unbind();
        
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);
        
    
    
    // BLINN PHONG
    prog = make_shared<Program>();
    prog->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
    prog->setVerbose(true);
    prog->init();
    prog->addAttribute("aPos");
    prog->addAttribute("aNor");
    prog->addUniform("invTransposeMV");
    prog->addUniform("MV");
    prog->addUniform("P");
    prog->addUniform("t");
    
    // lights
    prog->addUniform("newLightsColors");
    prog->addUniform("newLightsPos");
    prog->addUniform("ka");
    prog->addUniform("kd");
    prog->addUniform("ks");
    prog->addUniform("s");
    prog->setVerbose(false);
    
    progPass2 = make_shared<Program>();
    progPass2->setShaderNames(RESOURCE_DIR + "tvert.glsl", RESOURCE_DIR + "tfrag.glsl");
    progPass2->setVerbose(false);
    progPass2->init();
    progPass2->addAttribute("aPos");
    progPass2->addAttribute("aTex");
    progPass2->addUniform("P");
    progPass2->addUniform("MV");
    progPass2->addUniform("windowSize");
    progPass2->addUniform("textureA");
    progPass2->addUniform("textureB");
    progPass2->addUniform("textureC");
    progPass2->addUniform("textureD");
    progPass2->addUniform("newLightsColors");
    progPass2->addUniform("newLightsPos");
    progPass2->addUniform("useBlur");
    //progPass2->addUniform("ks");
    //progPass2->addUniform("s");

    progPass2->bind();
    glUniform1i(progPass2->getUniform("textureA"), 0);
    glUniform1i(progPass2->getUniform("textureB"), 1);
    glUniform1i(progPass2->getUniform("textureC"), 2);
    glUniform1i(progPass2->getUniform("textureD"), 3);
    progPass2->unbind();

    
    // Surface of Revolution
    prog3 = make_shared<Program>();
    prog3->setShaderNames(RESOURCE_DIR + "cell_vert.glsl", RESOURCE_DIR + "frag.glsl");
    prog3->setVerbose(true);
    prog3->init();
    prog3->addAttribute("aPos");
    prog3->addAttribute("aNor");
    prog3->addUniform("invTransposeMV");
    prog3->addUniform("MV");
    prog3->addUniform("P");
    prog3->addUniform("t");
    
    // lights
    prog3->addUniform("newLightsColors");
    prog3->addUniform("newLightsPos");
    prog3->addUniform("ka");
    prog3->addUniform("kd");
    prog3->addUniform("ks");
    prog3->addUniform("s");
    prog3->setVerbose(false);
    //
    // Vertex buffer setup
    //
    
    // BOUNCING SPHERE
    vector<float> posBuf;
    vector<float> norBuf;
    vector<unsigned int> indBuf;
    
    // SPIRAL THINGY (SURFACE OF REVOLUTION)
    vector<float> posBuf2;
    vector<float> norBuf2;
    vector<unsigned int> indBuf2;
    
    // QUAD
    vector<float> posBuf3;
    vector<float> norBuf3;
    vector<unsigned int> indBuf3;
    
	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation
	
	shape = make_shared<Shape>();
    teapot = make_shared<Shape>();
    cube = make_shared<Shape>();
    sphere = make_shared<Shape>();
    frustum = make_shared<Shape>();
    square = make_shared<Shape>();
    
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();
    
    teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
    teapot->init();
    
    cube->loadMesh(RESOURCE_DIR + "cube.obj");
    cube->init();
    
    sphere->loadMesh(RESOURCE_DIR + "sphere.obj");
    sphere->init();
    
    frustum->loadMesh(RESOURCE_DIR + "frustum.obj");
    frustum->init();
    
    square->loadMesh(RESOURCE_DIR + "square.obj");
    square->init();

    // Create ground plane
    float scaleOffs = 0.0f;
    Material material1 = {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f), 10.0f}; // Green
    SceneObject ground(cube, glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(15.0f, 0.01f, 15.0f), material1, scaleOffs, 0.0f, false, false, false);
    sceneObjects.push_back(ground);
    
    // Create 100 objects
    int numObjectsPerRow = 10;
    int numObjectsPerColumn = 10;
    float spacing = 1.2f;
    
    float avgHeight = 0;
    for (int i = 0; i < numObjectsPerRow; ++i) {
        for (int j = 0; j < numObjectsPerColumn; ++j) {
            scaleOffs += 0.1f;
            float randX = -6 + static_cast<float>(i) * spacing ;  // Random x-coordinate within grid width
            float randZ = -6 + static_cast<float>(j) * spacing ; // Random z-coordinate within grid height
            float randScale = 0.2f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.5f; // Random scale between 0.1 and 0.5
            
            // random colors
            float randomR = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
            float randomG = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
            float randomB = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
            
            float randRotation = (float)(rand() % 360);

            // ambient is now an emissive color of 0.
            material1 = {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(randomR, randomG, randomB), glm::vec3(1.0f, 1.0f, 1.0f), 10.0f};
            if (j % 2 == 0) {
                float minY = teapot->getMinY();
                avgHeight += (-minY * randScale - 0.2f );
                SceneObject tea(teapot, glm::vec3(randX, ((-minY * randScale ) - 0.2f), randZ), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(randScale), material1, scaleOffs,  randRotation, false, false, true);
                sceneObjects.push_back(tea);
            } else {
                float minY = shape->getMinY();
                avgHeight += (-minY * randScale - 0.2f );
                SceneObject bun(shape, glm::vec3(randX, (-minY * randScale )  - 0.2f, randZ), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(randScale), material1, scaleOffs, randRotation, false, true, false);
                sceneObjects.push_back(bun);
            }
        }
    }
    
    
    // draw Bouncing Spheres
    for (int i = 0; i < 10; i++) {
        
        float r = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
        float g = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
        float b = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
        
        bouncingSphColors[i] = glm::vec3(r, g, b);
        
    }
    for (int i = 0; i < 25; i++) {
        scaleOffs += 0.1f;
        float x = -7 + static_cast<float>(rand() % 15);  // Random x-coordinate within grid width
        float z = -7 + static_cast<float>(rand() % 15); // Random z-coordinate within grid height
        
        // ambient is now an emissive color of 0.
        material1 = {glm::vec3(0.0f, 0.0f, 0.0f), bouncingSphColors[(i + 1) % 10], glm::vec3(1.0f, 1.0f, 1.0f), 10.0f};
        
        SceneObject BBall(shape, glm::vec3(x, 0.0f, z), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.3f), material1, scaleOffs, 0.0f, false, false, false);
        sceneObjects.push_back(BBall);
        
        
    }
    // draw Spirals Surface of Revolutions
    for (int i = 0; i < 10; i++) {
        scaleOffs += 0.1f;
        float x = -7 + static_cast<float>(rand() % 15);  // Random x-coordinate within grid width
        float z = -7 + static_cast<float>(rand() % 15); // Random z-coordinate within grid height
        
        // ambient is now an emissive color of 0.
        material1 = {glm::vec3(0.0f, 0.0f, 0.0f), bouncingSphColors[(i + 1) % 10], glm::vec3(1.0f, 1.0f, 1.0f), 10.0f};
        
        SceneObject spiral(shape, glm::vec3(x, 0.0f, z), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.1f), material1, scaleOffs, 0.0f, false, false, false);
        sceneObjects.push_back(spiral);
        
        
    }
    // CREATING LIGHTS
    avgHeight /= 100.0f;
    
    Material lightMat;
    for (int i = 0; i < 70; i++) {
        float randx = -7 + static_cast<float>(rand() % 15);  // Random x-coordinate within grid width
        float randz = -7 + static_cast<float>(rand() % 15); // Random z-coordinate within grid height
        
        float r = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
        float g = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
        float b = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
        
        Light newlight = {glm::vec3(randx , avgHeight + 0.3f, randz ), glm::vec3(r, g, b)};
        lightMat = {newlight.color, glm::vec3(0.0f), glm::vec3(0.0f), 10.0f};
        lightColors[i] = newlight.color;
        SceneObject lightSphere(sphere,glm::vec3(newlight.position.x, newlight.position.y, newlight.position.z) , glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.05f, 0.05f, 0.05f), lightMat, 0.0f, 0.0f, false, false, false);
        sceneObjects.push_back(lightSphere);
        
        newLights.push_back(newlight);
    }
    
    
    
    
    int numPoints = 50;
    for (int i = 0; i < numPoints; i++) {
        float theta = static_cast<float>(i) / (numPoints - 1) * M_PI; // From [0, pi]
        for (int j = 0; j < numPoints; j++) {
            
            float phi = static_cast<float>(j) / (numPoints - 1) * 2 * M_PI; // Range [0, 2pi]
            
            float x = sin(theta) * sin(phi);
            float y = cos(theta);
            float z = sin(theta) * cos(phi);

            posBuf.push_back(x);
            posBuf.push_back(y);
            posBuf.push_back(z);
            
            norBuf.push_back(x);
            norBuf.push_back(y);
            norBuf.push_back(z);
            
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
    GLuint tmp[3];
    glGenBuffers(3, tmp);
    bufIDs["bPos"] = tmp[0];
    bufIDs["bNor"] = tmp[1];
    //bufIDs["bTex"] = tmp[2];
    bufIDs["bInd"] = tmp[2];
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
    glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bNor"]);
    glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bTex"]);
    //glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size()*sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(norBuf.size() == posBuf.size());
    // END DRAWING BOUNCING SPHERES
    
    // DRAWING SURFACE OF REVOLUTOIN SPIRLA THINGY
    float maxX = 10.0f; // Maximum value of x
    float maxTheta = 2.0f * M_PI; // Maximum value of theta
    numPoints = 50;
    
    for (int i = 0; i < numPoints; i++) {
       float x = maxX * static_cast<float>(i) / (numPoints - 1);
       for (int j = 0; j < numPoints; j++) {
           
           float theta = maxTheta * static_cast<float>(j) / (numPoints - 1);
           
           // Position (send x and theta)
           posBuf2.push_back(x);
           posBuf2.push_back(theta);
           posBuf2.push_back(0.0f);
           
           // Normal (send in zeros)
           norBuf2.push_back(0.0f);
           norBuf2.push_back(0.0f);
           norBuf2.push_back(0.0f);
       }
    }
    for (int i = 0; i < numPoints - 1; ++i) {
       for (int j = 0; j < numPoints - 1; ++j) {
           int index = i * numPoints + j;
           indBuf2.push_back(index);
           indBuf2.push_back(index + 1);
           indBuf2.push_back(index + 1 + numPoints);

           indBuf2.push_back(index);
           indBuf2.push_back(index + numPoints + 1);
           indBuf2.push_back(index + numPoints);
       }
    }
    // Total number of indices
    indCount2 = (int)indBuf2.size();
    
    
       
    // Generate buffer IDs and put them in the bufIDs map.
    GLuint tmp2[3];
    glGenBuffers(3, tmp2);
    bufIDs2["bPos"] = tmp2[0];
    bufIDs2["bNor"] = tmp2[1];
    bufIDs2["bInd"] = tmp2[2];
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs2["bPos"]);
    glBufferData(GL_ARRAY_BUFFER, posBuf2.size()*sizeof(float), &posBuf2[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs2["bNor"]);
    glBufferData(GL_ARRAY_BUFFER, norBuf2.size()*sizeof(float), &norBuf2[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs2["bInd"]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf2.size()*sizeof(unsigned int), &indBuf2[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    assert(norBuf2.size() == posBuf2.size());
    
    // QUAD
    // Vert 0
    
    posBuf3.push_back(-0.5f);
    posBuf3.push_back(-0.5f);
    posBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(1.0f);
    
    // Vert 1
    posBuf3.push_back(0.5f);
    posBuf3.push_back(-0.5f);
    posBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(1.0f);
  
    // Vert 2
    posBuf3.push_back(-0.5f);
    posBuf3.push_back(0.5f);
    posBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(1.0f);
    
    // Vert 3
    posBuf3.push_back(0.5f);
    posBuf3.push_back(0.5f);
    posBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(0.0f);
    norBuf3.push_back(1.0f);
    
    // indices
    indBuf3.push_back(0);
    indBuf3.push_back(1);
    indBuf3.push_back(3);
    indBuf3.push_back(0);
    indBuf3.push_back(3);
    indBuf3.push_back(2);
    
    // Creating vertices
    // WORKS FOR ALL THE WAY TO 50 X 50 POINTS
    
    numPoints = 3;
    float stepSize = 1.0f / (numPoints - 1);
    for (int i = 0; i < numPoints; i++) {
        for (int j = 0; j < numPoints; j++) {
            float x = -0.5f + j * stepSize;
            float y = -0.5f + i * stepSize;
            
            posBuf3.push_back(x);
            posBuf3.push_back(y);
            posBuf3.push_back(0.0f);
            
            norBuf3.push_back(0.0f);
            norBuf3.push_back(0.0f);
            norBuf3.push_back(1.0f);
            
        }
    }
    for (int i = 0; i < numPoints - 1; ++i) {
        for (int j = 0; j < numPoints - 1; ++j) {
                int index = i * numPoints + j;
                indBuf3.push_back(index);
                indBuf3.push_back(index + 1);
                indBuf3.push_back(index + 1 + numPoints);

                indBuf3.push_back(index);
                indBuf3.push_back(index + numPoints + 1);
                indBuf3.push_back(index + numPoints);
            }
        }
    GLuint tmp3[2];
    glGenBuffers(2, tmp3);
    bufIDs3["bPos"] = tmp3[0];
    bufIDs3["bInd"] = tmp3[1];
    glBindBuffer(GL_ARRAY_BUFFER, bufIDs3["bPos"]);
    glBufferData(GL_ARRAY_BUFFER, posBuf3.size()*sizeof(float), &posBuf3[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs3["bInd"]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf3.size()*sizeof(unsigned int), &indBuf3[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
    assert(norBuf3.size() == posBuf3.size());
    
    GLSL::checkError(GET_FILE_LINE);
    
    // Creating textures
    glGenFramebuffers(1, &framebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    
    // set up textureA
    glGenTextures(1, &textureA);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, textureWidth, textureHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureA, 0);
    
    // set up textureB
    glGenTextures(1, &textureB);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, textureWidth, textureHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textureB, 0);
    
    // set up textureC
    glGenTextures(1, &textureC);
    glBindTexture(GL_TEXTURE_2D, textureC);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, textureWidth, textureHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, textureC, 0);
    
    // set up textureD
    glGenTextures(1, &textureD);
    glBindTexture(GL_TEXTURE_2D, textureD);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, textureWidth, textureHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, textureD, 0);
    
    // set up depth tests to happen in pass 1
    GLuint depthrenderbuffer;
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, textureWidth, textureHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    // We now tell OpenGL that we want two textures as the output of this framebuffer.
    GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, attachments);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "Framebuffer is not ok" << endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
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
	/*if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}*/
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
    glViewport(0, 0, width, height);

	double t = glfwGetTime();
    
	if(!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}
   
    // Matrix stacks
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
    camera->setAspect(1.0f);
    
    vector<glm::vec3> newLightPositions;
    // Apply camera transforms
    
    P->pushMatrix();
    // DRAW MAIN SCENE
    camera->applyProjectionMatrix(P);
    MV->pushMatrix();
    camera->applyViewMatrix(MV); // view matrix on left
    // Lights
    
   
    
    glm::vec4 newLightPosView;
    glm::vec3 newLightPos;
    for (int i = 0; i < 70; i++) {
        newLightPosView = MV->topMatrix() * glm::vec4(newLights[i].position, 1.0f); // tranform light into camera space
        newLightPos = glm::vec3(newLightPosView);
        newLightPositions.push_back(newLightPos);
       
    }
    prog->bind();
    
    for (int i = 0; i < 70; i++) {
        glUniform3fv(prog->getUniform("newLightsColors"), 70, glm::value_ptr(lightColors[0]));
        glUniform3fv(prog->getUniform("newLightsPos"), 70, glm::value_ptr(newLightPositions[0]));
    }
    
    // Draw Bonucing Balls
    for (int i = 101; i < 126; i++) {
        sceneObjects[i].drawBouncingBall(P, MV, t);
    }
    
    
    sceneObjects[0].draw(P, MV, t); // ground
    // Draw 100 objects
    for (int i = 1; i < 101; i++) {
        sceneObjects[i].draw(P, MV, t);
    }
    for (int i = 136; i < 206; i++) {
        sceneObjects[i].drawSphere(P, MV, t);
    }
    prog->unbind();
    
    prog3->bind();
    for (int i = 0; i < 70; i++) {
        glUniform3fv(prog3->getUniform("newLightsColors"), 70, glm::value_ptr(lightColors[0]));
        glUniform3fv(prog3->getUniform("newLightsPos"), 70, glm::value_ptr(newLightPositions[0]));
    }
    // DRAW SPIRALS (SURFACE OF REVOLUTION
    for (int i = 126; i < 136; i++) {
        sceneObjects[i].drawSurfaceOfRevolution(P, MV, t);
    }
    
    prog3->unbind();
    
    
    MV->popMatrix();
    P->popMatrix();
    

    
    // PASS2
    //////////////////////////////////////////////////////
    // Render to the screen
    //////////////////////////////////////////////////////
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    auto P_HUD = make_shared<MatrixStack>();
    
    camera->setAspect((float)width/(float)height);
    P->pushMatrix();
    progPass2->bind();
    
    MV->pushMatrix();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureC);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textureD);
    
    P_HUD->pushMatrix();
    camera->applyProjectionMatrix(P_HUD);
    for (int i = 0; i < 70; i++) {
        glUniform3fv(progPass2->getUniform("newLightsColors"), 70, glm::value_ptr(lightColors[0]));
        glUniform3fv(progPass2->getUniform("newLightsPos"), 70, glm::value_ptr(newLightPositions[0]));
    }    MV->pushMatrix();
    
    // drawing quad
    MV->scale(glm::vec3(2.0f));
    glUniformMatrix4fv(progPass2->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
    glUniformMatrix4fv(progPass2->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
    glUniform2f(progPass2->getUniform("windowSize"), width, height);
    square->draw(progPass2);
    MV->popMatrix();
    P_HUD->popMatrix();
    
    camera->applyProjectionMatrix(P);
    camera->applyViewMatrix(MV);
    
    
    glActiveTexture(GL_TEXTURE0);

    progPass2->unbind();
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
