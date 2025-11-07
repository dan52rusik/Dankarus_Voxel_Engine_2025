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
#include "graphics/Texture.h"
#include "graphics/VoxelRenderer.h"
#include "loaders/png_loading.h"
#include "voxels/ChunkManager.h"
#include "voxels/MCChunk.h"
#include "voxels/voxel.h"
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
	
	Shader* voxelShader = load_shader("res/voxel_vert.glsl", "res/voxel_frag.glsl");
	if (voxelShader == nullptr) {
		std::cerr << "failed to load voxel shader" << std::endl;
		Window::terminate();
		return 1;
	}
	
	Texture* texture = load_texture("res/block.png");
	if (texture == nullptr) {
		std::cerr << "failed to load texture" << std::endl;
		Window::terminate();
		return 1;
	}
	
	VoxelRenderer voxelRenderer(1024 * 1024 * 8);

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
		
		// Тестовая установка блока по нажатию клавиши T
		if (Events::jpressed(GLFW_KEY_T)) {
			int bx = (int)camera->position.x;
			int by = (int)camera->position.y;
			int bz = (int)camera->position.z;
			std::cout << "Тестовая установка блока в позиции камеры: (" << bx << ", " << by << ", " << bz << ")" << std::endl;
			chunkManager.setVoxel(bx, by, bz, 2);
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
		
		// Обработка кликов мыши для установки/удаления блоков
		{
			vec3 end;
			vec3 norm;
			vec3 iend;
			
			// Сначала проверяем воксельные блоки
			voxel* vox = chunkManager.rayCast(camera->position, camera->front, 10.0f, end, norm, iend);
			
			if (vox != nullptr) {
				// Нашли воксельный блок
				if (Events::jclicked(GLFW_MOUSE_BUTTON_1)) {
					// Удаление блока (левая кнопка мыши)
					int bx = (int)iend.x;
					int by = (int)iend.y;
					int bz = (int)iend.z;
					std::cout << "[УДАЛЕНИЕ] Блок удален в координатах (" << bx << ", " << by << ", " << bz << ")" << std::endl;
					chunkManager.setVoxel(bx, by, bz, 0);
				}
				if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
					// Установка блока рядом с найденным (правая кнопка мыши)
					int bx = (int)(iend.x) + (int)(norm.x);
					int by = (int)(iend.y) + (int)(norm.y);
					int bz = (int)(iend.z) + (int)(norm.z);
					std::cout << "[УСТАНОВКА] Попытка установить блок рядом с воксельным блоком в координатах (" 
					          << bx << ", " << by << ", " << bz << ")" << std::endl;
					chunkManager.setVoxel(bx, by, bz, 2);
					
					// Проверяем, установился ли блок
					voxel* checkVox = chunkManager.getVoxel(bx, by, bz);
					if (checkVox != nullptr && checkVox->id == 2) {
						std::cout << "[УСПЕХ] Блок успешно установлен в координатах (" << bx << ", " << by << ", " << bz << ")" << std::endl;
					} else {
						std::cout << "[ОШИБКА] Блок не установлен в координатах (" << bx << ", " << by << ", " << bz << ")" << std::endl;
					}
				}
			} else {
				// Не нашли воксельный блок - ищем поверхность земли (Marching Cubes)
				vec3 surfacePos;
				vec3 surfaceNorm;
				if (chunkManager.rayCastSurface(camera->position, camera->front, 10.0f, surfacePos, surfaceNorm)) {
					if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
						// Установка блока на поверхность земли
						std::cout << "[ПОВЕРХНОСТЬ] Найдена поверхность земли в координатах (" 
						          << surfacePos.x << ", " << surfacePos.y << ", " << surfacePos.z 
						          << ") нормаль: (" << surfaceNorm.x << ", " << surfaceNorm.y << ", " << surfaceNorm.z << ")" << std::endl;
						
						// Округляем до целых координат (сетка блоков)
						int bx = (int)std::round(surfacePos.x);
						int by = (int)std::round(surfacePos.y);
						int bz = (int)std::round(surfacePos.z);
						
						// Если поверхность выше, ставим блок на поверхность, иначе немного выше
						if (surfaceNorm.y > 0.5f) {
							// Поверхность сверху - ставим на неё
							by = (int)std::floor(surfacePos.y) + 1;
							std::cout << "[УСТАНОВКА] Поверхность сверху, ставим блок на поверхность" << std::endl;
						} else {
							// Поверхность сбоку - ставим рядом
							by = (int)std::round(surfacePos.y);
							std::cout << "[УСТАНОВКА] Поверхность сбоку, ставим блок рядом" << std::endl;
						}
						
						std::cout << "[УСТАНОВКА] Попытка установить блок на поверхность земли в координатах (" 
						          << bx << ", " << by << ", " << bz << ")" << std::endl;
						
						chunkManager.setVoxel(bx, by, bz, 2);
						
						// Проверяем, установился ли блок
						voxel* checkVox = chunkManager.getVoxel(bx, by, bz);
						if (checkVox != nullptr && checkVox->id == 2) {
							std::cout << "[УСПЕХ] Блок успешно установлен на поверхность земли в координатах (" 
							          << bx << ", " << by << ", " << bz << ")" << std::endl;
						} else {
							std::cout << "[ОШИБКА] Блок не установлен на поверхность земли в координатах (" 
							          << bx << ", " << by << ", " << bz << ")" << std::endl;
						}
					}
				} else {
					// Отладочный вывод, если не нашли поверхность
					if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
						std::cout << "[ОШИБКА] Поверхность земли не найдена. Позиция камеры: (" 
						          << camera->position.x << ", " << camera->position.y << ", " << camera->position.z 
						          << ") Направление: (" << camera->front.x << ", " << camera->front.y << ", " << camera->front.z << ")" << std::endl;
					}
				}
			}
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Обновляем чанки вокруг камеры
		chunkManager.update(camera->position, renderDistance);
		
		// Пересобираем меши вокселей для измененных чанков
		std::vector<MCChunk*> visibleChunks = chunkManager.getVisibleChunks();
		for (MCChunk* chunk : visibleChunks) {
			if (chunk->voxelMeshModified) {
				// Удаляем старый меш
				if (chunk->voxelMesh != nullptr) {
					delete chunk->voxelMesh;
					chunk->voxelMesh = nullptr;
				}
				
				// Собираем соседние чанки для face culling
				MCChunk* nearbyChunks[27] = {nullptr};
				for (MCChunk* other : visibleChunks) {
					glm::ivec3 diff = other->chunkPos - chunk->chunkPos;
					if (std::abs(diff.x) <= 1 && std::abs(diff.y) <= 1 && std::abs(diff.z) <= 1) {
						int index = (diff.y + 1) * 9 + (diff.z + 1) * 3 + (diff.x + 1);
						nearbyChunks[index] = other;
					}
				}
				
				// Генерируем новый меш
				chunk->voxelMesh = voxelRenderer.render(chunk, nearbyChunks);
				chunk->voxelMeshModified = false;
			}
		}
		
		// Отрисовываем Marching Cubes (поверхность земли)
		shader->use();
		shader->uniformMatrix("projview", camera->getProjection() * camera->getView());
		
		for (MCChunk* chunk : visibleChunks) {
			if (chunk->mesh != nullptr) {
				// Вычисляем матрицу модели для позиционирования чанка в мире
				mat4 chunkModel = translate(model, chunk->worldPos - vec3(
					MCChunk::CHUNK_SIZE_X / 2.0f,
					MCChunk::CHUNK_SIZE_Y / 2.0f,
					MCChunk::CHUNK_SIZE_Z / 2.0f
				));
				shader->uniformMatrix("model", chunkModel);
				chunk->mesh->draw(GL_TRIANGLES);
			}
		}
		
		// Отрисовываем воксельные блоки
		voxelShader->use();
		voxelShader->uniformMatrix("projview", camera->getProjection() * camera->getView());
		texture->bind();
		
		for (MCChunk* chunk : visibleChunks) {
			if (chunk->voxelMesh != nullptr) {
				mat4 chunkModel(1.0f); // Воксели уже в мировых координатах
				voxelShader->uniformMatrix("model", chunkModel);
				chunk->voxelMesh->draw(GL_TRIANGLES);
			}
		}
		
		Window::swapBuffers();
		Events::pullEvents();
	}

	delete shader;
	delete voxelShader;
	delete texture;

	Window::terminate();
	return 0;
}
