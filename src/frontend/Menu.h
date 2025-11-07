#ifndef FRONTEND_MENU_H_
#define FRONTEND_MENU_H_

#include "GameState.h"
#include <string>

class Batch2D;
class Font;
class Shader;

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
	
	// Отрисовка главного меню
	void drawMainMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Отрисовка меню паузы
	void drawPauseMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Вспомогательная функция для отрисовки кнопки
	void drawButton(Batch2D* batch, Font* font, const std::wstring& text, int x, int y, int width, int height, bool selected);
};

#endif /* FRONTEND_MENU_H_ */

