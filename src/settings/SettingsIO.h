#ifndef SETTINGS_SETTINGS_IO_H_
#define SETTINGS_SETTINGS_IO_H_

#include "Settings.h"
#include <string>

class SettingsIO {
public:
    // Загрузить настройки из файла
    static bool loadSettings(const std::string& filename, GameSettings& settings);
    
    // Сохранить настройки в файл
    static bool saveSettings(const std::string& filename, const GameSettings& settings);
    
    // Получить путь к файлу настроек
    static std::string getSettingsPath();
    
    // Получить путь к файлу управления
    static std::string getControlsPath();
};

#endif // SETTINGS_SETTINGS_IO_H_

