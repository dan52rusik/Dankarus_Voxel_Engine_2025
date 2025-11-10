#ifndef WINDOW_EVENTS_H_
#define WINDOW_EVENTS_H_

#include "Window.h"
#include <vector>

typedef unsigned int uint;

class Events {
public:
	static bool* _keys;
	static uint* _frames;
	static uint _current;
	static float deltaX;
	static float deltaY;
	static float x;
	static float y;
	static float scrollX;
	static float scrollY;
	static bool _cursor_locked;
	static bool _cursor_started;
	static std::vector<uint> codepoints;  // Для текстового ввода
	static std::vector<int> pressedKeys;   // Список нажатых клавиш в текущем кадре

	static int initialize();
	static void finalize();  // Очистка памяти
	static void pullEvents();

	static bool pressed(int keycode);
	static bool jpressed(int keycode);

	static bool clicked(int button);
	static bool jclicked(int button);

	static void toggleCursor();  // Исправлено: toggle вместо toogle
};

#endif /* WINDOW_EVENTS_H_ */
