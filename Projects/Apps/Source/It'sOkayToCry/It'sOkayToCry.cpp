// It'sOkayToCry.cpp 
// (c) Justin Thoreson
// 9 October 2021

#define _USE_MATH_DEFINES

#include <glad.h>
#include <GLFW/glfw3.h>
#include "GLXtras.h"
#include <time.h>
#include <vector>
#include "VecMat.h"

const float SCREEN_SIZE = 800;

GLuint vBuffer = 0;
GLuint program = 0;

// Define vertices (cube)
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; // Left, right, bottom, top, near far;
float vertices[][3] = {
	{l, b, n}, {l, t, n}, {r, b, n}, {r, t, n},
	{l, b, f}, {l, t, f}, {r, b, f}, {r, t, f},
};
int triangles[][3] = {
	{1,2,3}, {0,1,2}, {5,6,7}, {4,5,6}, {1,4,5}, {0,1,4},
	{3,6,7}, {2,3,6}, {2,4,6}, {0,2,4}, {3,5,7}, {1,3,5}
};

// Particle

const vec2 GRAVITY = vec2(0.0f, -0.0025f);
const float LIFE_DT = 0.005f;
const float PARTICLE_SIZE = 2.0f;
const float H_VARIANCE = 0.01f;
const int NUM_PARTICLES_SPAWNED = 5;

float rand_float(float min = 0, float max = 1) { return min + (float)rand() / (RAND_MAX / (max - min)); }

struct Particle {
	vec3 pos, vel;
	vec4 color;
	float life;
	Particle() {
		pos = vec3(0.0f), vel = vec3(0.0f), color = vec4(1.0f), life = 0.0f;
	}
	void Revive(vec3 new_pos = vec3(0.0f, 0.0f, 0.0f)) {
		life = 1.0f;
		pos = new_pos;
		vel = vec3(rand_float(-1 * H_VARIANCE, H_VARIANCE), rand_float(0.025f, 0.05f), rand_float(-1 * H_VARIANCE, H_VARIANCE));
	}
	void Run() {
		if (life > 0.0f) {
			life -= LIFE_DT;
			pos += vel;
			vel = GRAVITY;
			color = vec4(0.0f, life, 1.0f, 0.0f);
		}
	}
};

int num_particles = 500;
std::vector<Particle> particles = std::vector<Particle>();
int lastUsedParticle = 0;
bool spaceDown = false;

int FindNextParticle() {
	// Search for dead particle from last used particle
	for (int i = lastUsedParticle; i < num_particles; i++) {
		if (particles[i].life <= 0.0f) {
			lastUsedParticle = i;
			return i;
		}
	}
	// Search linearly through rest of particles
	for (int i = 0; i < lastUsedParticle; i++) {
		if (particles[i].life <= 0.0f) {
			lastUsedParticle = i;
			return i;
		}
	}
	// If no dead particles exist, simply take the first particle
	lastUsedParticle = 0;
	return 0;
}

void SpawnParticle(GLFWwindow* w) {
	vec3 m = vec3(0, 0, 0);
	for (int i = 0; i < NUM_PARTICLES_SPAWNED; i++) {
		int p = FindNextParticle();
		particles[p].Revive(m);
	}
}

// Shaders

const char* vertexShader = R"(
	#version 130
	in vec3 point;
	uniform mat4 view;
	void main() {
		gl_Position = view * vec4(point, 1);
	}
)";

const char* fragmentShader = R"(
	#version 130
	out vec4 pColor;
	uniform vec4 color;
	void main() {
		pColor = color;
	}
)";

void InitVertexBuffer() {
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

// Interactions and perspective transformations

vec2 mouseDown(0, 0);                   // Location of last mouse down
vec2 rotOld(0, 0), rotNew(0, 0);        // .x is rotation about Y-axis in degress, .y about X-axis
vec3 tranOld(0, 0, 0), tranNew(0, 0, -1);
float rotZ = 0;
static float rotSpeed = .3f;
static float tranSpeed = .0025f;
static float scalar = .3f;
static float fieldOfView = 30;
// Smile or frown
static float cheekPosY = -.3f;
static float mouthPosY = -.5;

void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
	// Called when mouse button pressed or released
	if (action == GLFW_PRESS) {
		// Save reference for MouseDrag
		double x, y;
		glfwGetCursorPos(w, &x, &y);
		mouseDown = vec2((float)x, (float)y);
	}
	if (action == GLFW_RELEASE) {
		// Save reference rotation
		rotOld = rotNew;
		tranOld = tranNew;
	}
}

void MouseMove(GLFWwindow* w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		// Compute mouse drag difference and update rotation
		//vec2 dif((float) x - mouseDown.x, (float) y - mouseDown.y);
		vec2 mouse((float)x, (float)y), dif = mouse - mouseDown;
		bool shift = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) ||
			glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT);
		// Translate or rotate about X-axis, Y-axis
		if (shift)
			tranNew = tranOld + tranSpeed * vec3(dif.x, -dif.y, 0);
		else
			rotNew = rotOld + rotSpeed * dif;
	}
}

