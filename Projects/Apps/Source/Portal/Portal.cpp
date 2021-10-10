// Portal.cpp
// (c) Justin Thoreson
// 10 October 2021

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <VecMat.h>
#include "Camera.h"
#include "GLXtras.h"

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

// Camera
const int SCREEN_SIZE = 750;
float fieldOfView = 30;
Camera camera(SCREEN_SIZE, SCREEN_SIZE, vec3(0, 0, 0), vec3(0, 0, -1), fieldOfView);

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

// Interactions and transformations

static float scalar = .3f;

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
        float fieldOfView = camera.GetFOV();
        bool shift = mods & GLFW_MOD_SHIFT;
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(w, GLFW_TRUE);
        fieldOfView += key == 'F' ? shift ? -5 : 5 : 0;
        fieldOfView = fieldOfView < 5 ? 5 : fieldOfView > 150 ? 150 : fieldOfView;
        camera.SetFOV(fieldOfView);
    }
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
    VertexAttribPointer(program, "point", 3, 0, (void*) 0);
    VertexAttribPointer(program, "color", 3, 0, (void*) sizeof(vertices));
    // Update view transformation
    int screenWidth, screenHeight;
    glfwGetWindowSize(w, &screenWidth, &screenHeight);
    float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
    mat4 view = camera.fullview * Scale(.1f);
    // Draw elements
    // Black cubes
    SetUniform(program, "view", view * Translate(3.25f, 0, 0) * Scale(1.25f, 1.f, 1.f));
    SetUniform(program, "uColor", vec4(0, 0, 0, 1));
    glViewport(0, 0, screenWidth, screenHeight);
    int nVertices = sizeof(triangles) / sizeof(int);
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    SetUniform(program, "view", view * Translate(-3.25f, 0, 0) * Scale(1.25f, 1.f, 1.f));
    SetUniform(program, "uColor", vec4(0, 0, 0, 1));
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    // Orange ring 
    const int NUM_MINI_CUBES = 60;
    for (int i = 1; i <= NUM_MINI_CUBES; i++) {
        mat4 miniCubeTran = Translate(-2.0f, 0, 0) * RotateX((float)i * 360 / NUM_MINI_CUBES) * Translate(0, 0, .5f) * Scale(.1f, .05f, .05f);
        SetUniform(program, "view", view * miniCubeTran);
        SetUniform(program, "uColor", vec4(1, 0.5, 0, 1));
        glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    }
    // Blue ring
    for (int i = 1; i <= NUM_MINI_CUBES; i++) {
        mat4 miniCubeTran = Translate(2.0f, 0, 0) * RotateX((float)i * 360 / NUM_MINI_CUBES) * Translate(0, 0, .5f) * Scale(.1f, .05f, .05f);
        SetUniform(program, "view", view * miniCubeTran);
        SetUniform(program, "uColor", vec4(0, 0, 1, 1));
        glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    }
    // Portal cube
    SetUniform(program, "view", view * Translate(-2.f + 2*cos(dt), 0, 0) * RotateZ(30 * dt) * RotateX(45) * RotateY(45) * Scale(.2f));
    SetUniform(program, "uColor", vec4(-1, -1, -1, -1));
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
    SetUniform(program, "view", view * Translate(2.f + 2*cos(dt), 0, 0) * RotateZ(30 * dt) * RotateX(45) * RotateY(45) * Scale(.2f));
    SetUniform(program, "uColor", vec4(-1, -1, -1, -1));
    glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);

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
    GLFWwindow *window = glfwCreateWindow(SCREEN_SIZE, SCREEN_SIZE, "Portal", NULL, NULL);
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
