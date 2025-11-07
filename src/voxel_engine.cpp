#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

#include "graphics/Shader.h"
#include "graphics/Mesh.h"
#include "graphics/MarchingCubes.h"
#include "noise/OpenSimplex.h"
#include "window/Window.h"
#include "window/Events.h"
#include "window/Camera.h"
#include <vector>

int WIDTH = 1280;
int HEIGHT = 720;

float vertices[] = {
	// x    y     z     u     v
   -1.0f,-1.0f, 0.0f, 0.0f, 0.0f,
	1.0f,-1.0f, 0.0f, 1.0f, 0.0f,
   -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,

	1.0f,-1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};

int attrs[] = {
		3,2,  0 //null terminator
};

int main() {
	Window::initialize(WIDTH, HEIGHT, "Window 2.0");
	Events::initialize();

	Shader* shader = load_shader("res/mc_vert.glsl", "res/mc_frag.glsl");
	if (shader == nullptr) {
		std::cerr << "failed to load shader" << std::endl;
		Window::terminate();
		return 1;
	}

	// Build scalar field via OpenSimplex FBM and extract isosurface via Marching Cubes
	const int NX = 32, NY = 32, NZ = 32; // cell counts
	const int SX = NX + 1, SY = NY + 1, SZ = NZ + 1; // grid nodes
	std::vector<float> field;
	field.resize(SX * SY * SZ);

	OpenSimplex3D noise(1337);
	const float baseFreq = 0.05f;
	const int octaves = 6;
	const float lacunarity = 2.0f;
	const float gain = 0.5f;
	const float seaLevel = 16.0f;
	const float verticalScale = 32.0f;

	for (int y = 0; y < SY; y++) {
		for (int z = 0; z < SZ; z++) {
			for (int x = 0; x < SX; x++) {
				float wx = (float)x;
				float wy = (float)y;
				float wz = (float)z;
				float n = noise.fbm(wx * baseFreq, wy * baseFreq, wz * baseFreq, octaves, lacunarity, gain);
				float density = n - (wy - seaLevel) / verticalScale;
				field[(y * SZ + z) * SX + x] = density;
			}
		}
	}

	Mesh* mesh = buildIsoSurface(field.data(), NX, NY, NZ, 0.0f);
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // Небесный бело-голубой фон

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	Camera* camera = new Camera(vec3(16, 16, 50), radians(90.0f));

	mat4 model(1.0f);
	// model = translate(model, vec3(0.5f, 0, 0));

	float lastTime = glfwGetTime();
	float delta = 0.0f;

	float camX = 0.0f;
	float camY = 0.0f;

	float speed = 5;

	while (!Window::isShouldClose()) {
		float currentTime = glfwGetTime();
		delta = currentTime - lastTime;
		lastTime = currentTime;

		if (Events::jpressed(GLFW_KEY_ESCAPE)) {
			Window::setShouldClose(true);
		}
		if (Events::jpressed(GLFW_KEY_TAB)) {
			Events::toogleCursor();
		}

		if (Events::pressed(GLFW_KEY_W)) {
			camera->position += camera->front * delta * speed;
		}
		if (Events::pressed(GLFW_KEY_S)) {
			camera->position -= camera->front * delta * speed;
		}
		if (Events::pressed(GLFW_KEY_D)) {
			camera->position -= camera->right * delta * speed;
		}
		if (Events::pressed(GLFW_KEY_A)) {
			camera->position += camera->right * delta * speed;
		}

		if (Events::_cursor_locked) {
			camY += Events::deltaY / Window::height * 2;
			camX += -Events::deltaX / Window::height * 2;

			if (camY < -radians(89.0f)) {
				camY = -radians(89.0f);
			}
			if (camY > radians(89.0f)) {
				camY = radians(89.0f);
			}

			camera->rotation = mat4(1.0f);
			camera->rotate(camY, camX, 0);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw VAO
		shader->use();
		shader->uniformMatrix("model", model);
		shader->uniformMatrix("projview", camera->getProjection() * camera->getView());
		mesh->draw(GL_TRIANGLES);
		Window::swapBuffers();
		Events::pullEvents();
	}

	delete shader;
	delete mesh;

	Window::terminate();
	return 0;
}
