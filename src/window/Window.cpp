#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Window.h"
#include <glm/glm.hpp>

using glm::vec4;

GLFWwindow* Window::window;
std::stack<glm::vec4> Window::scissorStack;
glm::vec4 Window::scissorArea;
int Window::width = 0;
int Window::height = 0;
int Window::fbWidth = 0;
int Window::fbHeight = 0;

int Window::initialize(int width, int height, const char* title){
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (window == nullptr){
		std::cerr << "Failed to create GLFW Window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK){
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
	
	// Получаем размеры framebuffer (для HiDPI)
	glfwGetFramebufferSize(window, &Window::fbWidth, &Window::fbHeight);
	glViewport(0, 0, Window::fbWidth, Window::fbHeight);

	Window::width = width;
	Window::height = height;
	Window::resetScissor();
	return 0;
}

void Window::setCursorMode(int mode){
	glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void Window::terminate(){
	glfwTerminate();
}

bool Window::isShouldClose(){
	return glfwWindowShouldClose(window);
}

void Window::setShouldClose(bool flag){
	glfwSetWindowShouldClose(window, flag);
}

void Window::swapBuffers(){
	glfwSwapBuffers(window);
	Window::resetScissor();
}

void Window::viewport(int x, int y, int width, int height){
	glViewport(x, y, width, height);
}

void Window::swapInterval(int interval){
	glfwSwapInterval(interval);
}

void Window::resetScissor() {
	scissorArea = vec4(0.0f, 0.0f, (float)width, (float)height);
	scissorStack = std::stack<glm::vec4>();
	glDisable(GL_SCISSOR_TEST);
}

void Window::pushScissor(glm::vec4 area) {
	if (scissorStack.empty()) {
		glEnable(GL_SCISSOR_TEST);
	}
	scissorStack.push(scissorArea);

	area.x = fmax(area.x, scissorArea.x);
	area.y = fmax(area.y, scissorArea.y);

	area.z = fmin(area.z, scissorArea.z);
	area.w = fmin(area.w, scissorArea.w);

	glScissor((int)area.x, height-(int)area.y-(int)area.w, (int)area.z, (int)area.w);
	scissorArea = area;
}

void Window::popScissor() {
	if (scissorStack.empty()) {
		std::cerr << "warning: extra Window::popScissor call" << std::endl;
		return;
	}
	glm::vec4 area = scissorStack.top();
	scissorStack.pop();
	glScissor((int)area.x, height-(int)area.y-(int)area.w, (int)area.z, (int)area.w);
	if (scissorStack.empty()) {
		glDisable(GL_SCISSOR_TEST);
	}
	scissorArea = area;
}

void Window::clear() {
	glClear(GL_COLOR_BUFFER_BIT);
}

double Window::time() {
	return glfwGetTime();
}
