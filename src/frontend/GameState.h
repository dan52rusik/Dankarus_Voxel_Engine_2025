#ifndef FRONTEND_GAMESTATE_H_
#define FRONTEND_GAMESTATE_H_

enum class GameState {
	MENU,         // Главное меню
	WORLD_SELECT, // Выбор мира из списка
	CREATE_WORLD, // Окно создания мира
	PLAYING,      // Игра
	PAUSED        // Пауза
};

#endif /* FRONTEND_GAMESTATE_H_ */

