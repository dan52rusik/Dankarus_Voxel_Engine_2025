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
#include <cmath>

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

// Функция для отрисовки контура блока (wireframe всего куба)
void drawBlockOutline(Shader* linesShader, const mat4& projview, int blockX, int blockY, int blockZ, const vec3& faceNormal) {
	// Цвет контура (белый с хорошей видимостью)
	const vec4 outlineColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	// Воксельные блоки рендерятся с центром в целых координатах (blockX, blockY, blockZ)
	// Грани блока идут от (center - 0.5) до (center + 0.5)
	// Контур должен точно совпадать с гранями блока
	float x0 = (float)blockX - 0.5f;
	float x1 = (float)blockX + 0.5f;
	float y0 = (float)blockY - 0.5f;
	float y1 = (float)blockY + 0.5f;
	float z0 = (float)blockZ - 0.5f;
	float z1 = (float)blockZ + 0.5f;
	
	// Небольшое смещение наружу для избежания z-fighting
	const float epsilon = 0.001f;
	vec3 center((float)blockX, (float)blockY, (float)blockZ);
	
	// Функция для смещения вершины наружу от центра
	auto offsetVertex = [&center, epsilon](float x, float y, float z) -> vec3 {
		vec3 vertex(x, y, z);
		vec3 dir = normalize(vertex - center);
		return vertex + dir * epsilon;
	};
	
	// Вектор вершин для всех 12 ребер куба (wireframe)
	std::vector<float> vertices;
	vertices.reserve(24 * 7); // 24 вершины (12 ребер * 2 точки) * 7 компонентов
	
	auto addLine = [&vertices, &outlineColor, &offsetVertex](float x1, float y1, float z1, float x2, float y2, float z2) {
		// Смещаем вершины наружу от центра для избежания z-fighting
		vec3 v1 = offsetVertex(x1, y1, z1);
		vec3 v2 = offsetVertex(x2, y2, z2);
		
		// Первая точка линии
		vertices.push_back(v1.x);
		vertices.push_back(v1.y);
		vertices.push_back(v1.z);
		vertices.push_back(outlineColor.r);
		vertices.push_back(outlineColor.g);
		vertices.push_back(outlineColor.b);
		vertices.push_back(outlineColor.a);
		// Вторая точка линии
		vertices.push_back(v2.x);
		vertices.push_back(v2.y);
		vertices.push_back(v2.z);
		vertices.push_back(outlineColor.r);
		vertices.push_back(outlineColor.g);
		vertices.push_back(outlineColor.b);
		vertices.push_back(outlineColor.a);
	};
	
	// Рисуем все 12 ребер куба (wireframe)
	// Y - вертикальная ось (вверх)
	// Нижняя грань (y = y0)
	addLine(x0, y0, z0, x1, y0, z0); // переднее нижнее ребро (по X)
	addLine(x1, y0, z0, x1, y0, z1); // правое нижнее ребро (по Z)
	addLine(x1, y0, z1, x0, y0, z1); // заднее нижнее ребро (по X)
	addLine(x0, y0, z1, x0, y0, z0); // левое нижнее ребро (по Z)
	
	// Верхняя грань (y = y1)
	addLine(x0, y1, z0, x1, y1, z0); // переднее верхнее ребро (по X)
	addLine(x1, y1, z0, x1, y1, z1); // правое верхнее ребро (по Z)
	addLine(x1, y1, z1, x0, y1, z1); // заднее верхнее ребро (по X)
	addLine(x0, y1, z1, x0, y1, z0); // левое верхнее ребро (по Z)
	
	// Вертикальные ребра (по Y)
	addLine(x0, y0, z0, x0, y1, z0); // переднее левое
	addLine(x1, y0, z0, x1, y1, z0); // переднее правое
	addLine(x1, y0, z1, x1, y1, z1); // заднее правое
	addLine(x0, y0, z1, x0, y1, z1); // заднее левое
	
	if (vertices.empty()) {
		return; // Нечего рисовать
	}
	
	// Создаем временный Mesh для линий
	const int lineAttrs[] = {3, 4, 0}; // позиция (3), цвет (4)
	Mesh lineMesh(vertices.data(), vertices.size() / 7, lineAttrs);
	
	// Сохраняем текущее состояние OpenGL
	GLboolean oldCullFace;
	GLboolean oldDepthTest;
	GLboolean oldDepthMask;
	GLboolean oldBlend;
	GLint oldDepthFunc;
	GLfloat oldLineWidth;
	
	glGetBooleanv(GL_CULL_FACE, &oldCullFace);
	glGetBooleanv(GL_DEPTH_TEST, &oldDepthTest);
	// GL_DEPTH_WRITEMASK возвращает GLboolean через glGetBooleanv
	GLboolean depthWriteMask;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteMask);
	oldDepthMask = depthWriteMask;
	glGetBooleanv(GL_BLEND, &oldBlend);
	glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
	glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
	
	// Настраиваем состояние для отрисовки линий
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE); // Не пишем в буфер глубины
	glDepthFunc(GL_LEQUAL); // Контур виден даже если он точно на грани
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(2.5f); // Толщина линии (немного толще для лучшей видимости)
	
	// Используем шейдер для линий
	linesShader->use();
	linesShader->uniformMatrix("u_projview", projview);
	
	// Рисуем линии
	lineMesh.draw(GL_LINES);
	
	// Восстанавливаем предыдущее состояние OpenGL
	glLineWidth(oldLineWidth);
	glDepthMask(oldDepthMask ? GL_TRUE : GL_FALSE);
	glDepthFunc(oldDepthFunc);
	if (oldBlend) glEnable(GL_BLEND);
	else glDisable(GL_BLEND);
	if (oldCullFace) glEnable(GL_CULL_FACE);
	else glDisable(GL_CULL_FACE);
	if (!oldDepthTest) glDisable(GL_DEPTH_TEST);
}

