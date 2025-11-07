#include <iostream>

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
#include "graphics/Batch2D.h"
#include "graphics/Font.h"
#include "loaders/png_loading.h"
#include "voxels/ChunkManager.h"
#include "voxels/MCChunk.h"
#include "voxels/voxel.h"
#include "voxels/WorldSave.h"
#include "frontend/Menu.h"
#include "frontend/GameState.h"
#include "window/Window.h"
#include "window/Events.h"
#include "window/Camera.h"
#include "maths/FrustumCulling.h"
#include "files/files.h"
#include <vector>
#include <string>

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

	Shader* shader = load_shader("res/shaders/mc_vert.glsl", "res/shaders/mc_frag.glsl");
	if (shader == nullptr) {
		std::cerr << "failed to load shader" << std::endl;
		Window::terminate();
		return 1;
	}
	
	Shader* voxelShader = load_shader("res/shaders/voxel_vert.glsl", "res/shaders/voxel_frag.glsl");
	if (voxelShader == nullptr) {
		std::cerr << "failed to load voxel shader" << std::endl;
		Window::terminate();
		return 1;
	}
	
	Texture* texture = load_texture("res/textures/blocks/stone.png");
	if (texture == nullptr) {
		std::cerr << "failed to load texture" << std::endl;
		Window::terminate();
		return 1;
	}
	
	// Загружаем UI шейдер
	Shader* uiShader = load_shader("res/shaders/ui.glslv", "res/shaders/ui.glslf");
	if (uiShader == nullptr) {
		std::cerr << "failed to load UI shader" << std::endl;
		Window::terminate();
		return 1;
	}
	// Устанавливаем текстурный юнит для UI шейдера (GL_TEXTURE0)
	glActiveTexture(GL_TEXTURE0);
	uiShader->use();
	uiShader->uniform1i("u_texture", 0);   // sampler → слот 0

	// Проверяем, что локация uniform'а существует
	GLint loc_texture = glGetUniformLocation(uiShader->id, "u_texture");
	std::cout << "[UI] loc u_texture = " << loc_texture << std::endl;
	if (loc_texture == -1) {
		std::cerr << "[UI] WARNING: u_texture uniform location = -1!" << std::endl;
	}
	
	// Загружаем шрифт
	std::vector<Texture*> fontPages;
	for (int i = 0; i <= 4; i++) {
		std::string fontFile = "res/fonts/font_" + std::to_string(i) + ".png";
		Texture* fontPage = load_texture(fontFile);
		if (fontPage == nullptr) {
			std::cerr << "failed to load font page " << i << std::endl;
			Window::terminate();
			return 1;
		}
		fontPages.push_back(fontPage);
	}
	
	// Настраиваем фильтры для пиксельного (Minecraft) шрифта
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (auto* t : fontPages) {
		t->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	
	Font* font = new Font(fontPages, fontPages[0]->height / 16);
	
	// Создаем Batch2D для UI
	Batch2D* batch = new Batch2D(1024);
	
	VoxelRenderer voxelRenderer(1024 * 1024 * 8);

	// Система меню
	Menu menu;
	
	// Проверяем существование сохранения
	const std::string saveFileName = "world.vxl";
	bool saveExists = files::file_exists(saveFileName);
	menu.setSaveFileExists(saveExists);
	if (saveExists) {
		std::cout << "[MENU] Save file found: " << saveFileName << std::endl;
	} else {
		std::cout << "[MENU] No save file found, will create new world" << std::endl;
	}
	
	// Создаем менеджер чанков для генерации ландшафта
	ChunkManager chunkManager;
	
	// Frustum culling для оптимизации рендеринга
	Frustum frustum;
	
	// Параметры генерации
	float baseFreq = 0.03f; // Меньшая частота для более плавных холмов
	int octaves = 4; // Меньше октав для более ровной поверхности
	float lacunarity = 2.0f;
	float gain = 0.5f;
	float baseHeight = 12.0f; // Базовая высота поверхности
	float heightVariation = 4.0f; // Вариация высоты холмов
	int seed = 1337; // Seed для генерации мира
	
	// Система сохранения/загрузки
	WorldSave worldSave;
	
	// Флаг инициализации мира
	bool worldInitialized = false;
	
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
	
	// Отслеживаем предыдущее состояние для проверки файла сохранения при переходе в меню
	GameState previousState = GameState::MENU;

	while (!Window::isShouldClose()) {
		float currentTime = glfwGetTime();
		delta = currentTime - lastTime;
		lastTime = currentTime;
		
		// Обновляем меню
		GameState currentState = menu.update();
		
		// Проверяем файл сохранения при переходе в главное меню
		if (currentState == GameState::MENU && previousState != GameState::MENU) {
			bool saveExists = files::file_exists(saveFileName);
			menu.setSaveFileExists(saveExists);
			if (saveExists) {
				std::cout << "[MENU] Save file found: " << saveFileName << std::endl;
			} else {
				std::cout << "[MENU] No save file found" << std::endl;
			}
		}
		
		previousState = currentState;
		
		// Обрабатываем действия меню
		Menu::MenuAction action = menu.getMenuAction();
		if (action == Menu::MenuAction::CREATE_WORLD) {
			// Создаем новый мир
			if (!worldInitialized) {
				// Очищаем старый мир, если был
				chunkManager.clear();
				chunkManager.setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
				worldInitialized = true;
				// Обновляем флаг существования сохранения (новый мир - сохранения нет)
				menu.setSaveFileExists(false);
				std::cout << "[GAME] New world created" << std::endl;
			}
			menu.clearMenuAction();
		} else if (action == Menu::MenuAction::LOAD_WORLD) {
			// Загружаем сохраненный мир
			if (!worldInitialized) {
				// Очищаем текущий мир перед загрузкой
				chunkManager.clear();
				if (worldSave.load(saveFileName, chunkManager, seed, baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation)) {
					std::cout << "[LOAD] World loaded successfully from " << saveFileName << std::endl;
					chunkManager.setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
					worldInitialized = true;
					// Обновляем флаг существования сохранения
					menu.setSaveFileExists(true);
				} else {
					std::cout << "[LOAD] Failed to load world, creating new world" << std::endl;
					chunkManager.setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
					worldInitialized = true;
					// Если загрузка не удалась, сохранения нет
					menu.setSaveFileExists(false);
				}
			}
			menu.clearMenuAction();
		} else if (action == Menu::MenuAction::QUIT) {
			// Выход из игры
			Window::setShouldClose(true);
		}
		
		// Если игра на паузе или в меню, не обрабатываем игровой ввод
		bool inputLocked = (currentState == GameState::MENU || currentState == GameState::PAUSED);
		
		// Если игра не инициализирована или в меню, пропускаем игровой цикл
		if (!worldInitialized || currentState == GameState::MENU) {
			// Отрисовываем только меню
			if (currentState == GameState::MENU) {
				// Убеждаемся, что viewport установлен правильно
				glViewport(0, 0, Window::fbWidth > 0 ? Window::fbWidth : Window::width, 
				                  Window::fbHeight > 0 ? Window::fbHeight : Window::height);
				
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				// UI state: отключаем cull и depth, включаем blend
				glDisable(GL_CULL_FACE);
				glDisable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				
				// Гарантируем нужный текстурный юнит перед UI-рендером
				glActiveTexture(GL_TEXTURE0);
				uiShader->use();
				uiShader->uniform1i("u_texture", 0);   // sampler → слот 0
				
				menu.draw(batch, font, uiShader, Window::width, Window::height);
				
				// Восстанавливаем состояние
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
			}
			Window::swapBuffers();
			Events::pullEvents();
			continue;
		}
		// Игровой ввод (только если игра не на паузе)
		if (!inputLocked) {
			if (Events::jpressed(GLFW_KEY_TAB)) {
				Events::toggleCursor();
			}
			
			// Тестовая установка блока по нажатию клавиши T
			if (Events::jpressed(GLFW_KEY_T)) {
				int bx = (int)camera->position.x;
				int by = (int)camera->position.y;
				int bz = (int)camera->position.z;
				std::cout << "Тестовая установка блока в позиции камеры: (" << bx << ", " << by << ", " << bz << ")" << std::endl;
				chunkManager.setVoxel(bx, by, bz, 2);
			}
			
			// Сохранение мира по нажатию F5
			if (Events::jpressed(GLFW_KEY_F5)) {
				float bf, l, g, bh, hv;
				int o;
				chunkManager.getNoiseParams(bf, o, l, g, bh, hv);
				if (worldSave.save(saveFileName, chunkManager, seed, bf, o, l, g, bh, hv)) {
					std::cout << "[SAVE] World saved successfully to " << saveFileName << std::endl;
					// Обновляем флаг существования сохранения
					menu.setSaveFileExists(true);
				} else {
					std::cout << "[SAVE] Failed to save world to " << saveFileName << std::endl;
				}
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
					std::cout << "[DELETE] Block removed at (" << bx << ", " << by << ", " << bz << ")" << std::endl;
					chunkManager.setVoxel(bx, by, bz, 0);
				}
				if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
					// Установка блока рядом с найденным (правая кнопка мыши)
					int bx = (int)(iend.x) + (int)(norm.x);
					int by = (int)(iend.y) + (int)(norm.y);
					int bz = (int)(iend.z) + (int)(norm.z);
					std::cout << "[PLACE] Attempting to place block next to voxel at (" 
					          << bx << ", " << by << ", " << bz << ")" << std::endl;
					chunkManager.setVoxel(bx, by, bz, 2);
					
					// Проверяем, установился ли блок
					voxel* checkVox = chunkManager.getVoxel(bx, by, bz);
					if (checkVox != nullptr && checkVox->id == 2) {
						std::cout << "[SUCCESS] Block placed at (" << bx << ", " << by << ", " << bz << ")" << std::endl;
					} else {
						std::cout << "[ERROR] Block NOT placed at (" << bx << ", " << by << ", " << bz << ")" << std::endl;
					}
				}
			} else {
				// Не нашли воксельный блок - ищем поверхность земли (Marching Cubes)
				vec3 surfacePos;
				vec3 surfaceNorm;
				if (chunkManager.rayCastSurface(camera->position, camera->front, 10.0f, surfacePos, surfaceNorm)) {
					if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
						// Установка блока на поверхность земли
						std::cout << "[SURFACE] Found ground surface at (" 
						          << surfacePos.x << ", " << surfacePos.y << ", " << surfacePos.z 
						          << ") normal: (" << surfaceNorm.x << ", " << surfaceNorm.y << ", " << surfaceNorm.z << ")" << std::endl;
						
						// Округляем до целых координат (сетка блоков)
						int bx = (int)std::round(surfacePos.x);
						int by = (int)std::round(surfacePos.y);
						int bz = (int)std::round(surfacePos.z);
						
						// Если поверхность выше, ставим блок на поверхность, иначе немного выше
						if (surfaceNorm.y > 0.5f) {
							// Поверхность сверху - ставим на неё
							by = (int)std::floor(surfacePos.y) + 1;
							std::cout << "[PLACE] Surface on top, placing block on surface" << std::endl;
						} else {
							// Поверхность сбоку - ставим рядом
							by = (int)std::round(surfacePos.y);
							std::cout << "[PLACE] Surface on side, placing block nearby" << std::endl;
						}
						
						std::cout << "[PLACE] Attempting to place block on ground surface at (" 
						          << bx << ", " << by << ", " << bz << ")" << std::endl;
						
						chunkManager.setVoxel(bx, by, bz, 2);
						
						// Проверяем, установился ли блок
						voxel* checkVox = chunkManager.getVoxel(bx, by, bz);
						if (checkVox != nullptr && checkVox->id == 2) {
							std::cout << "[SUCCESS] Block placed on ground surface at (" 
							          << bx << ", " << by << ", " << bz << ")" << std::endl;
						} else {
							std::cout << "[ERROR] Block NOT placed on ground surface at (" 
							          << bx << ", " << by << ", " << bz << ")" << std::endl;
						}
					}
				} else {
					// Отладочный вывод, если не нашли поверхность
					if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
						std::cout << "[ERROR] Ground surface not found. Camera pos: (" 
						          << camera->position.x << ", " << camera->position.y << ", " << camera->position.z 
						          << ") Direction: (" << camera->front.x << ", " << camera->front.y << ", " << camera->front.z << ")" << std::endl;
					}
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
		
		// Обновляем frustum для culling
		mat4 projview = camera->getProjection() * camera->getView();
		frustum.update(projview);
		
		// Отрисовываем Marching Cubes (поверхность земли)
		shader->use();
		shader->uniformMatrix("projview", projview);
		
		for (MCChunk* chunk : visibleChunks) {
			if (chunk->mesh != nullptr) {
				// Frustum culling для чанка
				vec3 chunkMin = chunk->worldPos - vec3(
					MCChunk::CHUNK_SIZE_X / 2.0f,
					MCChunk::CHUNK_SIZE_Y / 2.0f,
					MCChunk::CHUNK_SIZE_Z / 2.0f
				);
				vec3 chunkMax = chunk->worldPos + vec3(
					MCChunk::CHUNK_SIZE_X / 2.0f,
					MCChunk::CHUNK_SIZE_Y / 2.0f,
					MCChunk::CHUNK_SIZE_Z / 2.0f
				);
				
				if (!frustum.IsBoxVisible(chunkMin, chunkMax)) {
					continue; // Пропускаем чанк вне frustum
				}
				
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
		voxelShader->uniformMatrix("projview", projview);
		texture->bind();
		
		for (MCChunk* chunk : visibleChunks) {
			if (chunk->voxelMesh != nullptr) {
				// Frustum culling для воксельного чанка
				vec3 chunkMin = chunk->worldPos - vec3(
					MCChunk::CHUNK_SIZE_X / 2.0f,
					MCChunk::CHUNK_SIZE_Y / 2.0f,
					MCChunk::CHUNK_SIZE_Z / 2.0f
				);
				vec3 chunkMax = chunk->worldPos + vec3(
					MCChunk::CHUNK_SIZE_X / 2.0f,
					MCChunk::CHUNK_SIZE_Y / 2.0f,
					MCChunk::CHUNK_SIZE_Z / 2.0f
				);
				
				if (!frustum.IsBoxVisible(chunkMin, chunkMax)) {
					continue; // Пропускаем чанк вне frustum
				}
				
				mat4 chunkModel(1.0f); // Воксели уже в мировых координатах
				voxelShader->uniformMatrix("model", chunkModel);
				chunk->voxelMesh->draw(GL_TRIANGLES);
			}
		}
		
		// Отрисовываем меню паузы поверх мира
		if (currentState == GameState::PAUSED) {
			// UI state: отключаем cull и depth, включаем blend
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			
			// Гарантируем нужный текстурный юнит перед UI-рендером
			glActiveTexture(GL_TEXTURE0);
			uiShader->use();
			uiShader->uniform1i("u_texture", 0);   // sampler → слот 0
			
			menu.draw(batch, font, uiShader, Window::width, Window::height);
			
			// Восстанавливаем состояние
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
		}
		
		Window::swapBuffers();
		Events::pullEvents();
	}

	// Автосохранение при выходе
	if (worldInitialized) {
		float bf, l, g, bh, hv;
		int o;
		chunkManager.getNoiseParams(bf, o, l, g, bh, hv);
		if (worldSave.save(saveFileName, chunkManager, seed, bf, o, l, g, bh, hv)) {
			std::cout << "[SAVE] Auto-saved world on exit" << std::endl;
			menu.setSaveFileExists(true);
		} else {
			std::cout << "[SAVE] Failed to auto-save world on exit" << std::endl;
		}
	}
	
	delete shader;
	delete voxelShader;
	delete uiShader;
	delete texture;
	delete font;
	delete batch;

	Events::finalize();
	Window::terminate();
	return 0;
}
