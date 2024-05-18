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

using namespace std;
using namespace glm;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from

shared_ptr<Camera> camera;
shared_ptr<Program> progShape;
shared_ptr<Program> progSimple;

// Sphere and Ellipse
shared_ptr<Shape> shapeSphere;
vec3 sphere_T;
float sphere_S;
mat4 ellipsoid_E;

// Plane
shared_ptr<Shape> shapePlane;
vec3 plane_n;
vec3 plane_c;

class Hit
{
public:
	Hit() : x(0), n(0), t(0) {}
	Hit(const vec3 &x, const vec3 &n, float t) { this->x = x; this->n = n; this->t = t; }
	vec3 x; // position
	vec3 n; // normal
	float t; // distance
};

vec3 ray[2];
vector<Hit> hits;

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
		
		if(shift) {
			hits.clear();
			
			// Create projection, view, and camera matrices
			auto MV = make_shared<MatrixStack>();
			MV->loadIdentity();
			camera->applyProjectionMatrix(MV);
			mat4 P = MV->topMatrix();
			MV->loadIdentity();
			camera->applyViewMatrix(MV);
			mat4 V = MV->topMatrix();
			mat4 C = inverse(V);
			
			// Task 1: Update ray
            vec3 cw = vec3(C[3][0], C[3][1], C[3][2]);
            ray[0] = cw;
            // To get the ray direction, we need to go through the following series of transformations: pixel coords → normalized device coords → clip coords → eye coords → world coords.
            // Pixel coords → normalized device coords
            float xn = (2.0f * (float)xmouse ) / (float) width - 1.0f;
            float yn = 1.0f - (2.0f * (float)ymouse) / (float) height;
            
            // Normalized device coords → clip coords
            //                              w doesn't matter what we put here so we put 1
            vec4 pc = vec4(xn, yn, -1.0f, 1.0f);
            
            // Clip coords → eye coords: For this step, we use the inverse of the projection matrix:
            vec4 pe = inverse(P) * pc;
            // Since this projection matrix affects the homogeneous coordinate, after this multiplication manually set the w component of pe to 1.0
            pe = vec4(pe.x, pe.y, -1.0f, 1.0f);
            
            // Eye coords → world coords: For this step, we use the camera matrix (which is the inverse of the view matrix):
            vec4 pw = C * pe;
            
            // After all of these steps, we have pw, a point whose position falls under the mouse in world coordinates. Note, however, that there are infinitely many such positions in 3D. What we’re interested in is the ray direction, so we subtract the camera position from this and normalize:
            
            vec3 vw = normalize(vec3(pw.x, pw.y, pw.z) - cw);
            ray[1] = vw;
			
			// Task 2: Find plane intersection
            vec3 c = vec3(0.0f, 0.0f, -1.0f);
            vec3 normal = vec3(0.0f, 0.0f, 1.0f);
            
            float t = dot(normal, (c - vec3(pw.x, pw.y, pw.z))) / dot(normal, vw);
            vec3 x = vec3(pw.x, pw.y, pw.z) + t * vw;
			
            Hit rpInt(x, normal, t);
            hits.push_back(rpInt);
			
			// Task 3: Find sphere intersection(s)
            float a = dot(vw, vw);
            float b = 2 * dot(vw, vec3(pw));
            float newC = dot(vec3(pw), vec3(pw)) - 1.0f;
            float d = pow(b, 2) - (4 * a * newC);
            if (d > 0){
                float t1 = (-b + sqrt(d)) / (2.0f * a);
                float t2 = (-b - sqrt(d)) / (2.0f * a);
                vec3 x1 = vec3(pw.x, pw.y, pw.z) + t1 * vw;
                vec3 x2 = vec3(pw.x, pw.y, pw.z) + t2 * vw;
                // For a unit sphere at the origin, the normal is the same as the position: n = x.
                Hit one(x1, x1, t1);
                Hit two(x2, x2, t2);
                hits.push_back(one);
                hits.push_back(two);
            }
            
			// Task 4: Find scaled/translated sphere intersection(s)
            vec3 newPC = vec3(pw) - sphere_T;
            b = 2.0f * dot(vw, newPC);
            newC = dot(newPC, newPC) - pow(sphere_S, 2.0f);
            d = pow(b, 2) - (4 * a * newC);
            
            if (d > 0){
                float t1 = (-b + sqrt(d)) / (2.0f * a);
                float t2 = (-b - sqrt(d)) / (2.0f * a);
                vec3 x1 = vec3(pw.x, pw.y, pw.z) + t1 * vw;
                vec3 x2 = vec3(pw.x, pw.y, pw.z) + t2 * vw;
                
                vec3 normal1 = (x1 - sphere_T) / sphere_S;
                vec3 normal2 = (x2 - sphere_T) / sphere_S;
                Hit one(x1, normal1, t1);
                Hit two(x2, normal2, t2);
                hits.push_back(one);
                hits.push_back(two);
            }
            
            
            
			// Task 5: Find ellipsoid intersection(s)
            vec4 pLocalSpace = inverse(ellipsoid_E) * vec4(pw.x, pw.y, pw.z, 1.0f);
            vec4 vLocalSpace = inverse(ellipsoid_E) * vec4(vw, 0.0f);
            vec3 pLS = vec3(pLocalSpace);
            vec3 vLS = vec3(vLocalSpace);
            
            vLS = normalize(vLS);
            
            a = dot(vLS, vLS);
            b = 2.0f * dot(vLS, pLS);
            newC = dot(pLS, pLS) - 1.0f;
            d = pow(b, 2) - (4 * a * newC);
            
            if (d > 0){
                float t1 = (-b + sqrt(d)) / (2.0f * a); // in Local Space
                float t2 = (-b - sqrt(d)) / (2.0f * a); // in Local Space
                vec3 x1LS = pLS + t1 * vLS;
                vec3 x2LS = pLS + t2 * vLS;
                
                
                // Finally, we transform the hit position, normal, and distance into world coordinates:
                vec4 worldX1 = ellipsoid_E * vec4(x1LS, 1.0f);
                vec4 worldX2 = ellipsoid_E * vec4(x2LS, 1.0f);
                
                vec4 worldN1 = transpose(inverse(ellipsoid_E)) * vec4(x1LS, 0.0f);
                vec4 worldN2 = transpose(inverse(ellipsoid_E)) * vec4(x2LS, 0.0f);
                
                vec3 worldNOne = vec3(worldN1);
                vec3 worldNTwo = vec3(worldN2);
                
                worldNOne = normalize(worldNOne);
                worldNTwo = normalize(worldNTwo);
                
                t1 = length(worldX1 - pw);
                t2 = length(worldX2 - pw);
                
                if (dot(vw, vec3(worldX1 - pw)) < 0){
                    t1 = -t1;
                }
                if (dot(vw, vec3(worldX2 - pw)) < 0){
                    t2 = -t2;
                }
                
                Hit one(worldX1, worldNOne, t1);
                Hit two(worldX2, worldNTwo, t2);
                
                hits.push_back(one);
                hits.push_back(two);
            }
            
		}
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
	}
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	progShape = make_shared<Program>();
	progShape->setShaderNames(RESOURCE_DIR + "shape_vert.glsl", RESOURCE_DIR + "shape_frag.glsl");
	progShape->setVerbose(true);
	progShape->init();
	progShape->addAttribute("aPos");
	progShape->addAttribute("aNor");
	progShape->addUniform("P");
	progShape->addUniform("MV");
	progShape->addUniform("MVit");
	progShape->setVerbose(false);

	progSimple = make_shared<Program>();
	progSimple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	progSimple->setVerbose(true);
	progSimple->init();
	progSimple->addAttribute("aPos");
	progSimple->addAttribute("aNor");
	progSimple->addUniform("P");
	progSimple->addUniform("MV");
	progSimple->setVerbose(false);
	
	camera = make_shared<Camera>();
	camera->setInitDistance(5.0f);
	
	// Ray: position and direction
	ray[0] = vec3(0.0f, 0.0f, 5.0f);
	ray[1] = vec3(0.0f, 0.0f, -1.0f);
	
	shapeSphere = make_shared<Shape>();
	shapeSphere->loadMesh(RESOURCE_DIR + "sphere.obj");
	shapeSphere->init();
	
	shapePlane = make_shared<Shape>();
	shapePlane->loadMesh(RESOURCE_DIR + "square.obj");
	shapePlane->init();
	
	// Plane parameters
	plane_n = vec3(0.0f, 0.0f, 1.0f);
	plane_c = vec3(0.0f, 0.0f, -1.0f);
	
	// Sphere parameters
	sphere_S = 0.7;
	sphere_T = vec3(0.8f, -0.6f, 0.2f);
	
	// Ellipsoid parameters
	auto M = make_shared<MatrixStack>();
	M->translate(0.5f, 1.0f, 0.0f);
	vec3 axis = normalize(vec3(1.0f, 1.0f, 1.0f));
	M->rotate(1.0f, axis);
	M->scale(3.0f, 1.0f, 0.5f);
	ellipsoid_E = M->topMatrix();
	//cout << to_string(ellipsoid_E) << endl;
	
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
	
	// Reset point and line sizes
	glPointSize(1.0f);
	glLineWidth(1.0f);
	
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
	
	// Bind program for shapes
	progShape->bind();
	glUniformMatrix4fv(progShape->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	
	// Draw plane (assume no rotation is needed)
	MV->pushMatrix();
	MV->translate(plane_c);
	MV->scale(5.0f);
	glUniformMatrix4fv(progShape->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(progShape->getUniform("MVit"), 1, GL_FALSE, value_ptr(inverse(transpose(MV->topMatrix()))));
	shapePlane->draw(progShape);
	MV->popMatrix();
	
	// Draw 1st sphere
	MV->pushMatrix();
	glUniformMatrix4fv(progShape->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(progShape->getUniform("MVit"), 1, GL_FALSE, value_ptr(inverse(transpose(MV->topMatrix()))));
	shapeSphere->draw(progShape);
	MV->popMatrix();
	
	// Draw 2nd sphere
	MV->pushMatrix();
	MV->translate(sphere_T);
	MV->scale(sphere_S);
	glUniformMatrix4fv(progShape->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(progShape->getUniform("MVit"), 1, GL_FALSE, value_ptr(inverse(transpose(MV->topMatrix()))));
	shapeSphere->draw(progShape);
	MV->popMatrix();
	
	// Draw ellipsoid
	MV->pushMatrix();
	MV->multMatrix(ellipsoid_E);
	glUniformMatrix4fv(progShape->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glUniformMatrix4fv(progShape->getUniform("MVit"), 1, GL_FALSE, value_ptr(inverse(transpose(MV->topMatrix()))));
	shapeSphere->draw(progShape);
	MV->popMatrix();
	
	// Unbind program for shapes
	progShape->unbind();
	
	// Bind program for ray and hits
	progSimple->bind();
	
	// Draw ray
	glUniformMatrix4fv(progSimple->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
	glUniformMatrix4fv(progSimple->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	glVertex3fv(&ray[0][0]);
	vec3 ray1 = ray[0] + 100.0f * ray[1];
	glVertex3fv(&ray1[0]);
	glEnd();
	
	// Draw hits
	glPointSize(10.0f);
	glLineWidth(2.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	for(auto hit : hits) {
		glBegin(GL_POINTS);
		glVertex3fv(&hit.x[0]);
		glEnd();
		glBegin(GL_LINES);
		glVertex3fv(&hit.x[0]);
		vec3 x1 = hit.x + 0.5f*hit.n;
		glVertex3fv(&x1[0]);
		glEnd();
	}
	
	// Unbind program for ray and hits
	progSimple->unbind();
	
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
