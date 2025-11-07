#include "Menu.h"
#include "../window/Window.h"
#include "../window/Events.h"
#include "../graphics/Batch2D.h"
#include "../graphics/Font.h"
#include "../graphics/Shader.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <locale>
#include <codecvt>
#include <vector>

using namespace glm;

// Вспомогательные функции для рендера в стиле Minecraft

// Затемнение экрана (виньетка)
static void drawFullscreenTint(Batch2D* b, Shader* shader, int w, int h, glm::vec4 c) {
	shader->uniform1i("u_useTex", 0); // БЕЗ текстуры для панелей
	b->texture(nullptr); // Устанавливаем blank текстуру (белая 1x1)
	b->color = c;
	b->rect(0, 0, (float)w, (float)h);
}

// Прямоугольник с «майнкрафтовой» фаской
static void drawBevelRect(Batch2D* b, Shader* shader, float x, float y, float w, float h,
                          glm::vec4 body, glm::vec4 light, glm::vec4 dark) {
	shader->uniform1i("u_useTex", 0); // БЕЗ текстуры для панелей
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

Menu::Menu() : currentState(GameState::MENU), menuAction(MenuAction::NONE), selectedItem(0) {
}

Menu::~Menu() {
}

GameState Menu::update() {
	if (currentState == GameState::MENU) {
		// Главное меню
		if (Events::jpressed(GLFW_KEY_UP)) {
			selectedItem = (selectedItem - 1 + 3) % 3;
		}
		if (Events::jpressed(GLFW_KEY_DOWN)) {
			selectedItem = (selectedItem + 1) % 3;
		}
		if (Events::jpressed(GLFW_KEY_ENTER) || Events::jpressed(GLFW_KEY_SPACE)) {
			switch (selectedItem) {
				case 0: // Один игрок (создаём/загружаем мир)
					menuAction = MenuAction::LOAD_WORLD; // Сначала пытаемся загрузить, если нет - создаём
					currentState = GameState::PLAYING;
					break;
				case 1: // Настройки
					// TODO: открыть настройки
					break;
				case 2: // Выход
					menuAction = MenuAction::QUIT;
					break;
			}
		}
	} else if (currentState == GameState::PLAYING) {
		// Игра - проверяем паузу
		if (Events::jpressed(GLFW_KEY_ESCAPE)) {
			currentState = GameState::PAUSED;
			selectedItem = 0; // Сбрасываем выбор на "Continue"
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
	shader->uniform1i("u_useTex", 0); // по умолчанию панели — без текстуры
	
	batch->begin();
	
	// Затемняем фон (виньетка)
	drawFullscreenTint(batch, shader, fbWidth, fbHeight, vec4(0.0f, 0.0f, 0.0f, 0.35f));
	
	// Заголовок
	const std::wstring title = L"Voxel Noxel";
	int tW = font->calcWidth(title);
	int tX = (fbWidth - tW) / 2;
	int tY = fbHeight / 5; // повыше центра
	shader->uniform1i("u_useTex", 1); // ВКЛЮЧАЕМ текстуру для текста
	batch->color = style.textColor;
	font->draw(batch, title, tX, tY, STYLE_SHADOW);
	shader->uniform1i("u_useTex", 0); // ВОЗВРАЩАЕМСЯ к панелям
	batch->texture(nullptr); // Сбрасываем текстуру на blank (белая 1x1)
	
	// Пункты как в Minecraft (используем ASCII для совместимости)
	std::vector<std::wstring> items = {
		L"Single Player",
		L"Settings",
		L"Quit Game"
	};
	
	int w = style.buttonW;
	int h = style.buttonH;
	int x = (fbWidth - w) / 2;
	int y0 = tY + 80;
	
	for (size_t i = 0; i < items.size(); ++i) {
		int y = y0 + (int)i * (h + style.buttonGap);
		bool isSel = (int)i == selectedItem;
		drawButton(batch, font, shader, items[i], x, y, w, h, isSel);
	}
	
	// Убеждаемся, что флаг установлен правильно перед рендерингом
	shader->uniform1i("u_useTex", 0); // панели без текстуры
	batch->render();
}

void Menu::drawPauseMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight) {
	// Используем размеры framebuffer для ортографической проекции (для HiDPI)
	int fbWidth = Window::fbWidth > 0 ? Window::fbWidth : windowWidth;
	int fbHeight = Window::fbHeight > 0 ? Window::fbHeight : windowHeight;
	
	// Устанавливаем ортографическую проекцию для UI
	mat4 proj = ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f, -1.0f, 1.0f);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	shader->uniform1i("u_useTex", 0); // по умолчанию панели — без текстуры
	
	batch->begin();
	
	// Затемняем фон (виньетка)
	drawFullscreenTint(batch, shader, fbWidth, fbHeight, vec4(0.0f, 0.0f, 0.0f, 0.45f));
	
	// Заголовок
	const std::wstring title = L"Game Menu";
	int tW = font->calcWidth(title);
	int tX = (fbWidth - tW) / 2;
	int tY = fbHeight / 5;
	shader->uniform1i("u_useTex", 1); // ВКЛЮЧАЕМ текстуру для текста
	batch->color = style.textColor;
	font->draw(batch, title, tX, tY, STYLE_SHADOW);
	shader->uniform1i("u_useTex", 0); // ВОЗВРАЩАЕМСЯ к панелям
	batch->texture(nullptr); // Сбрасываем текстуру на blank (белая 1x1)
	
	// Пункты как в Minecraft (используем ASCII для совместимости)
	std::vector<std::wstring> items = {
		L"Back to Game",
		L"Settings",
		L"Main Menu"
	};
	
	int w = style.buttonW;
	int h = style.buttonH;
	int x = (fbWidth - w) / 2;
	int y0 = tY + 80;
	
	for (size_t i = 0; i < items.size(); ++i) {
		int y = y0 + (int)i * (h + style.buttonGap);
		bool isSel = (int)i == selectedItem;
		drawButton(batch, font, shader, items[i], x, y, w, h, isSel);
	}
	
	// Убеждаемся, что флаг установлен правильно перед рендерингом
	shader->uniform1i("u_useTex", 0); // панели без текстуры
	batch->render();
}

void Menu::drawButton(Batch2D* batch, Font* font, Shader* shader, const std::wstring& text,
                      int x, int y, int w, int h, bool selected) {
	glm::vec4 body   = selected ? style.panelHover : style.panelColor;
	glm::vec4 light  = selected ? style.bevelDark  : style.bevelLight; // инверсия фаски при hover
	glm::vec4 dark   = selected ? style.bevelLight : style.bevelDark;

	drawBevelRect(batch, shader, (float)x, (float)y, (float)w, (float)h, body, light, dark);

	// Текст по центру с пиксельной тенью
	int tw = font->calcWidth(text);
	int th = font->lineHeight();
	int tx = x + (w - tw) / 2;
	int ty = y + (h - th) / 2 + 1;

	// Minecraft-like shadow (STYLE_SHADOW)
	// Включаем текстуру для текста
	shader->uniform1i("u_useTex", 1); // ВКЛЮЧАЕМ текстуру для текста
	batch->color = style.textColor;
	font->draw(batch, text, tx, ty, STYLE_SHADOW);
	shader->uniform1i("u_useTex", 0); // ВОЗВРАЩАЕМСЯ к панелям
	batch->texture(nullptr); // Сбрасываем текстуру на blank (белая 1x1)
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
