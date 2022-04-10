// Portalv2.0.cpp
// (c) Justin Thoreson
// 29 October 2021

#define _USE_MATH_DEFINES

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <vector>
#include "VecMat.h"
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "time.h"
// For audio
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

/* COMMENTS
Faceted shapes are often more difficult than smooth surfaces to render with today's shader architecture;
typically facets and creases require duplication of vertices along the edge (same position, different normal)
In this program the cube is rendered in one of three ways:
	a) shade according to vertex color
	b) shade according to a flat (black) color
	c) shade according to an illuminated, faceted cube
ShadeCube, below, is perhaps the simplest way to do this
This file is getting big; perhaps good to create a separate file for particles?
*/

// GPU identifiers
GLuint vBuffer = 0;
GLuint cubeProgram = 0;

// Camera
int windowWidth = 750, windowHeight = 750;
float fieldOfView = 30;
Camera camera(windowWidth, windowHeight, vec3(0, 0, 0), vec3(0, 0, -10), fieldOfView);

// Define vertices (cube)
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; // left, right, bottom, top, near, far
float vertices[][3] = {
	{l,b,n}, {l,b,f}, {l,t,n}, {l,t,f}, {r,b,n}, {r,b,f}, {r,t,n}, {r,t,f}
};
int triangles[][3] = { // ccw order
	{1,2,3}, {0,2,1},  // left face
	{5,7,6}, {4,5,6},  // right
	{1,5,4}, {0,1,4},  // bottom
	{3,6,7}, {2,6,3},  // top
	{2,4,6}, {0,4,2},  // near
	{3,7,5}, {1,3,5}   // far
};
float colors[][3] = {
	{1,0,0}, {1,0,0}, {1,1,0}, {1,1,0}, {1,0,1}, {1,0,1}, {0,1,1}, {0,1,1}
};

// Shaders and vertex buffer

void InitVertexBuffer() {
	// Make GPU buffer for points & colors, set it active buffer
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// Allocate buffer memory to hold vertex locations and colors
	int sPnts = sizeof(vertices), sCols = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPnts + sCols, NULL, GL_STATIC_DRAW);
	// Load data to the GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, vertices);     // Start at beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, sPnts, sCols, colors);   // Start at end of points array, for length of colors array
}

// ShadeCube (JB)

const char *vertexCubeShader = R"(
	#version 130
	in vec3 point, color;
	out vec3 vPoint, vColor;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vColor = color;
	}
)";

const char *pixelCubeShader = R"(
	#version 130
	in vec3 vColor, vPoint;
	out vec4 pColor;
	uniform int useFlatColor = 0, useNormal = 0;
	uniform vec3 flatColor, normal;
	uniform vec3 lights[20];                         // max # lights is 20
	uniform int nlights = 0;
	float Intensity(vec3 normalV, vec3 eyeV, vec3 point, vec3 light) {
		vec3 lightV = normalize(light-point);        // light vector
		vec3 reflectV = reflect(lightV, normalV);    // highlight vector
		float d = max(0, dot(normalV, lightV));      // one-sided diffuse
		float s = max(0, dot(reflectV, eyeV));       // one-sided specular
		return clamp(d+pow(s, 50), 0, 1);
	}
	void main() {
		pColor = vec4(useFlatColor == 1? flatColor : vColor, 1);
		if (useNormal == 1) {
			vec3 N = normalize(normal);              // surface normal
			vec3 E = normalize(vPoint);              // eye vector
			float intensity = 0;
			for (int i = 0; i < nlights; i++)
				intensity += Intensity(N, E, vPoint, lights[i]);
			intensity = clamp(intensity, 0, 1);
			pColor.rgb *= intensity;
		}
	}
)";

void InitShader() {
	cubeProgram = LinkProgramViaCode(&vertexCubeShader, &pixelCubeShader);
	if (!cubeProgram) {
		printf("can't init shader program (type any key to exit)\n");
		getchar();
	}
}

// Particle

const vec2 GRAVITY = vec2(0.0f, -0.00025f);
const float LIFE_DT = 0.0075f;
const float PARTICLE_SIZE = 2.0f;
const float H_VARIANCE = 0.005f;
const int NUM_PARTICLES_SPAWNED = 1;

float rand_float(float min = 0, float max = 1) { return min + (float)rand() / (RAND_MAX / (max - min)); }

