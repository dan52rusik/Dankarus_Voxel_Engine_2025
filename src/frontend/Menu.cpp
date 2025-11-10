#include "Menu.h"
#include "../window/Window.h"
#include "../window/Events.h"
#include "../graphics/Batch2D.h"
#include "../graphics/Font.h"
#include "../graphics/Shader.h"
#include "../graphics/Texture.h"
#include "../files/files.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <locale>
#include <codecvt>
#include <vector>
#include <random>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace glm;

// Вспомогательные функции для рендера в стиле Minecraft

// Затемнение экрана (виньетка)
static void drawFullscreenTint(Batch2D* b, Shader* shader, int w, int h, glm::vec4 c) {
	glActiveTexture(GL_TEXTURE0);
	b->texture(nullptr); // Устанавливаем blank текстуру (белая 1x1)
	b->color = c;
	b->rect(0, 0, (float)w, (float)h);
}

// Прямоугольник с «майнкрафтовой» фаской
static void drawBevelRect(Batch2D* b, Shader* shader, float x, float y, float w, float h,
                          glm::vec4 body, glm::vec4 light, glm::vec4 dark) {
	glActiveTexture(GL_TEXTURE0);
	b->texture(nullptr); // Устанавливаем blank текстуру (белая 1x1)
	
	// Тело
	b->color = body;
	b->rect(x, y, w, h);
	
	// Верх/лево — светлая грань
	b->color = light;
	b->rect(x, y, w, 2);
	b->rect(x, y, 2, h);
	
	// Низ/право — тёмная грань
	b->color = dark;
	b->rect(x, y + h - 2, w, 2);
	b->rect(x + w - 2, y, 2, h);
}

Menu::Menu() : currentState(GameState::MENU), menuAction(MenuAction::NONE), selectedItem(0), saveFileExists(false),
               worldName("Новый мир"), seedInput(""), worldSeed(1337), createWorldSelectedItem(0), seedInputActive(false),
               worldSelectSelectedItem(0), worldsPath("worlds") {
	// Генерируем случайный seed при первом открытии
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(1, 2147483647);
	worldSeed = dis(gen);
	seedInput = std::to_string(worldSeed);
	
	// Создаем папку worlds, если её нет
	if (!files::directory_exists(worldsPath)) {
		files::create_directory(worldsPath);
	}
	
	// Загружаем список миров
	refreshWorldList();
}

Menu::~Menu() {
}

