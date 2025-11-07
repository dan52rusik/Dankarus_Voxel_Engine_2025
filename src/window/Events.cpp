#include "Events.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string.h>

bool* Events::_keys;
uint* Events::_frames;
uint Events::_current = 0;
float Events::deltaX = 0.0f;
float Events::deltaY = 0.0f;
float Events::x = 0.0f;
float Events::y = 0.0f;
bool Events::_cursor_locked = false;
bool Events::_cursor_started = false;
std::vector<uint> Events::codepoints;
std::vector<int> Events::pressedKeys;

#define _MOUSE_BUTTONS 1024

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
	if (Events::_cursor_started){
		Events::deltaX += xpos-Events::x;
		Events::deltaY += ypos-Events::y;
	}
	else {
		Events::_cursor_started = true;
	}
	Events::x = xpos;
	Events::y = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mode){
	if (action == GLFW_PRESS){
		Events::_keys[_MOUSE_BUTTONS+button] = true;
		Events::_frames[_MOUSE_BUTTONS+button] = Events::_current;
	}
	else if (action == GLFW_RELEASE){
		Events::_keys[_MOUSE_BUTTONS+button] = false;
		Events::_frames[_MOUSE_BUTTONS+button] = Events::_current;
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (action == GLFW_PRESS){
		Events::_keys[key] = true;
		Events::_frames[key] = Events::_current;
		Events::pressedKeys.push_back(key);
	}
	else if (action == GLFW_RELEASE){
		Events::_keys[key] = false;
		Events::_frames[key] = Events::_current;
	}
	else if (action == GLFW_REPEAT) {
		Events::pressedKeys.push_back(key);
	}
}

void character_callback(GLFWwindow* window, unsigned int codepoint){
	Events::codepoints.push_back(codepoint);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
	glViewport(0, 0, width, height);
	Window::fbWidth = width;
	Window::fbHeight = height;
}

void window_size_callback(GLFWwindow* window, int width, int height){
	Window::width = width;
	Window::height = height;
	// Обновляем framebuffer size
	glfwGetFramebufferSize(window, &Window::fbWidth, &Window::fbHeight);
	glViewport(0, 0, Window::fbWidth, Window::fbHeight);
}

int Events::initialize(){
	GLFWwindow* window = Window::window;
	_keys = new bool[1032];
	_frames = new uint[1032];

	memset(_keys, false, 1032*sizeof(bool));
	memset(_frames, 0, 1032*sizeof(uint));

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCharCallback(window, character_callback);
	return 0;
}

bool Events::pressed(int keycode){
	if (keycode < 0 || keycode >= _MOUSE_BUTTONS)
		return false;
	return _keys[keycode];
}

bool Events::jpressed(int keycode){
	if (keycode < 0 || keycode >= _MOUSE_BUTTONS)
		return false;
	return _keys[keycode] && _frames[keycode] == _current;
}

bool Events::clicked(int button){
	int index = _MOUSE_BUTTONS+button;
	return _keys[index];
}

bool Events::jclicked(int button){
	int index = _MOUSE_BUTTONS+button;
	return _keys[index] && _frames[index] == _current;
}

void Events::toggleCursor(){
	_cursor_locked = !_cursor_locked;
	Window::setCursorMode(_cursor_locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Events::finalize(){
	delete[] _keys;
	delete[] _frames;
}

void Events::pullEvents(){
	_current++;
	deltaX = 0.0f;
	deltaY = 0.0f;
	codepoints.clear();
	pressedKeys.clear();
	glfwPollEvents();
}
