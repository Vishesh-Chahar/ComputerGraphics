// LettersOrbitingCube.cpp
// (c) Justin Thoreson
// 9 October 2021

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <VecMat.h>
#include "GLXtras.h"

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// Vertices for the letters "JDTII" and a center cube
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; // left, right, bottom, top, near, far
float vertices[][3] = {
    // J
    {.125f, .5f, 0}, {-.125f, .5f, 0}, {.875f, .5f, 0},
    {-.875f, .5f, 0}, {.875f, .75f, 0}, {-.875f, .75f, 0},
    {-.125f, -.5f, 0}, {.125f, -.75f, 0}, {-.875f, -.5f, 0},
    {-.875f, -.75f, 0},

    //D
    {-.875f, .75f, 0}, {-.875f, -.75f, 0}, {-.625f, .5f, 0},
    {-.625f, -.5f, 0}, {-.375f, .75f, 0}, {-.375f, -.75f, 0},
    {.375f, 0, 0}, {.875f, 0, 0},

    //T
    {.125f, .5f, 0}, {-.125f, .5f, 0}, {.875f, .5f, 0},
    {-.875f, .5f, 0}, {.875f, .75f, 0}, {-.875f, .75f, 0},
    {-.125f, -.5f, 0}, {.125f, -.75f, 0},

    // II
    {-.875f, .75f, 0}, {-.875f, .5f, 0}, {-.875f, -.5f, 0},
    {-.875f, -.75f, 0}, {-.375f, .5f, 0}, {-.375f, -.5f, 0},
    {-.125f, .5f, 0}, {-.125f, -.5f, 0}, {.125f, .5f, 0}, 
    {.125f, -.5f, 0}, {.375f, .5f, 0}, {.375f, -.5f, 0},
    {.875f, .75f, 0}, {.875f, .5f, 0}, {.875f, -.5f, 0},
    {.875f, -.75f, 0},

    // Cube
    {l,b,n}, {l,b,f}, {l,t,n}, {l,t,f}, {r,b,n}, {r,b,f}, {r,t,n}, {r,t,f}
};

int cubeTriangles[][3] = { {43,44,45}, {42,43,44}, {47,48,49}, {46,47,48}, {43,46,47}, {42,43,46},
                       {45,48,49}, {44,45,48}, {44,46,48}, {42,44,46}, {45,47,49}, {43,45,47} };

// Triangles
int jTriangles[][3] = {
    {0, 1, 4}, {0, 2, 4}, {0, 1, 7}, {1, 3, 5},
    {1, 4, 5}, {1, 6, 7}, {6, 7, 9}, {6, 8, 9},
};

int dTriangles[][3] = {
    {10, 11, 13}, {10, 12, 13}, {10, 12, 14}, {11, 13, 15},
    {12, 14, 16}, {13, 15, 16}, {14, 16, 17}, {15, 16, 17}
};

int tTriangles[][3] = {
    {18, 19, 22}, {18, 20, 22}, {18, 19, 25}, {19, 21, 23},
    {19, 22, 23}, {19, 24, 25}
};

int iiTriangles[][3] = {
    {26, 27, 30}, {26, 30, 32}, {26, 32, 38}, {28, 29, 31},
    {29, 31, 33}, {29, 33, 35}, {29, 35, 41}, {30, 31, 33},
    {30, 32, 33}, {32, 34, 38}, {34, 35, 36}, {34, 36, 38},
    {35, 36, 37}, {35, 37, 41}, {36, 38, 39}, {37, 40, 41}
};

// Colors
float colors[][3] = {
    // J
    {1, 0, 0}, {1, 0, 0}, {1, 0, 1},
    {0, 0, 0}, {1, 0, 1}, {0, 0, 0},
    {1, 1, 0}, {1, 1, 0}, {0, 1, 1},
    {0, 1, 1},

    // D
    {0, 0, 0}, {0, 1, 1}, {1, 0, 0},
    {0, 1, 1}, {1, 0, 1}, {0, 1, 0},
    {1, 1, 0}, {1, 1, 0},

    // T
    {1, 0, 0}, {1, 0, 0}, {1, 0, 1},
    {0, 0, 0}, {1, 0, 1}, {0, 0, 0},
    {1, 1, 0}, {1, 1, 0},

    // II
    {0, 0, 0}, {0, 0, 0}, {0, 1, 1},
    {0, 1, 1}, {1, 0, 0}, {0, 1, 1},
    {1, 0, 0}, {1, 1, 0}, {0, 1, 1},
    {1, 1, 0}, {0, 1, 1}, {1, 1, 0},
    {1, 0, 1}, {1, 0, 1}, {1, 1, 0},
    {1, 1, 0},

    // Cube
    {1,0,0}, {1,0,0}, {1,1,0}, {1,1,0},
    {0,1,1}, {0,1,1}, {1,0,1}, {1,0,1}
};

// Shaders 

const char *vertexShader = R"(
    #version 130
    in vec3 point;
    in vec3 color;
    out vec4 vColor;
    uniform mat4 view;
    void main() {
        gl_Position = view*vec4(point, 1);
        vColor = vec4(color, 1);
    }
)";

const char *pixelShader = R"(
    #version 130
    in vec4 vColor;
    out vec4 pColor;
    void main() {
        pColor = vColor;
    }
)";

