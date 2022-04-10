// TransformColorfulLetters3D.cpp
// (c) Justin Thoreson
// 7 October 2021

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <VecMat.h>
#include "GLXtras.h"

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// Vertices for the letters "JDTII"
float points[][3] = {
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
};

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
    {1, 1, 0}
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
    // make GPU buffer for points & colors, set it active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate buffer memory to hold vertex locations and colors
    int sPnts = sizeof(points), sCols = sizeof(colors);
    glBufferData(GL_ARRAY_BUFFER, sPnts+sCols, NULL, GL_STATIC_DRAW);
    // load data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, points);
    // start at beginning of buffer, for length of points array
    glBufferSubData(GL_ARRAY_BUFFER, sPnts, sCols, colors);
    // start at end of points array, for length of colors array
}

bool InitShader() {
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program)
        printf("can't init shader program\n");
    return program != 0;
}

// Interaction & transformations

vec2 mouseDown(0, 0);                   // Location of last mouse down
vec2 rotOld(0, 0), rotNew(0, 0);        // .x is rotation about Y-axis in degress, .y about X-axis
vec3 tranOld(0, 0, 0), tranNew(0, 0, 0);
static float rotZ = 0;
static float rotSpeed = .3f;
static float tranSpeed = .0025f;
static float scalar = .3f;

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
    bool shift = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) ||
                 glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT);
    // Scale or rotate about Z-axis
    if (shift)
        scalar += spin * .05f;
    else
        rotZ += spin * 5.f;
}

// Application

void Display() {
    // Clear background
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // Access GPU vertex buffer
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // Associate position input to shader with position array in vertex buffer
    VertexAttribPointer(program, "point", 3, 0, (void*) 0);
    // Associate color input to shader with color array in vertex buffer
    VertexAttribPointer(program, "color", 3, 0, (void*) sizeof(points));
    // Update view transformation
    mat4 rot = RotateY(rotNew.x) * RotateX(rotNew.y) * RotateZ(rotZ);
    mat4 trans = Translate(tranNew);
    mat4 upperLeft = Translate(-.5f, .5f, 0);
    mat4 upperRight = Translate(.5f, .5f, 0);
    mat4 lowerLeft = Translate(-.5f, -.5f, 0);
    mat4 lowerRight = Translate(.5f, -.5f, 0);
    mat4 scale = Scale(scalar);
    // J
    SetUniform(program, "view", trans * upperLeft * scale * rot);
    int nVerticesJ = sizeof(jTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesJ, GL_UNSIGNED_INT, jTriangles);
    // D
    SetUniform(program, "view", trans * upperRight * scale * rot);
    int nVerticesD = sizeof(dTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesD, GL_UNSIGNED_INT, dTriangles);
    // T
    SetUniform(program, "view", trans * lowerLeft * scale * rot);
    int nVerticesT = sizeof(tTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesT, GL_UNSIGNED_INT, tTriangles);
    // II
    SetUniform(program, "view", trans * lowerRight * scale * rot);
    int nVerticesII = sizeof(iiTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesII, GL_UNSIGNED_INT, iiTriangles);
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

const char* credit = "\
	Created by Justin Thoreson\n\
-------------------------------------------------------\n\
";

const char* usage = "\n\
            LEFT-CLICK + DRAG: rotate view\n\
    SHIFT + LEFT-CLICK + DRAG: move letters\n\
                       SCROLL: rotate letters\n\
               SHIFT + SCROLL: resize letters\n\
";

int main() {
    glfwSetErrorCallback(ErrorGFLW);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_SAMPLES, 4); // Anti-alias
    GLFWwindow *window = glfwCreateWindow(750, 750, "JDTII - Transform Colorful Letters 3D", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    printf("GL version: %s\n", glGetString(GL_VERSION));
    printf("\n%s\n", credit);
    printf("Usage:\n%s\n", usage);
    PrintGLErrors();
    if (!InitShader())
        return 0;
    InitVertexBuffer();
    // Set callbacks for device interaction
    glfwSetMouseButtonCallback(window, MouseButton);
    glfwSetCursorPosCallback(window, MouseMove);
    glfwSetScrollCallback(window, MouseWheel);
    glfwSwapInterval(1); // Ensure no generated frame backlog
    // Event loop
    while (!glfwWindowShouldClose(window)) {
        Display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    Close();
    glfwDestroyWindow(window);
    glfwTerminate();
}