GameState Menu::update() {
	if (currentState == GameState::MENU) {
		// Главное меню - количество пунктов зависит от наличия сохранения
		int menuItemCount = saveFileExists ? 4 : 3; // "Новая игра", "Продолжить" (если есть), "Настройки", "Выход"
		
		if (Events::jpressed(GLFW_KEY_UP)) {
			selectedItem = (selectedItem - 1 + menuItemCount) % menuItemCount;
		}
		if (Events::jpressed(GLFW_KEY_DOWN)) {
			selectedItem = (selectedItem + 1) % menuItemCount;
		}
		if (Events::jpressed(GLFW_KEY_ENTER) || Events::jpressed(GLFW_KEY_SPACE)) {
			if (saveFileExists) {
				// Меню с сохранением: "Одиночная игра", "Продолжить", "Настройки", "Выход"
				switch (selectedItem) {
					case 0: { // Одиночная игра
						// Открываем окно выбора мира
						refreshWorldList();
						currentState = GameState::WORLD_SELECT;
						worldSelectSelectedItem = 0;
						break;
					}
					case 1: // Продолжить
						menuAction = MenuAction::LOAD_WORLD;
						currentState = GameState::PLAYING;
						break;
					case 2: // Настройки
						// TODO: открыть настройки
						break;
					case 3: // Выход
						menuAction = MenuAction::QUIT;
						break;
				}
			} else {
				// Меню без сохранения: "Одиночная игра", "Настройки", "Выход"
				switch (selectedItem) {
					case 0: { // Одиночная игра
						// Открываем окно выбора мира
						refreshWorldList();
						currentState = GameState::WORLD_SELECT;
						worldSelectSelectedItem = 0;
						break;
					}
					case 1: // Настройки
						// TODO: открыть настройки
						break;
					case 2: // Выход
						menuAction = MenuAction::QUIT;
						break;
				}
			}
		}
	} else if (currentState == GameState::WORLD_SELECT) {
		// Окно выбора мира
		if (Events::jpressed(GLFW_KEY_ESCAPE)) {
			currentState = GameState::MENU;
			worldSelectSelectedItem = 0;
		}
		// Количество пунктов: список миров + кнопка "Создать новый мир"
		// Минимум 1 пункт (кнопка "Создать новый мир")
		int itemCount = std::max(1, (int)worldList.size() + 1);
		if (Events::jpressed(GLFW_KEY_UP)) {
			worldSelectSelectedItem = (worldSelectSelectedItem - 1 + itemCount) % itemCount;
		}
		if (Events::jpressed(GLFW_KEY_DOWN)) {
			worldSelectSelectedItem = (worldSelectSelectedItem + 1) % itemCount;
		}
		if (Events::jpressed(GLFW_KEY_ENTER) || Events::jpressed(GLFW_KEY_SPACE)) {
			if (worldSelectSelectedItem < (int)worldList.size()) {
				// Выбран мир из списка - загружаем его
				std::string selectedPath = getSelectedWorldPath();
				std::cout << "[MENU] Selected world: " << selectedPath << " (index: " << worldSelectSelectedItem << ")" << std::endl;
				menuAction = MenuAction::LOAD_WORLD;
				currentState = GameState::PLAYING;
			} else {
				// Выбрана кнопка "Создать новый мир"
				currentState = GameState::CREATE_WORLD;
				createWorldSelectedItem = 2; // Выбираем кнопку "Создать мир"
				// Генерируем новый случайный seed
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_int_distribution<int> dis(1, 2147483647);
					worldSeed = dis(gen);
					seedInput = std::to_string(worldSeed);
				}
			}
		}
	} else if (currentState == GameState::PLAYING) {
		// Игра - проверяем паузу
		if (Events::jpressed(GLFW_KEY_ESCAPE)) {
			currentState = GameState::PAUSED;
			selectedItem = 0; // Сбрасываем выбор на "Continue"
		}
	} else if (currentState == GameState::CREATE_WORLD) {
		// Окно создания мира (4 пункта: название, seed, создать, назад)
		if (Events::jpressed(GLFW_KEY_ESCAPE)) {
			currentState = GameState::WORLD_SELECT;
			createWorldSelectedItem = 0;
		}
		if (Events::jpressed(GLFW_KEY_UP)) {
			createWorldSelectedItem = (createWorldSelectedItem - 1 + 4) % 4;
		}
		if (Events::jpressed(GLFW_KEY_DOWN)) {
			createWorldSelectedItem = (createWorldSelectedItem + 1) % 4;
		}
		
		// Обработка текстового ввода для названия мира и seed
		if (createWorldSelectedItem == 0) {
			// Поле названия мира активно
			// Обрабатываем ввод символов
			if (!Events::codepoints.empty()) {
				for (uint32_t codepoint : Events::codepoints) {
					if (codepoint >= 32 && codepoint < 127) {
						// Печатаемые ASCII символы
						if (worldName.size() < 32) { // Ограничение длины
							worldName += (char)codepoint;
						}
					}
				}
			}
			// Обрабатываем Backspace
			if (Events::jpressed(GLFW_KEY_BACKSPACE) && !worldName.empty()) {
				worldName.pop_back();
			}
		} else if (createWorldSelectedItem == 1) {
			// Поле seed активно
			// Обрабатываем ввод символов (только цифры и знак минус)
			if (!Events::codepoints.empty()) {
				for (uint32_t codepoint : Events::codepoints) {
					if ((codepoint >= '0' && codepoint <= '9') || (codepoint == '-' && seedInput.empty())) {
						if (seedInput.size() < 15) { // Ограничение длины
							seedInput += (char)codepoint;
						}
					}
				}
			}
			// Обрабатываем Backspace
			if (Events::jpressed(GLFW_KEY_BACKSPACE) && !seedInput.empty()) {
				seedInput.pop_back();
			}
			// При нажатии Enter/Space на поле seed - генерируем новый случайный seed
			if (Events::jpressed(GLFW_KEY_ENTER) || Events::jpressed(GLFW_KEY_SPACE)) {
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_int_distribution<int> dis(1, 2147483647);
					worldSeed = dis(gen);
					seedInput = std::to_string(worldSeed);
				}
			}
		}
		
		if (Events::jpressed(GLFW_KEY_ENTER) || Events::jpressed(GLFW_KEY_SPACE)) {
			if (createWorldSelectedItem == 2) {
				// Создать мир
				// Проверяем, что название мира не пустое
				if (worldName.empty() || worldName == "Новый мир") {
					// Если название пустое или по умолчанию, используем seed как название
					worldName = seedInput.empty() ? "Новый мир" : seedInput;
				}
				// Парсим seed из строки
				try {
					worldSeed = std::stoi(seedInput);
				} catch (...) {
					// Если не удалось распарсить, используем хеш строки
					std::hash<std::string> hasher;
					worldSeed = (int)(hasher(seedInput) & 0x7FFFFFFF);
					if (worldSeed == 0) worldSeed = 1337;
				}
				std::cout << "[MENU] Creating world with name: '" << worldName << "', seed: " << worldSeed << std::endl;
				menuAction = MenuAction::CREATE_WORLD;
				currentState = GameState::PLAYING;
			} else if (createWorldSelectedItem == 3) {
				// Назад - возвращаемся к выбору мира
				currentState = GameState::WORLD_SELECT;
				createWorldSelectedItem = 0;
			}
		}
	} else if (currentState == GameState::PAUSED) {
		// Меню паузы (теперь 3 пункта)
		if (Events::jpressed(GLFW_KEY_ESCAPE)) {
			currentState = GameState::PLAYING;
		}
		if (Events::jpressed(GLFW_KEY_UP)) {
			selectedItem = (selectedItem - 1 + 3) % 3;
		}
		if (Events::jpressed(GLFW_KEY_DOWN)) {
			selectedItem = (selectedItem + 1) % 3;
		}
		if (Events::jpressed(GLFW_KEY_ENTER) || Events::jpressed(GLFW_KEY_SPACE)) {
			// Вернуться в игру
			if (selectedItem == 0) {
				currentState = GameState::PLAYING;
			}
			// Настройки (пока ничего не делаем)
			else if (selectedItem == 1) {
				// TODO: открыть настройки
			}
			// Главное меню
			else if (selectedItem == 2) {
				currentState = GameState::MENU;
				selectedItem = 0;
			}
		}
	}
	
	return currentState;
}

