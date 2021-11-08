// 3DT.cpp
// (c) Justin Thoreson
// 8 October 2021

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <VecMat.h>
#include "GLXtras.h"

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// 3D Letter T
float points[][3] = {
    // Near
    {.125f, .5f, -1}, {-.125f, .5f, -1}, {.875f, .5f, -1},
    {-.875f, .5f, -1}, {.875f, .75f, -1}, {-.875f, .75f, -1},
    {-.125f, -.5f, -1}, {.125f, -.75f, -1},

    // Far
    {.125f, .5f, 1}, {-.125f, .5f, 1}, {.875f, .5f, 1},
    {-.875f, .5f, 1}, {.875f, .75f, 1}, {-.875f, .75f, 1},
    {-.125f, -.5f, 1}, {.125f, -.75f, 1},
};

int triangles[][3] = {
    // Near
    {0, 1, 4}, {0, 2, 4}, {0, 1, 7}, {1, 3, 5},
    {1, 4, 5}, {1, 6, 7},

    // Top
    {4, 5, 13}, {4, 12, 13},

    // Sides
    {3, 5, 13}, {3, 11, 13}, {2, 4, 12}, {2, 10, 12},
    {0, 7, 8}, {7, 8, 15}, {1, 6, 9}, {6, 9, 14},

    // Bottoms
    {1, 3, 11}, {1, 9, 11}, {0, 2, 10}, {0, 8, 10},
    {6, 7, 14}, {7, 14, 15},

    // Far
    {8, 9, 12}, {8, 10, 12}, {8, 9, 15}, {9, 11, 13},
    {9, 12, 13}, {9, 14, 15},
};

float colors[][3] = { 
    // Near
    {1, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 0}, 
    {1, 0, 1}, {0, 0, 0}, {1, 1, 0}, {1, 1, 0},

    // Far
    {1, 1, 0}, {1, 1, 0}, {0, 0, 0}, {1, 0, 1}, 
    {0, 0, 0}, {1, 0, 1}, {1, 0, 0}, {1, 0, 0}
};

float fieldOfView = 30, cubeSize = .5f, cubeStretch = cubeSize;

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

// Interaction

// Rotate 3D letter, mouse click rotational continuity
vec2 mouseDown(0, 0);                   // Location of last mouse down
vec2 rotOld(0, 0), rotNew(0, 0);        // .x is rotation about Y-axis in degress, .y about X-axis
float rotZ = 0;
static float rotSpeed = .3f;

// Move (translate) letter
vec3 tranOld(0, 0, 0), tranNew(0, 0, -1);
static float tranSpeed = .0025f;

// Scale letter
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
        case 'S':
            cubeStretch *= shift ? .9f : 1.1f;
            cubeStretch = cubeStretch < .02f ? .02f : cubeStretch;
            break;
        }
    }
}

// Application

void Display(GLFWwindow *w) {
    // clear background
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // access GPU vertex buffer
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // associate position input to shader with position array in vertex buffer
    VertexAttribPointer(program, "point", 3, 0, (void*) 0);
    // associate color input to shader with color array in vertex buffer
    VertexAttribPointer(program, "color", 3, 0, (void*) sizeof(points));
    // Get screen size
    int screenWidth, screenHeight;
    glfwGetWindowSize(w, &screenWidth, &screenHeight);
    // Compute aspect ratio
    float halfWidth = screenWidth / 2;
    float aspectRatio = (float)halfWidth / (float)screenHeight;
    // Update view transformation
    float nearDist = .001f, farDist = 500;
    mat4 persp = Perspective(fieldOfView, aspectRatio, nearDist, farDist);
    mat4 scale = Scale(cubeSize, cubeSize, cubeStretch);
    mat4 rot = RotateY(rotNew.x) * RotateX(rotNew.y);
    mat4 tran = Translate(tranNew);
    mat4 modelView = tran * rot * scale;
    mat4 view = persp * modelView;
    // Draw solid cube elements
    SetUniform(program, "view", view);
    glViewport(0, 0, halfWidth, screenHeight);
    int nVertices = sizeof(triangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    // Draw outline cube elements
    glViewport(halfWidth, 0, halfWidth, screenHeight);
    glLineWidth(5);
    for (int i = 0; i < 28; i++)
        glDrawElements(GL_LINE_LOOP, 3, GL_UNSIGNED_INT, &triangles[i]);
}

void ErrorGFLW(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
}

void Close() {
    // unbind vertex buffer and free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
}

const char* credit = "\
	Created by Justin Thoreson\n\
-------------------------------------------------------\n\
";

const char* usage = "\n\
                S & SHIFT + S: stretch/shrink T\n\
                F & SHIFT + F: change field of view\n\
            LEFT-CLICK + DRAG: rotate view\n\
    SHIFT + LEFT-CLICK + DRAG: move T\n\
                       SCROLL: resize T/zoom in and out\n\
";

int main() {
    glfwSetErrorCallback(ErrorGFLW);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_SAMPLES, 4); // Anti-alias
    GLFWwindow *window = glfwCreateWindow(750, 750, "3DT", NULL, NULL);
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
    glfwSetKeyCallback(window, Keyboard);
    glfwSwapInterval(1); // ensure no generated frame backlog
    // event loop
    while (!glfwWindowShouldClose(window)) {
        Display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    Close();
    glfwDestroyWindow(window);
    glfwTerminate();
}
