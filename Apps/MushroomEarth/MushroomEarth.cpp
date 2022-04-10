// MushroomEarth.cpp
// (c) Justin Thoreson
// 21 October 2021
// Earth is not flat

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <VecMat.h>
#include <vector>
#include "GLXtras.h"
#include "Mesh.h"
#include "Camera.h"
#include "Misc.h"

// GPU identifiers
GLuint program = 0, vBuffer = 0, texUnit = 0, texName; 

const char* objFilename = "C:/Users/jdtii/ComputerGraphics/Assets/Objects/Mushroom.obj";
const char* texFilename = "C:/Users/jdtii/ComputerGraphics/Assets/Textures/Earth.jpg";
std::vector<vec3> points;
std::vector<vec3> normals;
std::vector<vec2> textures;
std::vector<int3> triangles;

int winW = 750, winH = 750;
Camera camera((float) winW/winH, vec3(0, 0, 0), vec3(0, 0, -10));

// Shaders 

const char* vertexShader = R"(
    #version 130
    in vec2 uv;
    in vec3 point, normal;
    out vec2 vUv; 
    out vec3 vPoint, vNormal; 
    uniform mat4 modelview, persp; 
    uniform int freq = 1;
    void main() { 
        vPoint = (modelview*vec4(point, 1)).xyz; 
        vNormal = (modelview*vec4(normal, 0)).xyz; 
        gl_Position = persp*vec4(vPoint, 1);
        vUv = freq * uv;
    } 
)";

const char *pixelShader = R"(
    #version 130
    in vec2 vUv;
    in vec3 vPoint, vNormal;
    out vec4 pColor;
    uniform sampler2D texImage;
    uniform vec3 lightPos = vec3(1,0,0);     // light location
    uniform vec3 color;                      // default color
    uniform float amb = .05;                 // ambient coeff
    uniform float dif = .7;                  // diffuse coeff
    uniform float spc = .5;                  // specular coeff
    void main() {
        vec3 N = normalize(vNormal);         // surface normal
        vec3 L = normalize(lightPos-vPoint); // light vector
        vec3 E = normalize(vPoint);          // eye vector
        vec3 R = reflect(L, N);              // highlight vector
        float d = dif*max(0, dot(N, L));     // one-sided Lambert
        float h = max(0, dot(R, E));         // highlight term
        float s = spc*pow(h, 100);           // specular term
        float inten = clamp(amb+d+s, 0, 1);  // intensity
        //pColor = vec4(inten*color, 1);     // pixel shade
        vec3 tColor = texture(texImage, vUv).rgb; 
        pColor = color[0] == -1 ? vec4(inten*tColor,1) : vec4(inten*color,1);
    }
)";

void InitVertexBuffer() {
    // make GPU buffer for points & colors, set it active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate buffer memory to hold vertex locations and colors
    int sPnts = points.size() * sizeof(vec3), sTex = textures.size() * sizeof(vec2);
    glBufferData(GL_ARRAY_BUFFER, 2*sPnts+sTex, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, &points[0]);
    glBufferSubData(GL_ARRAY_BUFFER, sPnts, sPnts, &normals[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 2*sPnts, sTex, &textures[0]);
}

bool InitShader() {
    program = LinkProgramViaCode(&vertexShader, &pixelShader);
    if (!program)
        printf("can't init shader program\n");
    return program != 0;
}

// Interaction & transformations
static float scalar = .3f;

bool Shift(GLFWwindow* w) {
    return glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) ||
           glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT);
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

void Resize(GLFWwindow* w, int width, int height) {
    camera.Resize(winW = width, winH = height);
    glViewport(0, 0, winW, winH);
}

// Application

time_t startTime = clock();

void Display(GLFWwindow* w) {
    // Clear background
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Access GPU vertex buffer
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(GL_TEXTURE_2D, texName);
    // Associate position input to shader with position array in vertex buffer
    VertexAttribPointer(program, "point", 3, 0, (void*) 0);
    VertexAttribPointer(program, "normal", 3, 0, (void*) (points.size()*sizeof(vec3)));
    VertexAttribPointer(program, "uv", 2, 0, (void*) (2*points.size()*sizeof(vec3)));
    // Draw triangles using indexed vertices
    float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
    // Center mushroom w/ Earth texture, frequency is 1
    mat4 m = RotateY(10 * dt);
    SetUniform(program, "color", vec3(-1));
    SetUniform(program, "texImage", (int) texUnit);
    SetUniform(program, "modelview", camera.modelview * m);
    SetUniform(program, "persp", camera.persp);
    SetUniform(program, "freq", 1);
    glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
    // Orbital mushroom w/ Earth texture, frequency is 4
    m = RotateY(-90 * dt) * RotateZ(-180 -(90*dt)) * Translate(0, 0, 2.5f) * RotateY(90 * dt) * Scale(0.5);
    SetUniform(program, "color", vec3(-1));
    SetUniform(program, "texImage", (int)texUnit);
    SetUniform(program, "modelview", camera.modelview * m);
    SetUniform(program, "persp", camera.persp);
    SetUniform(program, "freq", 4); 
    glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
    // Orbital mushroom w/o texture
    m = RotateY(-90 * dt) * RotateZ(-90 * dt) * Translate(0, 0, -2.5f) * RotateY(90 * dt) * Scale(0.5);
    SetUniform(program, "color", vec3(0,1,0));
    SetUniform(program, "texImage", (int)texUnit);
    SetUniform(program, "modelview", camera.modelview * m);
    SetUniform(program, "persp", camera.persp);
    SetUniform(program, "freq", 1);
    glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
    glFlush();
}

void ErrorGFLW(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
}

void Close() {
    // Unbind vertex buffer and free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
    glDeleteBuffers(1, &texName);
}

const char* credit = "\
	Created by Justin Thoreson\n\
-------------------------------------------------------\n\
";

const char* usage = "\n\
            LEFT-CLICK + DRAG: rotate view\n\
    SHIFT + LEFT-CLICK + DRAG: move objects\n\
                       SCROLL: rotate view\n\
               SHIFT + SCROLL: zoom in and out\n\
";

int main() {
    glfwSetErrorCallback(ErrorGFLW);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_SAMPLES, 4); // Anti-alias
    GLFWwindow *window = glfwCreateWindow(winW, winH, "Mushroom Earth", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    // Read obj file
    if (!ReadAsciiObj((char*)objFilename, points, triangles, &normals, &textures)) {
        printf("Failed to read object file\n");
        getchar();
    }
    printf("%i vertices, %i triangles, %i normals, %i uvs\n", points.size(), triangles.size(), normals.size(), textures.size());
    Normalize(points, .8f);
    printf("GL version: %s\n", glGetString(GL_VERSION));
    PrintGLErrors();
    if (!InitShader())
        return 0;
    InitVertexBuffer();
    // Set callbacks for device interaction
    glfwSetMouseButtonCallback(window, MouseButton);
    glfwSetCursorPosCallback(window, MouseMove);
    glfwSetScrollCallback(window, MouseWheel);
    glfwSetWindowSizeCallback(window, Resize);
    printf("\n%s\n", credit);
    printf("Usage:\n%s\n", usage);
    glfwSwapInterval(1); // Ensure no generated frame backlog
    // Init texture map
    texName = LoadTexture((char*)texFilename, texUnit);
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
