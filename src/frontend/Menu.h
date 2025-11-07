#ifndef FRONTEND_MENU_H_
#define FRONTEND_MENU_H_

#include "GameState.h"
#include <string>
#include <glm/glm.hpp>

class Batch2D;
class Font;
class Shader;

// Стиль UI в стиле Minecraft
struct UIStyle {
	glm::vec4 bgColor        = {0.10f, 0.13f, 0.16f, 1.0f}; // фон
	glm::vec4 panelColor     = {0.18f, 0.22f, 0.26f, 1.0f}; // тело кнопки
	glm::vec4 bevelLight     = {0.75f, 0.75f, 0.75f, 1.0f}; // светлая фаска (верх/лево)
	glm::vec4 bevelDark      = {0.15f, 0.15f, 0.15f, 1.0f}; // тёмная фаска (низ/право)
	glm::vec4 panelHover     = {0.25f, 0.30f, 0.35f, 1.0f}; // hover/selected
	glm::vec4 textColor      = {1.00f, 1.00f, 1.00f, 1.0f};
	glm::vec4 textDisabled   = {0.60f, 0.60f, 0.60f, 1.0f};

	int buttonW = 360;
	int buttonH = 48;
	int buttonGap = 14;
	int panelPad = 18;
};

class Menu {
public:
	Menu();
	~Menu();
	
	// Обновление меню (обработка ввода)
	GameState update();
	
	// Отрисовка меню
	void draw(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Установка состояния
	void setState(GameState state);
	GameState getState() const;
	
	// Получить выбранное действие из главного меню
	enum class MenuAction {
		NONE,
		CREATE_WORLD,
		LOAD_WORLD,
		QUIT
	};
	
	MenuAction getMenuAction() const;
	void clearMenuAction();
	
private:
	GameState currentState;
	MenuAction menuAction;
	int selectedItem; // Выбранный пункт меню (для главного меню)
	UIStyle style; // Стиль UI в стиле Minecraft
	
	// Отрисовка главного меню
	void drawMainMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Отрисовка меню паузы
	void drawPauseMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Вспомогательная функция для отрисовки кнопки
	void drawButton(Batch2D* batch, Font* font, Shader* shader, const std::wstring& text, int x, int y, int width, int height, bool selected);
};

#endif /* FRONTEND_MENU_H_ */

