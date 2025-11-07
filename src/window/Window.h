#ifndef WINDOW_WINDOW_H_
#define WINDOW_WINDOW_H_

#include <glm/glm.hpp>
#include <stack>

class GLFWwindow;

class Window {
	static std::stack<glm::vec4> scissorStack;
	static glm::vec4 scissorArea;
public:
	static int width;
	static int height;
	static int fbWidth;  // Framebuffer width (для HiDPI)
	static int fbHeight; // Framebuffer height (для HiDPI)
	static GLFWwindow* window;
	static int initialize(int width, int height, const char* title);
	static void terminate();

	static void viewport(int x, int y, int width, int height);
	static void setCursorMode(int mode);
	static bool isShouldClose();
	static void setShouldClose(bool flag);
	static void swapBuffers();
	static void swapInterval(int interval);
	
	// Scissor test функции для отсечения рендеринга
	static void pushScissor(glm::vec4 area);
	static void popScissor();
	static void resetScissor();
	
	static void clear();
	static double time();
};

#endif /* WINDOW_WINDOW_H_ */