struct Particle {
	vec3 pos, vel;
	vec3 baseColor;
	vec3 currentColor; // JB: changed to vec3
	float life;
	Particle() {
		pos = vec3(0.0f), vel = vec3(0.0f), baseColor = currentColor = vec3(1.0f), life = 0.0f;
	}
	void Revive(vec3 new_pos = vec3(0.0f, 0.0f, 0.0f)) {
		life = 1.0f;
		pos = new_pos;
		vel = vec3(rand_float(-H_VARIANCE, H_VARIANCE), rand_float(0.001f, 0.0025f), rand_float(-H_VARIANCE, H_VARIANCE));
	}
	void Run() {
		if (life > 0.0f) {
			life -= LIFE_DT;
			pos += vel;
			//vel += GRAVITY;
			currentColor = vec3(0, 1 - life, 0) + baseColor;
		}
	}
};

int numParticles = 250;
std::vector<Particle> particles = std::vector<Particle>();
int lastUsedParticle = 0;
bool spaceDown = false;

int FindNextParticle() {
	// Search for dead particle from last used particle
	for (int i = lastUsedParticle; i < numParticles; i++) {
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

// Interactions and transformations

static float scalar = .3f;
static bool particlesOn = false, musicOn = false, shaded = true;

bool Shift(GLFWwindow* w) {
	return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
		glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
	// Called when mouse button pressed or released
	if (action == GLFW_PRESS) {
		// Save reference for MouseDrag
		double x, y;
		glfwGetCursorPos(w, &x, &y);
		camera.MouseDown((int)x, (int)y);
	}
	if (action == GLFW_RELEASE) {
		// Save reference rotation
		camera.MouseUp();
	}
}

void MouseMove(GLFWwindow* w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		camera.MouseDrag((int)x, (int)y, Shift(w));
	}
}

void MouseWheel(GLFWwindow* w, double ignore, double spin) {
	camera.MouseWheel(spin > 0, Shift(w));
}

void Keyboard(GLFWwindow* w, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(w, GLFW_TRUE);
		// Field of view
		float fieldOfView = camera.GetFOV();
		bool shift = mods & GLFW_MOD_SHIFT;
		fieldOfView += key == 'F' ? shift ? -5 : 5 : 0;
		fieldOfView = fieldOfView < 5 ? 5 : fieldOfView > 150 ? 150 : fieldOfView;
		camera.SetFOV(fieldOfView);
		// Toggle light, shaded cube
		if (key == GLFW_KEY_L) {
			shaded = !shaded;
			printf("Shading %s\n", shaded ? "enabled" : "disabled");
		}
		// Toggle particles
		if (key == GLFW_KEY_P) {
			particlesOn = !particlesOn;
			printf("Particles %s\n", particlesOn ? "enabled" : "disabled");
		}
		// Toggle music
		if (key == GLFW_KEY_M) {
			if (!musicOn) {
				// Play audio. EXULGOR - Untitled 001
				PlaySound(TEXT("C:/Users/jdtii/ComputerGraphics/Assets/Audio/EXULGOR_Untitled001"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
				printf("Playing: EXULGOR - Untitled 001\n");
				musicOn = !musicOn;

			}
			else {
				// Stop playing music
				PlaySound(0, 0, 0);
				printf("No music is playing\n");
				musicOn = !musicOn;
			}
		}
	}
}

void Resize(GLFWwindow* w, int width, int height) {
	camera.Resize(windowWidth = width, windowHeight = height);
	glViewport(0, 0, windowWidth, windowHeight);
}

// Application

time_t startTime = clock();
static float cubePosition;

void ShadeCube(bool faceted, mat4 m, vec3 color = vec3(1)) {
	SetUniform(cubeProgram, "modelview", m);
	if (!faceted) {
		SetUniform(cubeProgram, "useFlatColor", 1);
		SetUniform(cubeProgram, "useNormal", 0);
		SetUniform(cubeProgram, "flatColor", color);
		int nVertices = sizeof(triangles) / sizeof(int);
		glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	}
	if (faceted) {
		SetUniform(cubeProgram, "useFlatColor", 0);
		SetUniform(cubeProgram, "useNormal", shaded ? 1 : 0);
		for (int i = 0; i < 12; i++) {
			int* t = &triangles[i][0];
			vec3 p[] = { vertices[t[0]], vertices[t[1]], vertices[t[2]] };
			vec3 n = cross(p[1] - p[0], p[2] - p[1]);
			SetUniform(cubeProgram, "normal", n);
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, t);
		}
	}
}

void AnimateDrawParticles(mat4 tran, vec3 color) {
	for (int i = 0; i < numParticles; i++) {
		if (particles[i].life > 0.0f) {
			particles[i].baseColor = color;
			particles[i].Run();
			vec4 res = tran * vec4(particles[i].pos, 1);
			Disk(vec3(res.x, res.y, res.z), 10, particles[i].currentColor);
		}
	}
}

void Display(GLFWwindow *w) {
	// Clear background
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// Init shader program, set vertex pull for points and colors
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glUseProgram(cubeProgram);
	VertexAttribPointer(cubeProgram, "point", 3, 0, (void*) 0);
	VertexAttribPointer(cubeProgram, "color", 3, 0, (void*) sizeof(vertices));
	float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
	mat4 persp = camera.persp;
	// Portals
	mat4 m1 = Translate(3.25f, 0, 0) * Scale(1.25f, 1, 1), m2 = Translate(-3.25f, 0, 0) * Scale(1.25f, 1, 1);
	mat4 p1 = Translate(2.f, 0, 0) * Scale(1.25f, 1, 1) * RotateZ(90), p2 = Translate(-2.f, 0, 0) * Scale(1.25f, 1, 1) * RotateZ(-90);
	vec4 e1 = m1 * vec4(-1, 0, 0, 1), e2 = m2 * vec4(1, 0, 0, 1);
	vec3 entrance1(e1.x, e1.y, e1.z), entrance2(e2.x, e2.y, e2.z);   // Portal entrances
	// Transformed portal entrances, determine lights
	//vec4 l1 = camera.modelview*vec4(entrance1, 1), l2 = camera.modelview*vec4(entrance2, 1);
	//vec3 lights[] = { vec3(l1.x, l1.y, l1.z), vec3(l2.x, l2.y, l2.z) };
	const int NUM_LIGHTS = 16;
	vec4 ls[NUM_LIGHTS];
	std::vector<vec3> lights;
	for (int i = 0; i < NUM_LIGHTS / 2; i++)
		ls[i] = camera.modelview * RotateX((float)i * 360 / NUM_LIGHTS) * Translate(0, 0, .5f) * vec4(entrance1, 1);
	for (int i = NUM_LIGHTS / 2; i < NUM_LIGHTS; i++)
		ls[i] = camera.modelview * RotateX((float)i * 360 / NUM_LIGHTS) * Translate(0, 0, .5f) * vec4(entrance2, 1);
	for (int i = 0; i < NUM_LIGHTS; i++)
		lights.push_back(vec3(ls[i].x, ls[i].y, ls[i].z));
	SetUniform(cubeProgram, "persp", persp);
	SetUniform(cubeProgram, "lights", &lights[0]);
	SetUniform(cubeProgram, "nlights", NUM_LIGHTS); // 2);
	// Black cubes
	ShadeCube(false, camera.modelview*m1, vec3(0, 0, 0));
	ShadeCube(false, camera.modelview*m2, vec3(0, 0, 0));
	// Rings
	const int NUM_MINI_CUBES = 60;
	for (int i = 1; i <= NUM_MINI_CUBES; i++) {
		mat4 miniCubeTran = RotateX((float)i * 360 / NUM_MINI_CUBES) * Translate(0, 0, .5f) * Scale(.1f, .05f, .05f);
		ShadeCube(false, camera.modelview*Translate(-2.0f, 0, 0)*miniCubeTran, vec3(1, 0.5, 0));
		ShadeCube(false, camera.modelview*Translate(2.0f, 0, 0)*miniCubeTran, vec3(0, 0, 1));
	}
	// Portal cubes
	mat4 m = RotateX(30 * dt) * RotateX(45) * RotateY(45) * Scale(.2f);
	cubePosition = 2 * cos(dt);
	ShadeCube(true, camera.modelview*Translate(-2+cubePosition, 0, 0)*m);
	ShadeCube(true, camera.modelview*Translate(2+cubePosition, 0, 0)*m);
	// Particles
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(camera.persp*camera.modelview);
	AnimateDrawParticles(p1, vec3(0, 0, 1)); 
	AnimateDrawParticles(p2, vec3(1, 0, 0));
	glFlush();

}
void Close() {
	// Unbind vertex buffer and free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

int main() {
	srand((int) time(NULL));
	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_SAMPLES, 4); // Anti-alias
	GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Portal", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	printf("GL version: %s\n", glGetString(GL_VERSION));
	PrintGLErrors();
	InitShader();
	InitVertexBuffer();
	// Set particles
	for (int i = 0; i < numParticles; i++)
		particles.push_back(Particle());
	// Set callbacks for device interaction
	glfwSetMouseButtonCallback(window, MouseButton);
	glfwSetCursorPosCallback(window, MouseMove);
	glfwSetScrollCallback(window, MouseWheel);
	glfwSetWindowSizeCallback(window, Resize);
	glfwSetKeyCallback(window, Keyboard);
	glfwSwapInterval(1); // Ensure no generated frame backlog
	// Event loop
	while (!glfwWindowShouldClose(window)) {
		Display(window);
		bool thresholdCrossed = cubePosition < 0.5 && cubePosition > -0.5;
		if (thresholdCrossed && particlesOn)
			SpawnParticle(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	Close();
	glfwDestroyWindow(window);
	glfwTerminate();
}