void Menu::draw(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight) {
	if (currentState == GameState::MENU) {
		drawMainMenu(batch, font, shader, windowWidth, windowHeight);
	} else if (currentState == GameState::WORLD_SELECT) {
		drawWorldSelectMenu(batch, font, shader, windowWidth, windowHeight);
	} else if (currentState == GameState::CREATE_WORLD) {
		drawCreateWorldMenu(batch, font, shader, windowWidth, windowHeight);
	} else if (currentState == GameState::PAUSED) {
		drawPauseMenu(batch, font, shader, windowWidth, windowHeight);
	}
}

void Menu::drawMainMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight) {
	// Используем размеры framebuffer для ортографической проекции (для HiDPI)
	int fbWidth = Window::fbWidth > 0 ? Window::fbWidth : windowWidth;
	int fbHeight = Window::fbHeight > 0 ? Window::fbHeight : windowHeight;
	
	// Устанавливаем ортографическую проекцию для UI
	mat4 proj = ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f, -1.0f, 1.0f);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	
	// PASS 1: Панели
	glActiveTexture(GL_TEXTURE0);
	batch->begin();
	
	// Затемняем фон (виньетка)
	drawFullscreenTint(batch, shader, fbWidth, fbHeight, vec4(0.0f, 0.0f, 0.0f, 0.35f));
	
	// Пункты как в Minecraft (только панели, без текста)
	// Количество пунктов зависит от наличия сохранения
		std::vector<std::wstring> items;
		if (saveFileExists) {
			items = {
				L"Одиночная игра",
				L"Продолжить",
				L"Настройки",
				L"Выход"
			};
		} else {
			items = {
				L"Одиночная игра",
				L"Настройки",
				L"Выход"
			};
		}
	
	int w = style.buttonW;
	int h = style.buttonH;
	int x = (fbWidth - w) / 2;
	int tY = (int)(fbHeight / 5);
	int y0 = tY + 80;
	
	for (size_t i = 0; i < items.size(); ++i) {
		int y = y0 + (int)i * (h + style.buttonGap);
		bool isSel = (int)i == selectedItem;
		// Рисуем только панель кнопки (без текста)
		glm::vec4 body   = isSel ? style.panelHover : style.panelColor;
		glm::vec4 light  = isSel ? style.bevelDark  : style.bevelLight;
		glm::vec4 dark   = isSel ? style.bevelLight : style.bevelDark;
		drawBevelRect(batch, shader, (float)x, (float)y, (float)w, (float)h, body, light, dark);
	}
	
	batch->render();
	
	// PASS 2: Текст
	// Устанавливаем шейдер и uniform один раз для всего UI-пасса
	glActiveTexture(GL_TEXTURE0);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	shader->uniform1i("u_texture", 0);  // устанавливаем один раз на весь пасс
	
	// ОТЛАДКА: Проверка атласа font_4.png (page=4)
	// Раскомментируйте для проверки содержимого атласа
	// Должны быть видны кириллические буквы в диапазонах 0xC0..0xFF
	// В строках 12-13 (0xC0..0xDF) должны быть А-Я
	// В строках 14-15 (0xE0..0xFF) должны быть а-я
	// Если видите латиницу - атлас нужно пересобрать
	/*
	batch->color = style.textColor;
	std::wstring row;
	for (int r = 0; r < 16; ++r) {
		row.clear();
		for (int c = 0; c < 16; ++c) {
			// Создаем байт напрямую (0x00..0xFF)
			unsigned char byte = (unsigned char)((r << 4) | c);
			// Для проверки атласа используем Unicode символы, которые маппятся в CP1251
			// Для кириллицы (0xC0-0xFF) используем соответствующие Unicode символы
			wchar_t ch;
			if (byte >= 0xC0 && byte <= 0xDF) {
				// А-Я: маппим CP1251 → Unicode
				ch = (wchar_t)(0x0410 + (byte - 0xC0)); // U+0410 (А) + offset
			} else if (byte >= 0xE0 && byte <= 0xFF) {
				// а-я: маппим CP1251 → Unicode
				ch = (wchar_t)(0x0430 + (byte - 0xE0)); // U+0430 (а) + offset
			} else if (byte == 0xA8) {
				ch = 0x0401; // Ё
			} else if (byte == 0xB8) {
				ch = 0x0451; // ё
			} else {
				// Для остальных байтов используем прямой маппинг
				ch = (wchar_t)byte;
			}
			// Маппим в page=4 для кириллицы
			ch = (wchar_t)((4 << 8) | (ch & 0xFFFF));
			row.push_back(ch);
		}
		font->draw(batch, shader, row, 8, 8 + r*18, STYLE_NONE);
	}
	*/
	
	// Заголовок (округляем до целых для четкости)
	const std::wstring title = L"Voxel Noxel";
	int tW = font->calcWidth(title);
	int tX = (int)((fbWidth - tW) / 2);
	batch->color = style.textColor;
	font->draw(batch, shader, title, tX, tY, STYLE_SHADOW);
	
	// Текст на кнопках
	for (size_t i = 0; i < items.size(); ++i) {
		int y = y0 + (int)i * (h + style.buttonGap);
		int tw = font->calcWidth(items[i]);
		int th = font->lineHeight();
		int tx = (int)(x + (w - tw) / 2);
		int ty = (int)(y + (h - th) / 2 + 1);
		
		batch->color = style.textColor;
		font->draw(batch, shader, items[i], tx, ty, STYLE_SHADOW);
	}
}

