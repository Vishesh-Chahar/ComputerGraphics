// Portalv1.0.cpp
// (c) Justin Thoreson
// 29 October 2021

#define _USE_MATH_DEFINES

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <vector>
#include "VecMat.h"
#include "Camera.h"
#include "GLXtras.h"
// For audio
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// Camera
int windowWidth = 800, windowHeight = 800;
float fieldOfView = 30;
Camera camera(windowWidth, windowHeight, vec3(0, 0, 0), vec3(0, 0, -1), fieldOfView);

// Define vertices (cube)
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; // left, right, bottom, top, near, far
float vertices[][3] = {
    {l,b,n}, {l,b,f}, {l,t,n}, {l,t,f}, {r,b,n}, {r,b,f}, {r,t,n}, {r,t,f}
};
int triangles[][3] = {
    {1,2,3}, {0,1,2}, {5,6,7}, {4,5,6}, {1,4,5}, {0,1,4},
    {3,6,7}, {2,3,6}, {2,4,6}, {0,2,4}, {3,5,7}, {1,3,5}
};
float colors[][3] = {
    {1,0,0}, {1,0,0}, {1,1,0}, {1,1,0}, {1,0,1}, {1,0,1}, {0,1,1}, {0,1,1}
};

// Shaders and vertex buffer

const char* vertexShader = R"(
    #version 130
    in vec3 point;
    in vec3 color;
    out vec4 vColor;
    uniform mat4 modelView;
    uniform mat4 persp;
    void main() {
        gl_Position = persp * modelView * vec4(point, 1);
        vColor = vec4(color, 1);
    }
)";

const char* pixelShader = R"(
    #version 130
    in vec4 vColor;
    out vec4 pColor;
    uniform vec4 uColor;
    void main() {
        // Override color if uniform set else default
        pColor = uColor[0] != -1 ? uColor : vColor; 
    }
)";

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

bool InitShader() {
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program)
        printf("can't init shader program\n");
    return program != 0;
}

// Particle

//const vec2 GRAVITY = vec2(0.0f, -0.00025f);
const float LIFE_DT = 0.0075f;
const float PARTICLE_SIZE = 2.0f;
const float H_VARIANCE = 0.005f;
const int NUM_PARTICLES_SPAWNED = 1;

float rand_float(float min = 0, float max = 1) { return min + (float)rand() / (RAND_MAX / (max - min)); }

struct Particle {
    vec3 pos, vel;
    vec4 baseColor;
    vec4 currentColor;
    float life;
    Particle() {
        pos = vec3(0.0f), vel = vec3(0.0f), baseColor = currentColor = vec4(1.0f), life = 0.0f;
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
            currentColor = vec4(0, 1 - life, 0, 0) + baseColor;
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
static bool particlesOn = false, musicOn = false;

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

void Display(GLFWwindow *w) {
    // Clear background
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // Init shader program, set vertex pull for points and colors
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    VertexAttribPointer(program, "point", 3, 0, (void*) 0);
    VertexAttribPointer(program, "color", 3, 0, (void*) sizeof(vertices));
    // Update view transformation
    int screenWidth, screenHeight;
    glfwGetWindowSize(w, &screenWidth, &screenHeight);
    float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
    mat4 modelView = camera.modelview * Scale(.1f);
    mat4 persp = camera.persp;
    // Draw elements
    // Black cubes
    SetUniform(program, "modelView", modelView * Translate(3.25f, 0, 0) * Scale(1.25f, 1.f, 1.f));
    SetUniform(program, "persp", persp);
    SetUniform(program, "uColor", vec4(0, 0, 0, 1));
    glViewport(0, 0, screenWidth, screenHeight);
    int nVertices = sizeof(triangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    SetUniform(program, "modelView", modelView * Translate(-3.25f, 0, 0) * Scale(1.25f, 1.f, 1.f));
    SetUniform(program, "persp", persp);
    SetUniform(program, "uColor", vec4(0, 0, 0, 1));
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    // Orange ring 
    const int NUM_MINI_CUBES = 60;
    for (int i = 1; i <= NUM_MINI_CUBES; i++) {
        mat4 miniCubeTran = Translate(-2.0f, 0, 0) * RotateX((float)i * 360 / NUM_MINI_CUBES) * Translate(0, 0, .5f) * Scale(.1f, .05f, .05f);
        SetUniform(program, "modelView", modelView * miniCubeTran);
        SetUniform(program, "persp", persp);
        SetUniform(program, "uColor", vec4(1, 0.5, 0, 1));
        glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    }
    // Blue ring
    for (int i = 1; i <= NUM_MINI_CUBES; i++) {
        mat4 miniCubeTran = Translate(2.0f, 0, 0) * RotateX((float)i * 360 / NUM_MINI_CUBES) * Translate(0, 0, .5f) * Scale(.1f, .05f, .05f);
        SetUniform(program, "modelView", modelView * miniCubeTran);
        SetUniform(program, "persp", persp);
        SetUniform(program, "uColor", vec4(0, 0, 1, 1));
        glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    }
    // Blue particles
    mat4 shift = Translate(2.f, 0, 0) * RotateZ(90);
    for (int i = 0; i < numParticles; i++) {
        if (particles[i].life > 0.0f) {
            particles[i].baseColor = vec4(0, 0, 1, 1);
            particles[i].Run();
            mat4 scale = Scale(.025f);
            mat4 trans = Translate(particles[i].pos);
            SetUniform(program, "modelView", modelView * shift * trans * scale);
            SetUniform(program, "persp", persp);
            SetUniform(program, "uColor", particles[i].currentColor);
            glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
        }
    }
    // Orange Particles
    shift = Translate(-2.f, 0, 0) * RotateZ(-90);
    for (int i = 0; i < numParticles; i++) {
        if (particles[i].life > 0.0f) {
            particles[i].baseColor = vec4(1, 0, 0, 1);
            particles[i].Run();
            mat4 scale = Scale(.025f);
            mat4 trans = Translate(particles[i].pos);
            SetUniform(program, "modelView", modelView * shift * trans * scale);
            SetUniform(program, "persp", persp);
            SetUniform(program, "uColor", particles[i].currentColor);
            glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
        }
    }
    // Portal cube
    mat4 portalCube = RotateZ(30 * dt) * RotateX(45) * RotateY(45) * Scale(.2f);
    SetUniform(program, "modelView", modelView * Translate(-2.f + 2 * cos(dt), 0, 0) * portalCube);
    SetUniform(program, "persp", persp);
    SetUniform(program, "uColor", vec4(-1, -1, -1, -1));
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    SetUniform(program, "modelView", modelView * Translate(2.f + 2 * cos(dt), 0, 0) * portalCube);
    SetUniform(program, "persp", persp);
    SetUniform(program, "uColor", vec4(-1, -1, -1, -1));
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glFlush();

}

void ErrorGFLW(int id, const char *reason) {
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
    if (!InitShader())
        return 0;
    InitVertexBuffer();
    // Set particles
    for (int i = 0; i < numParticles; i++) {
        particles.push_back(Particle());
    }
    // Print commands
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
        if (particlesOn)
            SpawnParticle(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    Close();
    glfwDestroyWindow(window);
    glfwTerminate();
}