void MouseWheel(GLFWwindow* w, double ignore, double spin) {
	tranNew.z += spin > 0 ? -.1f : .1f;
	tranOld = tranNew;
}

void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (key == GLFW_KEY_SPACE) {
		if (action == GLFW_PRESS) {
			spaceDown = true;
			// Frown
			cheekPosY = -.5f;
			mouthPosY = -.3f;
		}
		if (action == GLFW_RELEASE) {
			spaceDown = false;
			// Smile
			cheekPosY = -.3f;
			mouthPosY = -.5f;
		}
	}
}

// Application

void Display(GLFWwindow* w) {
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	VertexAttribPointer(program, "point", 3, 0, (void*)0);
	int screenWidth, screenHeight;
	glfwGetWindowSize(w, &screenWidth, &screenHeight);
	glViewport(0, 0, screenWidth, screenHeight);
	float aspectRatio = (float)screenWidth / (float)screenHeight;
	float nearDist = .001f, farDist = 500;
	mat4 persp = Perspective(fieldOfView, aspectRatio, nearDist, farDist);
	mat4 scale = Scale(.3f);
	mat4 rot = RotateY(rotNew.x) * RotateX(rotNew.y);
	mat4 tran = Translate(tranNew);
	mat4 view = persp * tran * rot * scale;
	// Render eyes
	mat4 left = view * Translate(-.5f, .3f, 0) * Scale(.1f);
	mat4 right = view * Translate(.5f, .3f, 0) * Scale(.1f);
	int nVertices = sizeof(triangles) / sizeof(int);
	SetUniform(program, "view", left);
	SetUniform(program, "color", vec4(1, 0, 0, 1));
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	SetUniform(program, "view", right);
	SetUniform(program, "color", vec4(1, 0, 0, 1));
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	// Render mouth
	mat4 lCheek = view * Translate(-.5f, cheekPosY, 0) * Scale(.1f);
	mat4 rCheek = view * Translate(.5f, cheekPosY, 0) * Scale(.1f);
	mat4 mouth = view * Translate(0, mouthPosY, 0) * Scale(.4f, .1f, .1f);
	SetUniform(program, "view", lCheek);
	SetUniform(program, "color", vec4(1, 0, 0, 1));
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	SetUniform(program, "view", rCheek);
	SetUniform(program, "color", vec4(1, 0, 0, 1));
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	SetUniform(program, "view", mouth);
	SetUniform(program, "color", vec4(1, 0, 0, 1));
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	// Render tears
	const int NUM_ROWS = 6;
	for (int i = -NUM_ROWS / 2; i < NUM_ROWS / 2; i++) {
		mat4 shift = Translate(-.5f, .2f, 0) * Translate(i/35.f, 0, .1f);
		for (int j = 0; j < num_particles; j++) {
			if (particles[j].life > 0.0f) {
				particles[j].Run();
				mat4 scale = Scale(PARTICLE_SIZE / SCREEN_SIZE);
				mat4 trans = Translate(particles[j].pos);
				mat4 m = view * shift * trans * scale;
				SetUniform(program, "view", m);
				SetUniform(program, "color", particles[j].color);
				glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
			}
		}
	}
	for (int i = -NUM_ROWS / 2; i < NUM_ROWS / 2; i++) {
		mat4 shift = Translate(.5f, .2f, 0) * Translate(i / 35.f, 0, .1f);
		for (int j = 0; j < num_particles; j++) {
			if (particles[j].life > 0.0f) {
				particles[j].Run();
				mat4 scale = Scale(PARTICLE_SIZE / SCREEN_SIZE);
				mat4 trans = Translate(particles[j].pos);
				mat4 m = view * shift * trans * scale;
				SetUniform(program, "view", m);
				SetUniform(program, "color", particles[j].color);
				glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
			}
		}
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFlush();
}

void ErrorGFLW(int id, const char* reason) {
	printf("GFLW error %i: %s\n", id, reason);
}

void Close() {
	// Unbind vertex buffer and free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

int main() {
	glfwSetErrorCallback(ErrorGFLW);
	srand(time(NULL));
	if (!glfwInit())
		return 1;
	GLFWwindow* window = glfwCreateWindow(SCREEN_SIZE, SCREEN_SIZE, "It's okay to cry", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwSetWindowPos(window, 100, 100);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	PrintGLErrors();
	if (!(program = LinkProgramViaCode(&vertexShader, &fragmentShader)))
		return 0;
	InitVertexBuffer();
	for (int i = 0; i < num_particles; i++) {
		particles.push_back(Particle());
	}
	// Set callbacks for device interaction
	glfwSetMouseButtonCallback(window, MouseButton);
	glfwSetScrollCallback(window, MouseWheel);
	glfwSetCursorPosCallback(window, MouseMove);
	glfwSetKeyCallback(window, Keyboard);
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window)) {
		Display(window);
		if (spaceDown) {
			SpawnParticle(window);
		}
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	Close();
	glfwDestroyWindow(window);
	glfwTerminate();
}