void Menu::drawPauseMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight) {
	// Используем размеры framebuffer для ортографической проекции (для HiDPI)
	int fbWidth = Window::fbWidth > 0 ? Window::fbWidth : windowWidth;
	int fbHeight = Window::fbHeight > 0 ? Window::fbHeight : windowHeight;
	
	// Устанавливаем ортографическую проекцию для UI
	mat4 proj = ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f, -1.0f, 1.0f);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	
	// PASS 1: Панели
	glActiveTexture(GL_TEXTURE0);
	batch->begin();
	
	// Затемняем фон (виньетка)
	drawFullscreenTint(batch, shader, fbWidth, fbHeight, vec4(0.0f, 0.0f, 0.0f, 0.45f));
	
	// Пункты как в Minecraft (только панели, без текста)
	std::vector<std::wstring> items = {
		L"Вернуться в игру",
		L"Настройки",
		L"Главное меню"
	};
	
	int w = style.buttonW;
	int h = style.buttonH;
	int x = (fbWidth - w) / 2;
	int tY = (int)(fbHeight / 5);
	int y0 = tY + 80;
	
	for (size_t i = 0; i < items.size(); ++i) {
		int y = y0 + (int)i * (h + style.buttonGap);
		bool isSel = (int)i == selectedItem;
		// Рисуем только панель кнопки (без текста)
		glm::vec4 body   = isSel ? style.panelHover : style.panelColor;
		glm::vec4 light  = isSel ? style.bevelDark  : style.bevelLight;
		glm::vec4 dark   = isSel ? style.bevelLight : style.bevelDark;
		drawBevelRect(batch, shader, (float)x, (float)y, (float)w, (float)h, body, light, dark);
	}
	
	batch->render();
	
	// PASS 2: Текст
	// Устанавливаем шейдер и uniform один раз для всего UI-пасса
	glActiveTexture(GL_TEXTURE0);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	shader->uniform1i("u_texture", 0);  // устанавливаем один раз на весь пасс
	
	// Заголовок (округляем до целых для четкости)
	const std::wstring title = L"Пауза";
	int tW = font->calcWidth(title);
	int tX = (int)((fbWidth - tW) / 2);
	batch->color = style.textColor;
	font->draw(batch, shader, title, tX, tY, STYLE_SHADOW);
	
	// Текст на кнопках
	for (size_t i = 0; i < items.size(); ++i) {
		int y = y0 + (int)i * (h + style.buttonGap);
		int tw = font->calcWidth(items[i]);
		int th = font->lineHeight();
		int tx = (int)(x + (w - tw) / 2);
		int ty = (int)(y + (h - th) / 2 + 1);
		batch->color = style.textColor;
		font->draw(batch, shader, items[i], tx, ty, STYLE_SHADOW);
	}
}

