// RotatingColorfulLetters.cpp
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
float points[][2] = {
    // J
    {.125f, .5f}, {-.125f, .5f}, {.875f, .5f},
    {-.875f, .5f}, {.875f, .75f}, {-.875f, .75f},
    {-.125f, -.5f}, {.125f, -.75f}, {-.875f, -.5f},
    {-.875f, -.75f},

    //D
    {-.875f, .75f}, {-.875f, -.75f}, {-.625f, .5f},
    {-.625f, -.5f}, {-.375f, .75f}, {-.375f, -.75f},
    {.375f, 0}, {.875f, 0},

    //T
    {.125f, .5f}, {-.125f, .5f}, {.875f, .5f},
    {-.875f, .5f}, {.875f, .75f}, {-.875f, .75f},
    {-.125f, -.5f}, {.125f, -.75f},

    // II
    {-.875f, .75f}, {-.875f, .5f}, {-.875f, -.5f},
    {-.875f, -.75f}, {-.375f, .5f}, {-.375f, -.5f},
    {-.125f, .5f}, {-.125f, -.5f}, {.125f, .5f},
    {.125f, -.5f}, {.375f, .5f}, {.375f, -.5f},
    {.875f, .75f}, {.875f, .5f}, {.875f, -.5f},
    {.875f, -.75f},
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
    in vec2 point;
    in vec3 color;
    out vec4 vColor;
    uniform mat4 view;
    void main() {
        gl_Position = view*vec4(point, 0, 1);
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
    int sPnts = sizeof(points), sCols = sizeof(colors);
    glBufferData(GL_ARRAY_BUFFER, sPnts+sCols, NULL, GL_STATIC_DRAW);
    // Load data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, points);      // Start at beginning of buffer, for length of points array
    glBufferSubData(GL_ARRAY_BUFFER, sPnts, sCols, colors);  // Start at end of points array, for length of colors array
}

bool InitShader() {
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program)
        printf("can't init shader program\n");
    return program != 0;
}

time_t startTime = clock();
static const float INIT_DEG_PER_SEC = 30, INIT_SCALING_RATE = 1;
static float degPerSec = INIT_DEG_PER_SEC, startAngleValue = 0, scalingRate = INIT_SCALING_RATE;

// Interaction

void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Handle angle jump
        time_t currentTime = clock();
        float dt = (float)(currentTime - startTime) / CLOCKS_PER_SEC;
        startAngleValue += dt * degPerSec;
        startTime = currentTime;
        // Keyboard commands
        switch (key) {
        case 'A': degPerSec *= 1.3f; break;                                                 // Increase rotation speed
        case 'S': degPerSec = degPerSec < 0 ? -INIT_DEG_PER_SEC : INIT_DEG_PER_SEC; break;  // Reset rotation
        case 'D': degPerSec *= .7f; break;                                                  // Decrease rotation speed
        case 'R': degPerSec *= -1; break;                                                   // Reverse rotation direction
        case 'I': scalingRate *= 1.2f; break;                                               // Increase scaling rate
        case 'O': scalingRate *= .9f; break;                                                // Decrease scaling rate
        case 'P': scalingRate = INIT_SCALING_RATE; break;                                   // Reset scaling rate;
        }
    }
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// Application

void Display() {
    // Clear background
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    // Access GPU vertex buffer
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // Associate position input to shader with position array in vertex buffer
    VertexAttribPointer(program, "point", 2, 0, (void*)0);
    // Associate color input to shader with color array in vertex buffer
    VertexAttribPointer(program, "color", 3, 0, (void*)sizeof(points));
    // Compute elapsed time, determine radAng, send to GPU
    float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
    mat4 rot = RotateZ(startAngleValue + degPerSec * dt);
    mat4 scale = Scale(.5f * (1 + sin(scalingRate * dt)) / 2);
    mat4 upperLeft = Translate(-.5f, .5f, 0);
    mat4 upperRight = Translate(.5f, .5f, 0);
    mat4 lowerLeft = Translate(-.5f, -.5f, 0);
    mat4 lowerRight = Translate(.5f, -.5f, 0);
    // Render vertices
    // J
    SetUniform(program, "view", upperLeft * rot * scale);
    int nVerticesJ = sizeof(jTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesJ, GL_UNSIGNED_INT, jTriangles);
    // D
    SetUniform(program, "view", upperRight * rot * scale);
    int nVerticesD = sizeof(dTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesD, GL_UNSIGNED_INT, dTriangles);
    // T
    SetUniform(program, "view", lowerLeft * rot * scale);
    int nVerticesT = sizeof(tTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesT, GL_UNSIGNED_INT, tTriangles);
    // II
    SetUniform(program, "view", lowerRight * rot * scale);
    int nVerticesII = sizeof(iiTriangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVerticesII, GL_UNSIGNED_INT, iiTriangles);
    glFlush();
}

void ErrorGFLW(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
}

void Close() {
    // Enbind vertex buffer and free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
}

const char* credit = "\
	Created by Justin Thoreson\n\
-------------------------------------------------------\n\
";

const char* usage = "\n\
    A: increase rotation speed\n\
    S: reset rotation speed\n\
    D: decrease rotation speed\n\
    R: reverse rotation direction\n\
    I: increase scaling rate\n\
    O: decrease scaling rate\n\
    P: reset scaling rate\n\
";

int main() {
    glfwSetErrorCallback(ErrorGFLW);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_SAMPLES, 4); // Anti-alias
    GLFWwindow *w = glfwCreateWindow(750, 750, "JDTII - Rotating Colorful Letters", NULL, NULL);
    if (!w) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    printf("GL version: %s\n", glGetString(GL_VERSION));
    printf("\n%s\n", credit);
    printf("Usage:\n%s\n", usage);
    PrintGLErrors();
    if (!InitShader())
        return 0;
    InitVertexBuffer();
    glfwSetKeyCallback(w, Keyboard);
    glfwSwapInterval(1); // Ensure no generated frame backlog
    // Event loop
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }
    Close();
    glfwDestroyWindow(w);
    glfwTerminate();
}
