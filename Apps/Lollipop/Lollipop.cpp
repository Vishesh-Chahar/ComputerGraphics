// Lollipop.cpp
// (c) Justin Thoreson
// 8 October 2021

#include <glad.h>											// GL header file
#include <glfw3.h>											// GL toolkit
#include <stdio.h>											// printf, etc.
#include "GLXtras.h"										// convenience routines

GLuint vBuffer = 0;											// GPU vert buf ID, valid if > 0
GLuint program = 0;											// shader prog ID, valid if > 0

// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
	#version 130
	in vec2 point;											// 2D point from GPU memory
	void main() {
		// REQUIREMENT 1A) transform vertex:
		gl_Position = vec4(point, 0, 1);					// 'built-in' variable
	}
)";

// pixel shader: operations after the rasterizer
const char *pixelShader = R"(
	#version 130
	out vec4 pColor;

	int Radius(int x, int y) {
		return int(sqrt(pow(x-200, 2) + pow(y-200, 2)));   // Calculate radius
	}

	void main() {
		// REQUIREMENT 1B) shade pixel:
		int row = int(gl_FragCoord.y);					   // Get row
		int col = int(gl_FragCoord.x);					   // Get column
		if (Radius(row, col) % 40 < 10) pColor = vec4(1, 0, 0, 1);
		else if (Radius(row, col) % 40 < 20) pColor = vec4(1, 1, 0, 1);
		else if (Radius(row, col) % 40 < 30) pColor = vec4(0, 1, 1, 1);
		else pColor = vec4(1, 0, 1, 1);
	}
)";

void InitVertexBuffer() {
	// REQUIREMENT 3A) create GPU buffer, copy 4 vertices
#ifdef GL_QUADS
	float pts[][2] = {{-1,-1},{-1,1},{1,1},{1,-1}};			// �object�
#else
	float pts[][2] = {{-1,-1},{-1,1},{1,1},{-1,-1},{1,1},{1,-1}};
#endif
	glGenBuffers(1, &vBuffer);								// ID for GPU buffer
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);					// make it active
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
}

void Display() {
	glUseProgram(program);									// ensure correct program
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);					// activate vertex buffer
	// REQUIREMENT 3B) set vertex feeder
	GLint id = glGetAttribLocation(program, "point");
	glEnableVertexAttribArray(id);
	glVertexAttribPointer(id, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	// in subsequent code the above three lines will be replaced with
	// VertexAttribPointer(program, "point", 2, 0, (void *) 0);
#ifdef GL_QUADS
	glDrawArrays(GL_QUADS, 0, 4);							// display entire window
#else
	glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
	glFlush();												// flush GL ops
}

void GlfwError(int id, const char *reason) {
	printf("GFLW error %i: %s\n", id, reason);
	getchar();
}

void APIENTRY GlslError(GLenum source, GLenum type, GLuint id, GLenum severity,
						GLsizei len, const GLchar *msg, const void *data) {
	printf("GLSL Error: %s\n", msg);
	getchar();
}

int AppError(const char *msg) {
	glfwTerminate();
	printf("Error: %s\n", msg);
	getchar();
	return 1;
}

const char* credit = "\n\
	      Created by Justin Thoreson\n\
          Special thanks to Jules Bloomenthal\n\n\
-------------------------------------------------------\n\
";

int main() {												// application entry
	glfwSetErrorCallback(GlfwError);						// init GL framework
	if (!glfwInit())
		return 1;
	// create named window of given size
	GLFWwindow *w = glfwCreateWindow(400, 400, "Colorful lollipop :)", NULL, NULL);
	if (!w)
		return AppError("can't open window");
	glfwMakeContextCurrent(w);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);	// set OpenGL extensions
	printf("Credit:\n%s\n", credit);
	// following line will not compile if glad.h < OpenGLv4.3
	glDebugMessageCallback(GlslError, NULL);
	// REQUIREMENT 2) build shader program
	if (!(program = LinkProgramViaCode(&vertexShader, &pixelShader)))
		return AppError("can't link shader program");
	InitVertexBuffer();										// set GPU vertex memory
	while (!glfwWindowShouldClose(w)) {						// event loop
		Display();
		if (PrintGLErrors())								// test for runtime GL error
			getchar();										// if so, pause
		glfwSwapBuffers(w);									// double-buffer is default
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
