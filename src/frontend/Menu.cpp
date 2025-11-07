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

using namespace glm;

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
				case 0: // Create World
					menuAction = MenuAction::CREATE_WORLD;
					currentState = GameState::PLAYING;
					break;
				case 1: // Load World
					menuAction = MenuAction::LOAD_WORLD;
					currentState = GameState::PLAYING;
					break;
				case 2: // Quit
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
		// Меню паузы
		if (Events::jpressed(GLFW_KEY_ESCAPE)) {
			currentState = GameState::PLAYING;
		}
		if (Events::jpressed(GLFW_KEY_UP)) {
			selectedItem = (selectedItem - 1 + 2) % 2;
		}
		if (Events::jpressed(GLFW_KEY_DOWN)) {
			selectedItem = (selectedItem + 1) % 2;
		}
		if (Events::jpressed(GLFW_KEY_ENTER) || Events::jpressed(GLFW_KEY_SPACE)) {
			// Продолжить игру
			if (selectedItem == 0) {
				currentState = GameState::PLAYING;
			}
			// Выход из игры
			else if (selectedItem == 1) {
				menuAction = MenuAction::QUIT;
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
	// Устанавливаем ортографическую проекцию для UI
	mat4 proj = ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, -1.0f, 1.0f);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	
	batch->begin();
	
	// Полупрозрачный фон
	batch->color = vec4(0.0f, 0.0f, 0.0f, 0.7f);
	batch->rect(0, 0, windowWidth, windowHeight);
	
	// Заголовок
	int titleY = windowHeight / 4;
	batch->color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	std::wstring title = L"VoxelNoxel";
	int titleWidth = font->calcWidth(title);
	font->draw(batch, title, (windowWidth - titleWidth) / 2, titleY, STYLE_OUTLINE);
	
	// Кнопки меню
	int buttonY = windowHeight / 2;
	int buttonHeight = 40;
	int buttonSpacing = 60;
	
	drawButton(batch, font, L"Новая игра", windowWidth / 2 - 100, buttonY, 200, buttonHeight, selectedItem == 0);
	drawButton(batch, font, L"Загрузить игру", windowWidth / 2 - 100, buttonY + buttonSpacing, 200, buttonHeight, selectedItem == 1);
	drawButton(batch, font, L"Закрыть", windowWidth / 2 - 100, buttonY + buttonSpacing * 2, 200, buttonHeight, selectedItem == 2);
	
	batch->render();
}

void Menu::drawPauseMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight) {
	// Устанавливаем ортографическую проекцию для UI
	mat4 proj = ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, -1.0f, 1.0f);
	shader->use();
	shader->uniformMatrix("u_projview", proj);
	
	batch->begin();
	
	// Полупрозрачный фон
	batch->color = vec4(0.0f, 0.0f, 0.0f, 0.7f);
	batch->rect(0, 0, windowWidth, windowHeight);
	
	// Заголовок
	int titleY = windowHeight / 3;
	batch->color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	std::wstring title = L"Пауза";
	int titleWidth = font->calcWidth(title);
	font->draw(batch, title, (windowWidth - titleWidth) / 2, titleY, STYLE_OUTLINE);
	
	// Кнопки меню
	int buttonY = windowHeight / 2;
	int buttonHeight = 40;
	int buttonSpacing = 60;
	
	drawButton(batch, font, L"Продолжить", windowWidth / 2 - 100, buttonY, 200, buttonHeight, selectedItem == 0);
	drawButton(batch, font, L"Выход", windowWidth / 2 - 100, buttonY + buttonSpacing, 200, buttonHeight, selectedItem == 1);
	
	batch->render();
}

void Menu::drawButton(Batch2D* batch, Font* font, const std::wstring& text, int x, int y, int width, int height, bool selected) {
	// Фон кнопки
	if (selected) {
		batch->color = vec4(0.2f, 0.4f, 0.6f, 0.9f);
	} else {
		batch->color = vec4(0.1f, 0.1f, 0.1f, 0.8f);
	}
	batch->rect(x, y, width, height);
	
	// Рамка кнопки
	batch->color = selected ? vec4(0.4f, 0.6f, 0.8f, 1.0f) : vec4(0.3f, 0.3f, 0.3f, 1.0f);
	batch->line(x, y, x + width, y, batch->color.r, batch->color.g, batch->color.b, batch->color.a);
	batch->line(x + width, y, x + width, y + height, batch->color.r, batch->color.g, batch->color.b, batch->color.a);
	batch->line(x + width, y + height, x, y + height, batch->color.r, batch->color.g, batch->color.b, batch->color.a);
	batch->line(x, y + height, x, y, batch->color.r, batch->color.g, batch->color.b, batch->color.a);
	
	// Текст кнопки
	batch->color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	int textWidth = font->calcWidth(text);
	int textX = x + (width - textWidth) / 2;
	int textY = y + (height - font->lineHeight()) / 2;
	font->draw(batch, text, textX, textY);
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