// Функция для отрисовки прицела в центре экрана
void drawCrosshair(Batch2D* batch, Shader* uiShader, int windowWidth, int windowHeight) {
	// Настраиваем UI состояние
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	// Используем UI шейдер
	glActiveTexture(GL_TEXTURE0);
	uiShader->use();
	uiShader->uniform1i("u_texture", 0);
	
	// Устанавливаем ортографическую проекцию для UI (как в Menu)
	mat4 uiProj = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f);
	uiShader->uniformMatrix("u_projview", uiProj);
	
	// Начинаем батч
	batch->begin();
	
	// Размеры прицела
	float crosshairSize = 20.0f;  // Длина линий
	float crosshairThickness = 2.0f;  // Толщина линий
	float crosshairGap = 4.0f;  // Разрыв в центре
	
	// Цвет прицела (белый с небольшой прозрачностью для лучшей видимости)
	batch->color = vec4(1.0f, 1.0f, 1.0f, 0.8f);
	
	// Центр экрана
	float centerX = windowWidth / 2.0f;
	float centerY = windowHeight / 2.0f;
	
	// Вертикальная линия (верхняя часть)
	batch->rect(centerX - crosshairThickness / 2.0f, 
	           centerY - crosshairSize / 2.0f - crosshairGap / 2.0f,
	           crosshairThickness, 
	           crosshairSize / 2.0f - crosshairGap / 2.0f);
	
	// Вертикальная линия (нижняя часть)
	batch->rect(centerX - crosshairThickness / 2.0f, 
	           centerY + crosshairGap / 2.0f,
	           crosshairThickness, 
	           crosshairSize / 2.0f - crosshairGap / 2.0f);
	
	// Горизонтальная линия (левая часть)
	batch->rect(centerX - crosshairSize / 2.0f - crosshairGap / 2.0f,
	           centerY - crosshairThickness / 2.0f,
	           crosshairSize / 2.0f - crosshairGap / 2.0f,
	           crosshairThickness);
	
	// Горизонтальная линия (правая часть)
	batch->rect(centerX + crosshairGap / 2.0f,
	           centerY - crosshairThickness / 2.0f,
	           crosshairSize / 2.0f - crosshairGap / 2.0f,
	           crosshairThickness);
	
	// Рендерим батч
	batch->render();
	
	// Восстанавливаем состояние
	batch->color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

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
	
	// Загружаем шейдер для линий (для контура блока)
	Shader* linesShader = load_shader("res/shaders/lines.glslv", "res/shaders/lines.glslf");
	if (linesShader == nullptr) {
		std::cerr << "failed to load lines shader" << std::endl;
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
	
	// Проверяем существование сохранения (старый способ - больше не используется)
	// Теперь используем систему миров в папке worlds/
	const std::string saveFileName = "world.vxl";
	bool saveExists = files::file_exists(saveFileName);
	// Не устанавливаем флаг saveFileExists на основе старого файла
	// menu.setSaveFileExists(saveExists);
	if (saveExists) {
		std::cout << "[MENU] Old save file found: " << saveFileName << " (ignored, using worlds/ system)" << std::endl;
	} else {
		std::cout << "[MENU] No old save file found" << std::endl;
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
	
	// Путь к текущему миру
	std::string currentWorldPath = "";
	
	// Флаг инициализации мира
	bool worldInitialized = false;
	
	// Радиус загрузки чанков вокруг камеры
	const int renderDistance = 3;
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // Небесный бело-голубой фон

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// Включаем sRGB-коррекцию для правильного отображения цветов
	// Если используешь sRGB, раскомментируй следующую строку и закомментируй pow() в шейдере
	// glEnable(GL_FRAMEBUFFER_SRGB);

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
	
	// Переменные для отрисовки контура блока под курсором
	bool hasTargetBlock = false;
	int targetBlockX = 0;
	int targetBlockY = 0;
	int targetBlockZ = 0;
	vec3 targetBlockNormal(0.0f);

	while (!Window::isShouldClose()) {
		float currentTime = glfwGetTime();
		delta = currentTime - lastTime;
		lastTime = currentTime;
		
		// Обновляем меню
		GameState currentState = menu.update();
		
		// Обновляем список миров при переходе в окно выбора мира
		if (currentState == GameState::WORLD_SELECT && previousState != GameState::WORLD_SELECT) {
			menu.refreshWorldList();
			// Сбрасываем флаг существования сохранения, так как используем систему миров
			menu.setSaveFileExists(false);
		}
		
		// Сбрасываем инициализацию мира при переходе в меню или окно выбора мира
		if ((currentState == GameState::MENU || currentState == GameState::WORLD_SELECT) && 
		    (previousState == GameState::PLAYING || previousState == GameState::PAUSED)) {
			// Сохраняем текущий мир перед выходом
			if (worldInitialized && !currentWorldPath.empty()) {
				float bf, l, g, bh, hv;
				int o;
				chunkManager.getNoiseParams(bf, o, l, g, bh, hv);
				worldSave.save(currentWorldPath, chunkManager, seed, bf, o, l, g, bh, hv);
				std::cout << "[GAME] World saved before exit to menu: " << currentWorldPath << std::endl;
			}
			// Сбрасываем инициализацию мира
			worldInitialized = false;
			currentWorldPath = "";
			chunkManager.clear();
			std::cout << "[GAME] World uninitialized, ready for new world" << std::endl;
		}
		
		previousState = currentState;
		
		// Обрабатываем действия меню
		Menu::MenuAction action = menu.getMenuAction();
		if (action == Menu::MenuAction::CREATE_WORLD) {
			// Создаем новый мир
			// Всегда очищаем старый мир перед созданием нового
			if (worldInitialized) {
				chunkManager.clear();
				worldInitialized = false;
				currentWorldPath = "";
			}
			if (!worldInitialized) {
				// Получаем seed и название мира из меню
				seed = menu.getWorldSeed();
				std::string worldName = menu.getWorldName();
				
				// Формируем имя файла из названия мира (убираем недопустимые символы)
				std::string safeName = worldName;
				for (char& c : safeName) {
					if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
						c = '_';
					}
				}
#ifdef _WIN32
				std::string saveFileName = "worlds\\" + safeName + ".vxl";
#else
				std::string saveFileName = "worlds/" + safeName + ".vxl";
#endif
				currentWorldPath = saveFileName;
				
				// Очищаем старый мир, если был
				chunkManager.clear();
				chunkManager.setSeed(seed); // Устанавливаем seed для генерации мира
				chunkManager.setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
				worldInitialized = true;
				
				// Сохраняем мир сразу после создания, чтобы он появился в списке
				float bf, l, g, bh, hv;
				int o;
				chunkManager.getNoiseParams(bf, o, l, g, bh, hv);
				if (worldSave.save(saveFileName, chunkManager, seed, bf, o, l, g, bh, hv)) {
					std::cout << "[GAME] New world created and saved: name='" << worldName << "', seed=" << seed << ", path=" << saveFileName << std::endl;
					menu.setSaveFileExists(true);
					// Обновляем список миров после создания
					menu.refreshWorldList();
				} else {
					std::cout << "[GAME] New world created but failed to save: name='" << worldName << "', seed=" << seed << ", path=" << saveFileName << std::endl;
					menu.setSaveFileExists(false);
				}
			}
			menu.clearMenuAction();
		} else if (action == Menu::MenuAction::LOAD_WORLD) {
			// Загружаем сохраненный мир
			// Всегда очищаем текущий мир перед загрузкой нового
			if (worldInitialized) {
				chunkManager.clear();
				worldInitialized = false;
				currentWorldPath = "";
			}
			if (!worldInitialized) {
				// Получаем путь к выбранному миру
				std::string saveFileName = menu.getSelectedWorldPath();
				if (saveFileName.empty()) {
					// Если путь пустой, выводим ошибку и не загружаем
					std::cout << "[LOAD] Error: No world selected, cannot load" << std::endl;
					menu.clearMenuAction();
					continue;
				}
				currentWorldPath = saveFileName;
				
				std::cout << "[LOAD] Attempting to load world from: " << saveFileName << std::endl;
				
				// Очищаем текущий мир перед загрузкой
				chunkManager.clear();
				if (worldSave.load(saveFileName, chunkManager, seed, baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation)) {
					std::cout << "[LOAD] World loaded successfully from " << saveFileName << " (seed: " << seed << ")" << std::endl;
					chunkManager.setSeed(seed);
					chunkManager.setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
					worldInitialized = true;
					// Обновляем флаг существования сохранения
					menu.setSaveFileExists(true);
				} else {
					std::cout << "[LOAD] Failed to load world from " << saveFileName << ", creating new world" << std::endl;
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
		bool inputLocked = (currentState == GameState::MENU || currentState == GameState::WORLD_SELECT || currentState == GameState::CREATE_WORLD || currentState == GameState::PAUSED);
		
		// Если игра не инициализирована или в меню/окне выбора мира/окне создания мира, пропускаем игровой цикл
		if (!worldInitialized || currentState == GameState::MENU || currentState == GameState::WORLD_SELECT || currentState == GameState::CREATE_WORLD) {
			// Отрисовываем только меню
			if (currentState == GameState::MENU || currentState == GameState::WORLD_SELECT || currentState == GameState::CREATE_WORLD) {
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
				if (!currentWorldPath.empty()) {
					float bf, l, g, bh, hv;
					int o;
					chunkManager.getNoiseParams(bf, o, l, g, bh, hv);
					if (worldSave.save(currentWorldPath, chunkManager, seed, bf, o, l, g, bh, hv)) {
						std::cout << "[SAVE] World saved successfully to " << currentWorldPath << std::endl;
						// Обновляем флаг существования сохранения
						menu.setSaveFileExists(true);
						// Обновляем список миров
						menu.refreshWorldList();
					} else {
						std::cout << "[SAVE] Failed to save world to " << currentWorldPath << std::endl;
					}
				} else {
					std::cout << "[SAVE] No world path set, cannot save" << std::endl;
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
			// Увеличиваем расстояние raycast для лучшего определения блоков
			voxel* vox = chunkManager.rayCast(camera->position, camera->front, 15.0f, end, norm, iend);
			
			// Сохраняем информацию о блоке под курсором для отрисовки контура
			if (vox != nullptr && vox->id != 0) {
				hasTargetBlock = true;
				// Используем точные координаты из raycast (iend уже содержит целочисленные координаты блока)
				targetBlockX = (int)iend.x;
				targetBlockY = (int)iend.y;
				targetBlockZ = (int)iend.z;
				targetBlockNormal = norm;
			} else {
				hasTargetBlock = false;
			}
			
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
		// Передаем параметры для процедурного текстурирования
		shader->uniform1f("u_baseHeight", baseHeight);
		shader->uniform1f("u_heightVariation", heightVariation);
		// Нормализуем направление света на CPU перед передачей
		vec3 lightDir = normalize(vec3(0.4f, 0.8f, 0.4f));
		shader->uniform3f("u_lightDir", lightDir.x, lightDir.y, lightDir.z);
		
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
		
		// Отрисовываем контур блока под курсором (только во время игры и если есть цель)
		// Используем уже вычисленный projview из предыдущего рендеринга
		if (currentState == GameState::PLAYING && hasTargetBlock) {
			drawBlockOutline(linesShader, projview, targetBlockX, targetBlockY, targetBlockZ, targetBlockNormal);
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
		
		// Отрисовываем прицел во время игры (не в меню и не на паузе)
		if (currentState == GameState::PLAYING) {
			drawCrosshair(batch, uiShader, Window::width, Window::height);
		}
		
		Window::swapBuffers();
		Events::pullEvents();
	}

	// Автосохранение при выходе
	if (worldInitialized && !currentWorldPath.empty()) {
		float bf, l, g, bh, hv;
		int o;
		chunkManager.getNoiseParams(bf, o, l, g, bh, hv);
		if (worldSave.save(currentWorldPath, chunkManager, seed, bf, o, l, g, bh, hv)) {
			std::cout << "[SAVE] Auto-saved world on exit to " << currentWorldPath << std::endl;
			menu.setSaveFileExists(true);
			// Обновляем список миров
			menu.refreshWorldList();
		} else {
			std::cout << "[SAVE] Failed to auto-save world on exit" << std::endl;
		}
	}
	
	delete shader;
	delete voxelShader;
	delete uiShader;
	delete linesShader;
	delete texture;
	delete font;
	delete batch;

	Events::finalize();
	Window::terminate();
	return 0;
}
