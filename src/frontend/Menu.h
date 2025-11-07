#ifndef FRONTEND_MENU_H_
#define FRONTEND_MENU_H_

#include "GameState.h"
#include <string>

class Menu {
public:
	Menu();
	~Menu();
	
	// Обновление меню (обработка ввода)
	GameState update();
	
	// Отрисовка меню
	void draw();
	
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
	void drawMainMenu();
	
	// Отрисовка меню паузы
	void drawPauseMenu();
};

#endif /* FRONTEND_MENU_H_ */