void Menu::drawButton(Batch2D* batch, Font* font, Shader* shader, const std::wstring& text,
                      int x, int y, int w, int h, bool selected) {
	// Панели
	glActiveTexture(GL_TEXTURE0);
	glm::vec4 body   = selected ? style.panelHover : style.panelColor;
	glm::vec4 light  = selected ? style.bevelDark  : style.bevelLight; // инверсия фаски при hover
	glm::vec4 dark   = selected ? style.bevelLight : style.bevelDark;

	drawBevelRect(batch, shader, (float)x, (float)y, (float)w, (float)h, body, light, dark);

	// Текст по центру с пиксельной тенью (округляем до целых для четкости)
	int tw = font->calcWidth(text);
	int th = font->lineHeight();
	int tx = (int)(x + (w - tw) / 2);
	int ty = (int)(y + (h - th) / 2 + 1);

	// Minecraft-like shadow (STYLE_SHADOW)
	// Шейдер и uniform должны быть установлены вызывающим кодом
	batch->color = style.textColor;
	font->draw(batch, shader, text, tx, ty, STYLE_SHADOW);
}

// Вспомогательная функция для конвертации string в wstring
static std::wstring stringToWstring(const std::string& str) {
	if (str.empty()) return std::wstring();
#ifdef _WIN32
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
#else
	// Для других платформ используем codecvt
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(str);
#endif
}

