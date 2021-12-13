// PortalIllusion.cpp (Portalv4.0)
// (c) Justin Thoreson
// 11 December 2021

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
#include "Misc.h"
// Multimedia for audio
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// GPU Identifiers
GLuint vBuffer = 0, cubeProgram = 0;
GLuint heartFireTexUnit = 0, companionCubeTexUnit = 1, heartFireTexName, companionCubeTexName;

// Textures
const char* heartFireTexFileName = "C:/Users/jdtii/ComputerGraphics/Assets/Textures/HeartFire.jpg";
const char* companionCubeTexFileName = "C:/Users/jdtii/ComputerGraphics/Assets/Textures/CompanionCube.png";

// Camera
int windowWidth = 750, windowHeight = 750;
float fieldOfView = 40;
Camera camera(windowWidth, windowHeight, vec3(0, 0, 0), vec3(0, 0, -10), fieldOfView);

// Cube Vertices
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; // left, right, bottom, top, near, far
vec3 vertices[] = { {l,b,n}, {l,b,f}, {l,t,n}, {l,t,f}, {r,b,n}, {r,b,f}, {r,t,n}, {r,t,f} };
vec3 colors[] = { {1,0,0}, {1,0,0}, {1,1,0}, {1,1,0}, {1,0,1}, {1,0,1}, {0,1,1}, {0,1,1} };
vec3 normals[] = { {-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1} };
vec2 texs[] = { {0,0}, {1,0}, {1,1}, {0,1} };

// Cube Faces
int quads[][4] = {	// ccw order
	{ 0, 2, 3, 1 },	// left face
	{ 4, 5, 7, 6 },	// right
	{ 0, 1, 5, 4 },	// bottom
	{ 2, 6, 7, 3 },	// top
	{ 0, 4, 6, 2 },	// near
	{ 1, 3, 7, 5 }	// far
};

// Particles
const float H_VARIANCE = 0.005f;
const float LIFE_DT = 0.0075f;
float rand_float(float min = 0, float max = 1) { return min + (float)rand() / (RAND_MAX / (max - min)); }

struct Particle {
	vec3 pos, vel;
	vec3 baseColor;
	vec3 currentColor;
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
			currentColor = vec3(0, 1 - life, 0) + baseColor;
		}
	}
};
int numParticles = 250;
std::vector<Particle> particles = std::vector<Particle>();

// Interaction

time_t startTime = clock();
static float cubePosition;
static float scalar = .3f;
static bool particlesOn = false, musicOn = false, shaded = true, companionCubeTextured = false, oscillate = false;

// Initialization

void InitVertexBuffer() {
	// Set duplicated points
	vec3 pnts[24], nrms[24], cols[24];
	vec2 uvs[24];
	for (int f = 0; f < 6; f++) {	    // Each face of cube 
		for (int k = 0; k < 4; k++) {	// Each corner of face
			int vid = quads[f][k], count = 4 * f + k;
			pnts[count] = vertices[vid];
			cols[count] = colors[vid];
			nrms[count] = normals[f];
			uvs[count] = texs[k];
		}
	}
	// Make GPU buffer, set it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// Allocate buffer memory to hold vertex locations and colors
	int sPnts = sizeof(pnts), sCols = sizeof(cols), sNrms = sizeof(nrms), sUvs = sizeof(uvs);
	glBufferData(GL_ARRAY_BUFFER, sPnts+sCols+sNrms+sUvs, NULL, GL_STATIC_DRAW);
	// Load data to the GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, pnts);
	glBufferSubData(GL_ARRAY_BUFFER, sPnts, sCols, cols);
	glBufferSubData(GL_ARRAY_BUFFER, sPnts+sCols, sNrms, nrms);
	glBufferSubData(GL_ARRAY_BUFFER, sPnts+sCols+sNrms, sUvs, uvs);
}

// Shaders

const char *vertexCubeShader = R"(
	#version 130
	in vec3 point, color, normal;
	in vec2 uv;
	out vec3 vPoint, vColor, vNormal;
	out vec2 vUv;
	uniform mat4 modelview, persp;
	void main() {
		vPoint = (modelview*vec4(point, 1)).xyz;
		vNormal = (modelview*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vColor = color;
		vUv = uv;
	}
)";

const char *pixelCubeShader = R"(
	#version 130
	in vec2 vUv;
	in vec3 vPoint, vColor, vNormal;
	out vec4 pColor;
	uniform sampler2D textureImage;
	uniform bool useFlatColor = false, useTexture = false, useNormal = false, useUnifNorm = false; 
	uniform vec3 flatColor;
	uniform vec3 unifNorm;							 // normal set by uniform	
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
		vec3 texColor = texture(textureImage, vUv).rgb;				
		vec3 useColor = useTexture ? texColor : vColor;				
		pColor = vec4(useFlatColor ? flatColor : useColor, 1);
		if (useNormal) {
			vec3 N = normalize(useUnifNorm ? unifNorm : vNormal);             // surface normal
			vec3 E = normalize(vPoint);              // eye vector
			float intensity = 0;
			for (int i = 0; i < nlights; i++)
				intensity += Intensity(N, E, vPoint, lights[i]);
			intensity = clamp(intensity, 0, 1);
			pColor.rgb *= intensity;
		}
	}
)";

