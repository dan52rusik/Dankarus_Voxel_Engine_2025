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
	
	// Проверка существования сохранения
	void setSaveFileExists(bool exists);
	bool hasSaveFile() const;
	
	// Получить данные создания мира
	std::string getWorldName() const;
	int getWorldSeed() const;
	
	// Получить выбранный мир из списка
	std::string getSelectedWorldPath() const;
	
	// Обновить список миров
	void refreshWorldList();
	
	// Получить указатель на настройки (для изменения)
	struct GameSettings* getSettings() const { return settings; }
	void setSettings(struct GameSettings* s) { settings = s; }
	
	// Установить указатель на Engine для применения настроек
	void setEngine(class Engine* e) { engine = e; }
	
private:
	GameState currentState;
	MenuAction menuAction;
	int selectedItem; // Выбранный пункт меню (для главного меню)
	bool saveFileExists; // Существует ли сохранение
	UIStyle style; // Стиль UI в стиле Minecraft
	struct GameSettings* settings = nullptr; // Указатель на настройки
	class Engine* engine = nullptr; // Указатель на Engine для применения настроек
	
	// Данные окна создания мира
	std::string worldName; // Название мира
	std::string seedInput; // Введенный seed (строка)
	int worldSeed; // Сгенерированный seed
	int createWorldSelectedItem; // Выбранный пункт в окне создания мира (0 - название, 1 - seed, 2 - создать, 3 - назад)
	bool seedInputActive; // Активно ли поле ввода seed
	
	// Данные окна выбора мира
	std::vector<std::string> worldList; // Список миров (имена файлов)
	int worldSelectSelectedItem; // Выбранный мир в списке
	std::string worldsPath; // Путь к папке worlds
	
	// Данные экрана настроек
	int settingsSelectedItem; // Выбранный пункт в настройках
	GameState previousState; // Состояние до открытия настроек (для возврата)
	
	// Редактируемые значения настроек (копии для редактирования)
	std::string settingsRenderDistInput;
	std::string settingsFovInput;
	std::string settingsFogDistInput;
	bool settingsFullscreen;
	bool settingsVsync;
	bool settingsFogEnabled;
	
	// Отрисовка главного меню
	void drawMainMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Отрисовка меню паузы
	void drawPauseMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Отрисовка окна выбора мира
	void drawWorldSelectMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Отрисовка окна создания мира
	void drawCreateWorldMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Отрисовка экрана настроек
	void drawSettingsMenu(Batch2D* batch, Font* font, Shader* shader, int windowWidth, int windowHeight);
	
	// Вспомогательная функция для отрисовки кнопки
	void drawButton(Batch2D* batch, Font* font, Shader* shader, const std::wstring& text, int x, int y, int width, int height, bool selected);
	
	// Вспомогательная функция для отрисовки текстового поля
	void drawTextField(Batch2D* batch, Font* font, Shader* shader, const std::string& label, const std::string& value, int x, int y, int width, int height, bool selected, bool active);
	
	// Вспомогательная функция для отрисовки слайдера
	void drawSlider(Batch2D* batch, Font* font, Shader* shader, const std::string& label, float value, float min, float max, int x, int y, int width, int height, bool selected);
	
	// Вспомогательные функции для обработки мыши
	int getMouseHoveredItem(int fbWidth, int fbHeight, int itemCount, int buttonX, int buttonY, int buttonW, int buttonH, int buttonGap, int startY);
	void handleMouseInteraction(int windowWidth, int windowHeight);
	
	// Инициализация редактируемых значений настроек
	void initSettingsEditValues();
	
	// Сохранение настроек
	void saveSettingsChanges();
};

#endif /* FRONTEND_MENU_H_ */

