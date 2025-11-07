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
#include "voxels/ChunkManager.h"
#include "voxels/MCChunk.h"
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

	// Создаем менеджер чанков для генерации ландшафта
	ChunkManager chunkManager;
	
	// Параметры генерации
	const float baseFreq = 0.03f; // Меньшая частота для более плавных холмов
	const int octaves = 4; // Меньше октав для более ровной поверхности
	const float lacunarity = 2.0f;
	const float gain = 0.5f;
	const float baseHeight = 12.0f; // Базовая высота поверхности
	const float heightVariation = 4.0f; // Вариация высоты холмов
	
	chunkManager.setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
	
	// Радиус загрузки чанков вокруг камеры
	const int renderDistance = 3;
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // Небесный бело-голубой фон

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Камера выше для обзора ландшафта
	Camera* camera = new Camera(vec3(0.0f, baseHeight + 8.0f, 20.0f), radians(90.0f));

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

		// Обновляем чанки вокруг камеры
		chunkManager.update(camera->position, renderDistance);
		
		// Отрисовываем все видимые чанки
		shader->use();
		shader->uniformMatrix("projview", camera->getProjection() * camera->getView());
		
		std::vector<MCChunk*> visibleChunks = chunkManager.getVisibleChunks();
		for (MCChunk* chunk : visibleChunks) {
			if (chunk->mesh != nullptr) {
				// Вычисляем матрицу модели для позиционирования чанка в мире
				// worldPos - это центр чанка, но меш генерируется от (0,0,0), поэтому сдвигаем
				mat4 chunkModel = translate(model, chunk->worldPos - vec3(
					MCChunk::CHUNK_SIZE_X / 2.0f,
					MCChunk::CHUNK_SIZE_Y / 2.0f,
					MCChunk::CHUNK_SIZE_Z / 2.0f
				));
				shader->uniformMatrix("model", chunkModel);
				chunk->mesh->draw(GL_TRIANGLES);
			}
		}
		
		Window::swapBuffers();
		Events::pullEvents();
	}

	delete shader;

	Window::terminate();
	return 0;
}