void Menu::drawTextField(Batch2D* batch, Font* font, Shader* shader, const std::string& label, const std::string& value,
                         int x, int y, int width, int height, bool selected, bool active) {
	// Рисуем панель текстового поля
	glActiveTexture(GL_TEXTURE0);
	glm::vec4 body   = selected ? style.panelHover : style.panelColor;
	glm::vec4 light  = selected ? style.bevelDark  : style.bevelLight;
	glm::vec4 dark   = selected ? style.bevelLight : style.bevelDark;
	
	drawBevelRect(batch, shader, (float)x, (float)y, (float)width, (float)height, body, light, dark);
	
	// Конвертируем label и value в wstring
	std::wstring wLabel = stringToWstring(label);
	std::wstring wValue = stringToWstring(value);
	
	// Рисуем label слева
	batch->color = style.textColor;
	int labelW = font->calcWidth(wLabel);
	font->draw(batch, shader, wLabel, x + 8, y + (height - font->lineHeight()) / 2 + 1, STYLE_SHADOW);
	
	// Рисуем value справа
	int valueW = font->calcWidth(wValue);
	int valueX = x + width - valueW - 8;
	font->draw(batch, shader, wValue, valueX, y + (height - font->lineHeight()) / 2 + 1, STYLE_SHADOW);
}

void Menu::drawCreateWorldMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight) {
	// Используем размеры framebuffer для ортографической проекции (для HiDPI)
	int fbWidth = Window::fbWidth > 0 ? Window::fbWidth : windowWidth;
	int fbHeight = Window::fbHeight > 0 ? Window::fbHeight : windowHeight;
	
	// Устанавливаем ортографическую проекцию для UI
	mat4 proj = ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f, -1.0f, 1.0f);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	
	// PASS 1: Панели
	glActiveTexture(GL_TEXTURE0);
	batch->begin();
	
	// Затемняем фон (виньетка)
	drawFullscreenTint(batch, shader, fbWidth, fbHeight, vec4(0.0f, 0.0f, 0.0f, 0.35f));
	
	// Панель окна создания мира
	int panelW = 500;
	int panelH = 300;
	int panelX = (fbWidth - panelW) / 2;
	int panelY = (fbHeight - panelH) / 2;
	drawBevelRect(batch, shader, (float)panelX, (float)panelY, (float)panelW, (float)panelH,
	              style.panelColor, style.bevelLight, style.bevelDark);
	
	// Поля и кнопки
	int w = 460;
	int h = style.buttonH;
	int x = panelX + (panelW - w) / 2;
	int y0 = panelY + 60;
	int gap = style.buttonGap;
	
	// Поле названия мира (индекс 0)
	int y1 = y0;
	bool sel1 = (createWorldSelectedItem == 0);
	drawTextField(batch, font, shader, "Название мира:", worldName, x, y1, w, h, sel1, false);
	
	// Поле seed (индекс 1)
	int y2 = y1 + h + gap;
	bool sel2 = (createWorldSelectedItem == 1);
	drawTextField(batch, font, shader, "Seed:", seedInput, x, y2, w, h, sel2, false);
	
	// Кнопка "Создать мир" (индекс 2)
	int y3 = y2 + h + gap + 20;
	bool sel3 = (createWorldSelectedItem == 2);
	drawBevelRect(batch, shader, (float)x, (float)y3, (float)w, (float)h,
	              sel3 ? style.panelHover : style.panelColor,
	              sel3 ? style.bevelDark : style.bevelLight,
	              sel3 ? style.bevelLight : style.bevelDark);
	
	// Кнопка "Назад" (индекс 3)
	int y4 = y3 + h + gap;
	bool sel4 = (createWorldSelectedItem == 3);
	drawBevelRect(batch, shader, (float)x, (float)y4, (float)w, (float)h,
	              sel4 ? style.panelHover : style.panelColor,
	              sel4 ? style.bevelDark : style.bevelLight,
	              sel4 ? style.bevelLight : style.bevelDark);
	
	batch->render();
	
	// PASS 2: Текст
	glActiveTexture(GL_TEXTURE0);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	shader->uniform1i("u_texture", 0);
	
	// Заголовок
	const std::wstring title = L"Создание мира";
	int tW = font->calcWidth(title);
	int tX = (int)((fbWidth - tW) / 2);
	batch->color = style.textColor;
	font->draw(batch, shader, title, tX, panelY + 20, STYLE_SHADOW);
	
	// Текст на полях
	drawTextField(batch, font, shader, "Название мира:", worldName, x, y1, w, h, sel1, false);
	drawTextField(batch, font, shader, "Seed:", seedInput, x, y2, w, h, sel2, false);
	
	// Текст на кнопках
	const std::wstring createText = L"Создать мир";
	int createTW = font->calcWidth(createText);
	int createTX = (int)(x + (w - createTW) / 2);
	int createTY = (int)(y3 + (h - font->lineHeight()) / 2 + 1);
	batch->color = style.textColor;
	font->draw(batch, shader, createText, createTX, createTY, STYLE_SHADOW);
	
	const std::wstring backText = L"Назад";
	int backTW = font->calcWidth(backText);
	int backTX = (int)(x + (w - backTW) / 2);
	int backTY = (int)(y4 + (h - font->lineHeight()) / 2 + 1);
	batch->color = style.textColor;
	font->draw(batch, shader, backText, backTX, backTY, STYLE_SHADOW);
}

