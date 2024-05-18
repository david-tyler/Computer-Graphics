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
shared_ptr<Program> prog; // texture
shared_ptr<Program> prog2; // <-- Blinn - Phong
glm::mat3 T(1.0f);

shared_ptr<Texture> texture0;
shared_ptr<Texture> texture1;
shared_ptr<Texture> texture2;
glm::vec3 lightPosCam;


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


shared_ptr<Program> prog3; // <-- Silhoette Shader
shared_ptr<Program> prog4; // <-- Cell Shader
shared_ptr<Shape> shape;
shared_ptr<Shape> teapot;
shared_ptr<Shape> cube;
shared_ptr<Shape> sphere;
shared_ptr<Shape> frustum;

bool topDown = false;


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

    SceneObject(shared_ptr<Shape> _shape, glm::vec3 _translation, glm::vec3 _rotation, glm::vec3 _scale, Material _material, float _scaleOffset, float _randRotation)
        : shape(_shape), translation(_translation), rotation(_rotation), scale(_scale), colors(_material), scaleOffset(_scaleOffset), randRotation(_randRotation){}

    void draw(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, double t, bool doScale) {
        MV->pushMatrix();
        // ORDER: translate -> scale -> shear -> rotate
        
        MV->translate(translation);
        if (doScale) {
            float scaleChange = 1.0 + float(0.15/2) + (float(0.15/2)*(sin(2 * M_PI * 0.2f * (t + scaleOffset))));
            glm::vec3 dynamicScale = scale * (scaleChange);
            MV->scale(dynamicScale);
        }
        else{
            MV->scale(scale);
        }
        
        
        // SHEARING
        /*glm::mat4 S(1.0f);
        S[0][1] = 0.5f*cos(t);
        MV->multMatrix(S); */
        
        MV->rotate(randRotation, rotation);
        
        glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
        
        
        glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        glUniform3f(prog2->getUniform("ka"), colors.ambient.r, colors.ambient.g, colors.ambient.b);
        glUniform3f(prog2->getUniform("kd"), colors.diffuse.r, colors.diffuse.g, colors.diffuse.b);
        glUniform3f(prog2->getUniform("ks"), colors.specular.r, colors.specular.g, colors.specular.b);
        glUniform1f(prog2->getUniform("s"), colors.shininess);
        
        shape->draw(prog2);
        MV->popMatrix();
    }
    void groundDraw(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, double t, bool doScale, glm::vec3 lightPosCS) {
        texture0->bind(prog->getUniform("texture0"));
        texture1->bind(prog->getUniform("texture1"));
        texture2->bind(prog->getUniform("texture2"));
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T));
        glUniform3fv(prog->getUniform("lightPosCam"), 1, glm::value_ptr(lightPosCS));
        
        sphere->draw(prog);
        texture1->unbind();
        texture0->unbind();
       
    }
};
vector<Material> materials;
vector<Light> lights;
vector<bool> whichShader;
std::vector<SceneObject> sceneObjects;