// Display

void ActivateTextures() {
	glActiveTexture(GL_TEXTURE0 + heartFireTexUnit);
	glBindTexture(GL_TEXTURE_2D, heartFireTexName);
	glActiveTexture(GL_TEXTURE0 + companionCubeTexUnit);
	glBindTexture(GL_TEXTURE_2D, companionCubeTexName);
}

void AccessAttributes() {
	int sVrts = 24 * sizeof(vec3);
	VertexAttribPointer(cubeProgram, "point", 3, 0, (void*)0);
	VertexAttribPointer(cubeProgram, "color", 3, 0, (void*)sVrts);
	VertexAttribPointer(cubeProgram, "normal", 3, 0, (void*)(2 * sVrts));
	VertexAttribPointer(cubeProgram, "uv", 2, 0, (void*)(3 * sVrts));
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

void ComputeNormals() {
	for (int i = 0; i < 6; i++) {
		int* q = &quads[i][0];
		vec3 p[] = { vertices[q[0]], vertices[q[1]], vertices[q[2]] };
		vec3 n = cross(p[1] - p[0], p[2] - p[1]);
		SetUniform(cubeProgram, "unifNorm", n);
	}
}

void ShadeCube(bool faceted, bool textured, mat4 m, vec3 color = vec3(1)) {
	SetUniform(cubeProgram, "modelview", m);
	SetUniform(cubeProgram, "useTexture", textured);
	if (!faceted) {
		SetUniform(cubeProgram, "useFlatColor", true);
		SetUniform(cubeProgram, "useNormal", false);
		SetUniform(cubeProgram, "flatColor", color);
	}
	if (faceted || textured) {
		SetUniform(cubeProgram, "useFlatColor", false);
		SetUniform(cubeProgram, "useUnifNorm", true);
	}
	if (faceted) {
		SetUniform(cubeProgram, "useNormal", shaded);
		ComputeNormals();
	}
	glDrawArrays(GL_QUADS, 0, 24);
}

void Display(GLFWwindow *w) {
	// Clear background
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// Init shader program, set vertex pull for points and colors
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer); 
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(cubeProgram);
	ActivateTextures();
	AccessAttributes();
	// Portal positions
	float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
	mat4 persp = camera.persp;
	mat4 m1 = Translate(3.25f, 0, 0) * Scale(1.25f, 1.5, 1), m2 = Translate(-3.25f, 0, 0) * Scale(1.25f, 1.5, 1);
	mat4 m3 = Translate(0, (2.5f + 0.25f * cos(1.5f * dt)), -3.f) * RotateY(180) * Scale(1.5f, 1.5f, 0.005f);
	mat4 m4 = RotateX(30 * dt) * RotateX(45) * RotateY(45) * Scale(.2f);
	mat4 mOsc1 = Translate(0, cos(dt), 0), mOsc2 = Translate(0, -cos(dt), 0); // Portal oscillations
	mat4 p1 = Translate(2.f, 0, 0) * Scale(1.25f, 1, 1) * RotateZ(90), p2 = Translate(-2.f, 0, 0) * Scale(1.25f, 1, 1) * RotateZ(-90);
	// Transform portal entrances, determine lights
	vec4 e1 = m1 * vec4(-1, 0, 0, 1), e2 = m2 * vec4(1, 0, 0, 1);
	vec3 entrance1(e1.x, e1.y, e1.z), entrance2(e2.x, e2.y, e2.z);   // Portal entrances
	vec4 l1 = camera.modelview*vec4(entrance1, 1), l2 = camera.modelview*vec4(entrance2, 1);
	vec3 lights[] = { vec3(l1.x, l1.y, l1.z), vec3(l2.x, l2.y, l2.z) };
	const int NUM_LIGHTS = sizeof(lights) / sizeof(vec3);
	// Start rendering objects
	SetUniform(cubeProgram, "persp", persp);
	SetUniform(cubeProgram, "lights", &lights[0]);
	SetUniform(cubeProgram, "nlights", NUM_LIGHTS);
	// Black cubes
	ShadeCube(false, false, camera.modelview * (oscillate ? mOsc1 : mat4(1.f)) * m1, vec3(0, 0, 0));
	ShadeCube(false, false, camera.modelview * (oscillate ? mOsc2 : mat4(1.f)) * m2, vec3(0, 0, 0));
	// Textured album art cube
	if (musicOn) {
		SetUniform(cubeProgram, "textureImage", (int)heartFireTexUnit);
		ShadeCube(false, true, camera.modelview * m3);
	}
	// Portal entrances (rings)
	const int NUM_MINI_CUBES = 60;
	for (int i = 1; i <= NUM_MINI_CUBES; i++) {
		mat4 miniCubeTran = RotateX((float)i * 360 / NUM_MINI_CUBES) * Translate(0, 0, .5f) * Scale(.1f, .05f, .05f);
		ShadeCube(false, false, camera.modelview * (oscillate ? mOsc1 : mat4(1.f)) * Translate(2.0f, 0, 0) * miniCubeTran, vec3(0, 0, 1));
		ShadeCube(false, false, camera.modelview * (oscillate ? mOsc2 : mat4(1.f)) * Translate(-2.0f, 0, 0) * miniCubeTran, vec3(1, 0.5, 0));
	}
	// Portal cubes
	cubePosition = 2 * cos(dt);
	SetUniform(cubeProgram, "textureImage", (int)companionCubeTexUnit);
	ShadeCube(true, companionCubeTextured, camera.modelview * Translate(-2 + cubePosition, 0, 0) * m4);
	ShadeCube(true, companionCubeTextured, camera.modelview * Translate(2 + cubePosition, 0, 0) * m4);
	// Particles
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(camera.persp*camera.modelview);
	AnimateDrawParticles(p1, vec3(0, 0, 1)); 
	AnimateDrawParticles(p2, vec3(1, 0, 0));
	glFlush();
}