std::string Menu::getWorldName() const {
	return worldName;
}

int Menu::getWorldSeed() const {
	return worldSeed;
}

std::string Menu::getSelectedWorldPath() const {
	if (worldSelectSelectedItem >= 0 && worldSelectSelectedItem < (int)worldList.size()) {
#ifdef _WIN32
		std::string path = worldsPath + "\\" + worldList[worldSelectSelectedItem];
#else
		std::string path = worldsPath + "/" + worldList[worldSelectSelectedItem];
#endif
		std::cout << "[MENU] getSelectedWorldPath: index=" << worldSelectSelectedItem << ", world=" << worldList[worldSelectSelectedItem] << ", path=" << path << std::endl;
		return path;
	}
	std::cout << "[MENU] getSelectedWorldPath: invalid index " << worldSelectSelectedItem << " (list size: " << worldList.size() << ")" << std::endl;
	return "";
}

void Menu::refreshWorldList() {
	// Проверяем, существует ли папка worlds
	if (!files::directory_exists(worldsPath)) {
		files::create_directory(worldsPath);
		std::cout << "[MENU] Created worlds directory: " << worldsPath << std::endl;
	}
	
	worldList = files::list_files(worldsPath, ".vxl");
	std::cout << "[MENU] Found " << worldList.size() << " world(s) in " << worldsPath << std::endl;
	for (const auto& world : worldList) {
		std::cout << "[MENU]   - " << world << std::endl;
	}
	
	// Ограничиваем выбор, если список изменился
	if (worldSelectSelectedItem >= (int)worldList.size()) {
		worldSelectSelectedItem = std::max(0, (int)worldList.size() - 1);
	}
}