void drawHUD(shared_ptr<MatrixStack> P, shared_ptr<MatrixStack> MV, double t){
    // draw HUD Objects
    
       // draw bunny
       
        MV->pushMatrix();
        MV->translate(-0.7f, 0.415f, -0.15f);
        MV->scale(glm::vec3(0.3f));
        MV->rotate(t, glm::vec3(0.0f, -1.0f, 0.0f));
      
       glm::mat4 invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
          
          
        glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
        glUniform3f(prog2->getUniform("ka"), materials[1].ambient.r, materials[1].ambient.g, materials[1].ambient.b);
        glUniform3f(prog2->getUniform("kd"), materials[1].diffuse.r, materials[1].diffuse.g, materials[1].diffuse.b);
        glUniform3f(prog2->getUniform("ks"), materials[1].specular.r, materials[1].specular.g, materials[1].specular.b);
        glUniform1f(prog2->getUniform("s"), materials[1].shininess);
        shape->draw(prog2);
        MV->popMatrix();
       // end draw bunny
      
      
       // draw teapot
       // ORDER: translate -> scale -> shear -> rotate
       MV->pushMatrix();
       MV->translate(0.7f, 0.515f, -0.15f);
       
       MV->scale(glm::vec3(0.3f));
      
       /*glm::mat4 S(1.0f);
       S[0][1] = 0.5f*cos(t);
       MV->multMatrix(S);*/
       
       MV->rotate(t, glm::vec3(0.0f, -1.0f, 0.0f));
          
       invTransposeMV = glm::transpose(glm::inverse(MV->topMatrix()));
      
       glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
       glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
    glUniformMatrix4fv(prog2->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(invTransposeMV));
       glUniform3f(prog2->getUniform("ka"), materials[1].ambient.r, materials[1].ambient.g, materials[1].ambient.b);
       glUniform3f(prog2->getUniform("kd"), materials[1].diffuse.r, materials[1].diffuse.g, materials[1].diffuse.b);
       glUniform3f(prog2->getUniform("ks"), materials[1].specular.r, materials[1].specular.g, materials[1].specular.b);
       glUniform1f(prog2->getUniform("s"), materials[1].shininess);
    
       teapot->draw(prog2);
       MV->popMatrix();
    
       // end draw teapot
    
}
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
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera->processKeyboardInput(window);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera->processKeyboardInput(window);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera->processKeyboardInput(window);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera->processKeyboardInput(window);
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        topDown = !topDown;
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
    if (key == 'z') {
        // increase FOV
        camera->setFOV(3.0f);
    } else if (key == 'Z') {
        // decrease FOV
        camera->setFOV(-3.0f);
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
    
    
    Material material2 = {glm::vec3(0.0f, 0.0f, 0.2f), glm::vec3(0.0f, 0.0f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), 150.0f}; // Blue with green highlights
    Material material3 = {glm::vec3(0.3f, 0.3f, 0.4f), glm::vec3(0.3f, 0.3f, 0.4f), glm::vec3(0.3f, 0.3f, 0.3f), 50.0f}; // Grayish with low shininess
    
    Light light1 = {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.8f, 0.8f, 0.8f)};
    Light light2 = {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.2f, 0.2f, 0.0f)};
    
    Light light3 = {glm::vec3(10.0f, 30.0f, 10.0f), glm::vec3(0.8f, 0.8f, 0.8f)};
    
    lights.push_back(light1);
    lights.push_back(light2);
    lights.push_back(light3);

    
    
    
    materials.push_back(material2);
    materials.push_back(material3);
    
    
    
    // TEXTURE
    prog = make_shared<Program>();
    prog->setShaderNames(RESOURCE_DIR + "tvert.glsl", RESOURCE_DIR + "tfrag.glsl");
    prog->setVerbose(true);
    prog->init();
    prog->addAttribute("aPos");
    prog->addAttribute("aNor");
    prog->addAttribute("aTex");
    prog->addUniform("P");
    prog->addUniform("MV");
    prog->addUniform("T");
    prog->addUniform("texture0");
    prog->addUniform("texture1");
    prog->addUniform("texture2");
    prog->addUniform("lightPosCam");
    prog->setVerbose(false);
    
    texture0 = make_shared<Texture>();
    texture0->setFilename(RESOURCE_DIR + "earthKd.jpg");
    texture0->init();
    texture0->setUnit(0);
    texture0->setWrapModes(GL_REPEAT, GL_REPEAT);
    
    texture1 = make_shared<Texture>();
    texture1->setFilename(RESOURCE_DIR + "earthKs.jpg");
    texture1->init();
    texture1->setUnit(1);
    texture1->setWrapModes(GL_REPEAT, GL_REPEAT);
    
    texture2 = make_shared<Texture>();
    texture2->setFilename(RESOURCE_DIR + "earthClouds.jpg");
    texture2->init();
    texture2->setUnit(2);
    texture2->setWrapModes(GL_REPEAT, GL_REPEAT);
    
    lightPosCam.x = 1.0f;
    lightPosCam.y = 1.0f;
    lightPosCam.z = 1.0f;
    
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
    
    prog2->addUniform("invTransposeMV");
    prog2->addUniform("lightPos");
    prog2->addUniform("ka");
    prog2->addUniform("kd");
    prog2->addUniform("ks");
    prog2->addUniform("s");
    prog2->setVerbose(false);
    
    
	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation
	
	shape = make_shared<Shape>();
    teapot = make_shared<Shape>();
    cube = make_shared<Shape>();
    sphere = make_shared<Shape>();
    frustum = make_shared<Shape>();
    
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
    

    // Create ground plane
    float scaleOffs = 0.0f;
    Material material1 = {glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.1f, 0.7f, 0.1f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f}; // Green
    SceneObject ground(cube, glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(25.0f, 0.01f, 25.0f), material1, scaleOffs, 0.0f);
    sceneObjects.push_back(ground);
    
    Material sunLight = {glm::vec3(1.0f, 1.0f, 0.07f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 200.0f}; // yellow
    SceneObject sph(sphere, lights[2].position, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), sunLight, 0.0f, 0.0f);
    sceneObjects.push_back(sph);

    int numObjectsPerRow = 10;
    int numObjectsPerColumn = 10;
    float spacing = 2.0f;
    // Calculate the spacing between objects
    float gridSpacingX = (10 - (-10)) / static_cast<float>(numObjectsPerRow);
    float gridSpacingZ = (10 - (-10)) / static_cast<float>(numObjectsPerColumn);
    // Create 100 objects
    
    for (int i = 0; i < numObjectsPerRow; ++i) {
        for (int j = 0; j < numObjectsPerColumn; ++j) {
            scaleOffs += 0.1f;
            float randX = -10 + static_cast<float>(i) * gridSpacingX + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * spacing;
            float randZ = -10 + static_cast<float>(j) * gridSpacingZ + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * spacing;
            float randScale = 0.1f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.7f; // Random scale between 0.1 and 0.5
            
            // random colors
            float randomR = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
            float randomG = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
            float randomB = 0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.9f));
            
            float randRotation = (float)(rand() % 360);
            
            material1 = {glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(randomR, randomG, randomB), glm::vec3(1.0f, 0.9f, 0.8f), 200.0f};
            if (j % 2 == 0) {
                float minY = teapot->getMinY();
                SceneObject tea(teapot, glm::vec3(randX, ((-minY * randScale ) - 0.2f), randZ), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(randScale), material1, scaleOffs, randRotation);
                sceneObjects.push_back(tea);
            } else {
                float minY = shape->getMinY();
                SceneObject bun(shape, glm::vec3(randX, (-minY * randScale )  - 0.2f, randZ), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(randScale), material1, scaleOffs, randRotation);
                sceneObjects.push_back(bun);
            }
        }
    }
    
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
    auto P_HUD = make_shared<MatrixStack>();
    
    Light lightW = lights[2];
    // Apply camera transforms
    
    P->pushMatrix();
    
    MV->pushMatrix();
    // Lights
    
    glm::vec4 lightPosView = MV->topMatrix() * glm::vec4(lightW.position, 1.0f); // tranform light into camera space
    glm::vec3 lightPosCS = glm::vec3(lightPosView);
    
    
    prog2->bind();
    
   
    glUniform3f(prog2->getUniform("light1position"), lightPosCS.x,  lightPosCS.y,  lightPosCS.z);
    glUniform3f(prog2->getUniform("light1color"), lightW.color.r,  lightW.color.g,  lightW.color.b);
    
    // DRAW HUD
    P_HUD->pushMatrix();
    
    P_HUD->multMatrix(glm::perspective((float)(45.0*M_PI/180.0), (float)width/(float)height, 0.1f, 1000.0f));
    MV->pushMatrix();
    camera->applyProjectionMatrix(P_HUD);
    drawHUD(P, MV, t);
    MV->popMatrix();
    P_HUD->popMatrix();
    // END DRAW HUD
    
    // DRAW MAIN SCENE
    camera->applyProjectionMatrix(P);
    camera->applyViewMatrix(MV); // view matrix on left
    
    
    sceneObjects[1].draw(P, MV, t, false); // sun
    sceneObjects[0].draw(P, MV, t, false); // ground
    
    for (int i = 2; i < sceneObjects.size(); i++) {
        sceneObjects[i].draw(P, MV, t, true);
    }
    prog2->unbind();
    MV->popMatrix();
    P->popMatrix();
	
    // NEW VIEWPORT
    // Top-down viewport (new code for this task)
    if (topDown){
        auto topdownP = make_shared<MatrixStack>();
        auto topdownMV = make_shared<MatrixStack>();
        
        double s = 0.5;
        glViewport(0, 0, s*width, s*height);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, s*width, s*height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        topdownP->pushMatrix();
        
        // apply projection matrix
        topdownP->multMatrix(glm::ortho(0.0f, 25.0f, 0.0f, 25.0f, -30.0f, 30.0f));
        
        topdownMV->pushMatrix();
        prog2->bind();
        // apply view matrix
        topdownMV->translate(glm::vec3(12.5f, 12.5f, 0.0f));
        
        
        topdownMV->rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        
        
        // Lights
        glm::vec4 topdownLightPos = topdownMV->topMatrix() * glm::vec4(lightW.position, 1.0f); // tranform light into camera space
        lightW.position = glm::vec3(topdownLightPos);
        glUniform3f(prog2->getUniform("light1position"), lightW.position.x,  lightW.position.y,  lightW.position.z);
        glUniform3f(prog2->getUniform("light1color"), lightW.color.r,  lightW.color.g,  lightW.color.b);
        
        
        // DRAW FRUSTUM
        topdownMV->pushMatrix();
        
        
        
        
        glm::mat4 cameraMatrix = camera->inverseLookAt();
        
        topdownMV->multMatrix(cameraMatrix);
        
        float a = float(width/height);
        float theta = camera->getFOVy();
        float sX = float(a * glm::tan(theta/2));
        float sY = float(glm::tan(theta/2));
        
        topdownMV->scale(glm::vec3(sX, sY, 1.0f));
        
        
        glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(topdownP->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(topdownMV->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("invTransposeMV"), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
        glUniform3f(prog2->getUniform("ka"), 0.0f, 0.0f, 0.0f);
        glUniform3f(prog2->getUniform("kd"), 0.0f, 0.0f, 0.0f);
        glUniform3f(prog2->getUniform("ks"), materials[1].specular.r, materials[1].specular.g, materials[1].specular.b);
        glUniform1f(prog2->getUniform("s"), materials[1].shininess);
        
        frustum->draw(prog2);
        topdownMV->popMatrix();
        // END DRAW FRUSTUM
        
        sceneObjects[1].draw(topdownP, topdownMV, t, false);
        
        sceneObjects[0].draw(topdownP, topdownMV, t, false);
        
        for (int i = 2; i < sceneObjects.size(); i++) {
            sceneObjects[i].draw(topdownP, topdownMV, t, true);
        }
        
        
        prog2->unbind();
        
        topdownMV->popMatrix();
        topdownP->popMatrix();
    }
    
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
