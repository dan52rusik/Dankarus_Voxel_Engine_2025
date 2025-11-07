#include "Menu.h"
#include "../window/Window.h"
#include "../window/Events.h"
#include <GLFW/glfw3.h>
#include <iostream>

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

void Menu::draw() {
	if (currentState == GameState::MENU) {
		drawMainMenu();
	} else if (currentState == GameState::PAUSED) {
		drawPauseMenu();
	}
}

void Menu::drawMainMenu() {
	// Простая текстовая отрисовка главного меню
	// В будущем можно заменить на полноценный GUI
	// Выводим меню в консоль (пока просто для отладки)
	static int lastSelected = -1;
	if (lastSelected != selectedItem) {
		std::cout << "\n=== MAIN MENU ===" << std::endl;
		std::cout << (selectedItem == 0 ? "> " : "  ") << "1. Create World" << std::endl;
		std::cout << (selectedItem == 1 ? "> " : "  ") << "2. Load World" << std::endl;
		std::cout << (selectedItem == 2 ? "> " : "  ") << "3. Quit" << std::endl;
		std::cout << "================\n" << std::endl;
		lastSelected = selectedItem;
	}
}

void Menu::drawPauseMenu() {
	// Простая текстовая отрисовка меню паузы
	// Выводим меню паузы в консоль (пока просто для отладки)
	static int lastSelected = -1;
	if (lastSelected != selectedItem) {
		std::cout << "\n=== PAUSE MENU ===" << std::endl;
		std::cout << (selectedItem == 0 ? "> " : "  ") << "1. Continue" << std::endl;
		std::cout << (selectedItem == 1 ? "> " : "  ") << "2. Quit" << std::endl;
		std::cout << "==================\n" << std::endl;
		lastSelected = selectedItem;
	}
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