void InitVertexBuffer() {
    // Make GPU buffer for points & colors, set it active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // Allocate buffer memory to hold vertex locations and colors
    int sPnts = sizeof(vertices), sCols = sizeof(colors);
    glBufferData(GL_ARRAY_BUFFER, sPnts+sCols, NULL, GL_STATIC_DRAW);
    // Load data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, vertices);     // Start at beginning of buffer, for length of points array
    glBufferSubData(GL_ARRAY_BUFFER, sPnts, sCols, colors); // Start at end of points array, for length of colors array
}

bool InitShader() {
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program)
        printf("can't init shader program\n");
    return program != 0;
}

// Interactions and perspective transformation

vec2 mouseDown(0, 0);                       // Location of last mouse down
vec2 rotOld(0, 0), rotNew(0, 0);            // .x is rotation about Y-axis in degress, .y about X-axis
vec3 tranOld(0, 0, 0), tranNew(0, 0, -1);
static float rotZ = 0;
static float rotSpeed = .3f;
static float tranSpeed = .0025f;
static float scalar = .3f;
static float fieldOfView = 30;

void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
    // Called when mouse button pressed or released
    if (action == GLFW_PRESS) {
        // Save reference for MouseDrag
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        mouseDown = vec2((float) x, (float) y);
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

void Keyboard(GLFWwindow* w, int key, int scancode, int action, int mods) {
    bool shift = mods & GLFW_MOD_SHIFT;
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(w, GLFW_TRUE);
            break;
        case 'F':
            fieldOfView += shift ? -5 : 5;
            fieldOfView = fieldOfView < 5 ? 5 : fieldOfView > 150 ? 150 : fieldOfView;
            break;
        }
    }
}

// Application

time_t startTime = clock();

void Display(GLFWwindow *w) {
    // clear background
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // Access GPU vertex buffer
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // Associate position and color input to shader with position and color arrays in vertex buffer
    VertexAttribPointer(program, "point", 3, 0, (void*) 0);
    VertexAttribPointer(program, "color", 3, 0, (void*) sizeof(vertices));
    // Update view transformation
    int screenWidth, screenHeight;
    glfwGetWindowSize(w, &screenWidth, &screenHeight);
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    float nearDist = .001f, farDist = 500;
    float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
    mat4 persp = Perspective(fieldOfView, aspectRatio, nearDist, farDist);
    mat4 scale = Scale(.1f);
    mat4 shiftZ = Translate(0, 0, 2.f);
    mat4 shiftY = Translate(0, 1.5*cos(dt), 0);
    mat4 rotY90 = RotateY(90), rotY180 = RotateY(180), rotY270 = RotateY(270);
    mat4 rotY = RotateY(-30 * dt);
    mat4 rotX = RotateX(60 * dt);
    mat4 rot = RotateY(rotNew.x) * RotateX(rotNew.y);
    mat4 tran = Translate(tranNew);
    mat4 modelView = tran * rot * scale;
    mat4 view = persp * modelView;
    // Draw elements
    // J
    SetUniform(program, "view", view * rotY * shiftZ);
    glViewport(0, 0, screenWidth, screenHeight);
    int nVerticesJ = sizeof(jTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesJ, GL_UNSIGNED_INT, jTriangles);
    // D
    SetUniform(program, "view", view * rotY * rotY90 * shiftZ);
    int nVerticesD = sizeof(dTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesD, GL_UNSIGNED_INT, dTriangles);
    // T
    SetUniform(program, "view", view * rotY * rotY180 * shiftZ);
    int nVerticesT = sizeof(tTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesT, GL_UNSIGNED_INT, tTriangles);
    // II
    SetUniform(program, "view", view * rotY * rotY270 * shiftZ);
    int nVerticesII = sizeof(iiTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesII, GL_UNSIGNED_INT, iiTriangles);
    // Cube
    SetUniform(program, "view", view * shiftY * rotX * rotY * Scale(.75f));
    int nVerticesCube = sizeof(cubeTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesCube, GL_UNSIGNED_INT, cubeTriangles);
    // Ring 1
    const int NUM_MINI_CUBES = 30;
    for (int i = 1; i <= NUM_MINI_CUBES; i++) {
        SetUniform(program, "view", view * RotateX(60 * dt) * RotateZ(45) * RotateX((float)i*360/NUM_MINI_CUBES) * RotateZ(360*dt) * Translate(0, 0, 3.f) * Scale(.15f));
        glDrawElements(GL_TRIANGLES, nVerticesCube, GL_UNSIGNED_INT, cubeTriangles);
    }
    // Ring 2
    for (int i = 1; i <= NUM_MINI_CUBES; i++) {
        SetUniform(program, "view", view * RotateX(-60 * dt) * RotateZ(-45) * RotateX((float)i*360/NUM_MINI_CUBES) * RotateZ(360 * dt) * Translate(0, 0, 3.f) * Scale(.15f));
        glDrawElements(GL_TRIANGLES, nVerticesCube, GL_UNSIGNED_INT, cubeTriangles);
    }
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
    if (!glfwInit())
        return 1;
    GLFWwindow *window = glfwCreateWindow(750, 750, "JDTII - Letters Orbiting Cube", NULL, NULL);
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
    // Set callbacks for device interaction
    glfwSetMouseButtonCallback(window, MouseButton);
    glfwSetCursorPosCallback(window, MouseMove);
    glfwSetScrollCallback(window, MouseWheel);
    glfwSetKeyCallback(window, Keyboard);
    glfwSwapInterval(1); // Ensure no generated frame backlog
    // Event loop
    while (!glfwWindowShouldClose(window)) {
        Display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    Close();
    glfwDestroyWindow(window);
    glfwTerminate();
}