// Particle

const vec2 GRAVITY = vec2(0.0f, -0.00025f);
const float PARTICLE_SIZE = 2.0f;
const int NUM_PARTICLES_SPAWNED = 1;

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

// User Callbacks

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
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		camera.MouseDrag((int)x, (int)y, Shift());
}

void MouseWheel(GLFWwindow* w, double ignore, double spin) {
	camera.MouseWheel(spin > 0, Shift());
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
				PlaySound(0, 0, 0);
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
		// Toggle texture
		if (key == GLFW_KEY_T) {
			companionCubeTextured = !companionCubeTextured;
			printf("Texture %s\n", companionCubeTextured ? "enabled" : "disabled");
		}
		// Toggle portal oscillation
		if (key == GLFW_KEY_O) {
			oscillate = !oscillate;
			printf("Oscillation %s\n", oscillate ? "enabled" : "disabled");
		}
	}
}

// Credit & Usage

const char* credit = "\n\
	      Created by Justin Thoreson\n\
 Special thanks to Jules Bloomenthal & Devon McKee\n\n\
-------------------------------------------------------\n\
";

const char* usage = "\n\
            LEFT-CLICK + DRAG: adjust view\n\
    SHIFT + LEFT-CLICK + DRAG: move objects\n\
                       SCROLL: rotate view\n\
               SHIFT + SCROLL: zoom in and out\n\
               F or SHIFT + F: adjust field of view\n\
                            L: toggle lighting/shading\n\
                            P: toggle particles\n\
                            M: toggle music\n\
                            T: toggle texture\n\
                            O: toggle oscillation\n\n\
-------------------------------------------------------\n\
";

// Application

void PrintProgramInfo() {
	printf("GL version: %s\n\n", glGetString(GL_VERSION));
	printf("Credit:\n%s\n", credit);
	printf("Usage:\n%s\n", usage);
	printf("Toggles:\n\n");
}

void Resize(GLFWwindow* w, int width, int height) {
	camera.Resize(windowWidth = width, windowHeight = height);
	glViewport(0, 0, windowWidth, windowHeight);
}

void InitCallbacks(GLFWwindow* w) {
	glfwSetMouseButtonCallback(w, MouseButton);
	glfwSetCursorPosCallback(w, MouseMove);
	glfwSetScrollCallback(w, MouseWheel);
	glfwSetWindowSizeCallback(w, Resize);
	glfwSetKeyCallback(w, Keyboard);
}

void InitTextures() {
	heartFireTexName = LoadTexture((char*)heartFireTexFileName, heartFireTexUnit);
	companionCubeTexName = LoadTexture((char*)companionCubeTexFileName, companionCubeTexUnit);
}

void InitParticles() {
	for (int i = 0; i < numParticles; i++)
		particles.push_back(Particle());
}

void EmitParticles(GLFWwindow* w) {
	bool thresholdCrossed = cubePosition < 0.5 && cubePosition > -0.5;
	if (thresholdCrossed && particlesOn)
		SpawnParticle(w);
}

void Close() {
	// Unbind vertex buffer and free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glDeleteBuffers(1, &heartFireTexName);
}

int main() {
	srand((int) time(NULL));
	// Init GLFW library and create window
	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_SAMPLES, 4); // Anti-alias
	GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Portal Illusion", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	PrintProgramInfo();
	PrintGLErrors();
	// Link shader program
	cubeProgram = LinkProgramViaCode(&vertexCubeShader, &pixelCubeShader);
	if (!cubeProgram)
		printf("can't init shader program\n");
	InitVertexBuffer();
	InitParticles();       // Set particles
	InitCallbacks(window); // Set callbacks for device interaction
	glfwSwapInterval(1);   // Ensure no generated frame backlog
	InitTextures();        // Set textures
	// Event loop
	while (!glfwWindowShouldClose(window)) {
		Display(window);
		EmitParticles(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	// Terminate program, clean up
	Close();
	glfwDestroyWindow(window);
	glfwTerminate();
}