void Menu::drawWorldSelectMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight) {
	// Используем размеры framebuffer для ортографической проекции (для HiDPI)
	int fbWidth = Window::fbWidth > 0 ? Window::fbWidth : windowWidth;
	int fbHeight = Window::fbHeight > 0 ? Window::fbHeight : windowHeight;
	
	// Устанавливаем ортографическую проекцию для UI
	mat4 proj = ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f, -1.0f, 1.0f);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	
	// PASS 1: Панели
	glActiveTexture(GL_TEXTURE0);
	batch->begin();
	
	// Затемняем фон (виньетка)
	drawFullscreenTint(batch, shader, fbWidth, fbHeight, vec4(0.0f, 0.0f, 0.0f, 0.35f));
	
	// Панель окна выбора мира
	int panelW = 500;
	int panelH = std::min(500, 100 + (int)worldList.size() * (style.buttonH + style.buttonGap) + style.buttonH + 40);
	int panelX = (fbWidth - panelW) / 2;
	int panelY = (fbHeight - panelH) / 2;
	drawBevelRect(batch, shader, (float)panelX, (float)panelY, (float)panelW, (float)panelH,
	              style.panelColor, style.bevelLight, style.bevelDark);
	
	// Поля и кнопки
	int w = 460;
	int h = style.buttonH;
	int x = panelX + (panelW - w) / 2;
	int y0 = panelY + 60;
	int gap = style.buttonGap;
	
	// Список миров
	for (size_t i = 0; i < worldList.size(); ++i) {
		int y = y0 + (int)i * (h + gap);
		bool sel = ((int)i == worldSelectSelectedItem);
		drawBevelRect(batch, shader, (float)x, (float)y, (float)w, (float)h,
		              sel ? style.panelHover : style.panelColor,
		              sel ? style.bevelDark : style.bevelLight,
		              sel ? style.bevelLight : style.bevelDark);
	}
	
	// Кнопка "Создать новый мир" (внизу)
	// Если список пуст, кнопка находится на позиции y0
	int createY = worldList.empty() ? y0 : (y0 + (int)worldList.size() * (h + gap) + 20);
	bool selCreate = (worldSelectSelectedItem == (int)worldList.size());
	drawBevelRect(batch, shader, (float)x, (float)createY, (float)w, (float)h,
	              selCreate ? style.panelHover : style.panelColor,
	              selCreate ? style.bevelDark : style.bevelLight,
	              selCreate ? style.bevelLight : style.bevelDark);
	
	batch->render();
	
	// PASS 2: Текст
	glActiveTexture(GL_TEXTURE0);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	shader->uniform1i("u_texture", 0);
	
	// Заголовок
	const std::wstring title = L"Выбор мира";
	int tW = font->calcWidth(title);
	int tX = (int)((fbWidth - tW) / 2);
	batch->color = style.textColor;
	font->draw(batch, shader, title, tX, panelY + 20, STYLE_SHADOW);
	
	// Текст на кнопках миров
	for (size_t i = 0; i < worldList.size(); ++i) {
		int y = y0 + (int)i * (h + gap);
		std::wstring worldName = stringToWstring(worldList[i]);
		// Убираем расширение .vxl
		if (worldName.size() >= 4 && worldName.substr(worldName.size() - 4) == L".vxl") {
			worldName = worldName.substr(0, worldName.size() - 4);
		}
		int tw = font->calcWidth(worldName);
		int tx = (int)(x + (w - tw) / 2);
		int ty = (int)(y + (h - font->lineHeight()) / 2 + 1);
		batch->color = style.textColor;
		font->draw(batch, shader, worldName, tx, ty, STYLE_SHADOW);
	}
	
	// Текст на кнопке "Создать новый мир"
	const std::wstring createText = L"Создать новый мир";
	int createTW = font->calcWidth(createText);
	int createTX = (int)(x + (w - createTW) / 2);
	int createTY = (int)(createY + (h - font->lineHeight()) / 2 + 1);
	batch->color = style.textColor;
	font->draw(batch, shader, createText, createTX, createTY, STYLE_SHADOW);
}

void Menu::setState(GameState state) {
	currentState = state;
}

GameState Menu::getState() const {
	return currentState;
}

Menu::MenuAction Menu::getMenuAction() const {
	return menuAction;
}

void Menu::clearMenuAction() {
	menuAction = MenuAction::NONE;
}

void Menu::setSaveFileExists(bool exists) {
	saveFileExists = exists;
}

bool Menu::hasSaveFile() const {
	return saveFileExists;
}
