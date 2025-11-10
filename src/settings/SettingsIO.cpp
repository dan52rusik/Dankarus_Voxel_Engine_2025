#include "SettingsIO.h"
#include "../utils/json_simple.h"
#include "../files/files.h"
#include <iostream>
#include <fstream>

std::string SettingsIO::getSettingsPath() {
    return "settings.json";
}

std::string SettingsIO::getControlsPath() {
    return "controls.json";
}

bool SettingsIO::loadSettings(const std::string& filename, GameSettings& settings) {
    if (!files::file_exists(filename)) {
        std::cout << "[SETTINGS] Settings file not found, using defaults" << std::endl;
        // Сохраняем настройки по умолчанию
        saveSettings(filename, settings);
        return false;
    }
    
    try {
        json_simple::Value root = json_simple::Parser::parseFile(filename);
        
        // Загружаем настройки отображения
        if (root.has("display")) {
            json_simple::Value display = root["display"];
            if (display.has("width")) settings.display.width = (int)display["width"].getNumber();
            if (display.has("height")) settings.display.height = (int)display["height"].getNumber();
            if (display.has("fullscreen")) settings.display.fullscreen = display["fullscreen"].getBool();
            if (display.has("swapInterval")) settings.display.swapInterval = (int)display["swapInterval"].getNumber();
        }
        
        // Загружаем настройки графики
        if (root.has("graphics")) {
            json_simple::Value graphics = root["graphics"];
            if (graphics.has("renderDistance")) settings.graphics.renderDistance = graphics["renderDistance"].getInt();
            if (graphics.has("fov")) settings.graphics.fov = (float)graphics["fov"].getNumber();
            if (graphics.has("fogDistance")) settings.graphics.fogDistance = (float)graphics["fogDistance"].getNumber();
            if (graphics.has("fogEnabled")) settings.graphics.fogEnabled = graphics["fogEnabled"].getBool();
        }
        
        // Загружаем настройки управления
        if (root.has("controls")) {
            json_simple::Value controls = root["controls"];
            if (controls.has("keyForward")) settings.controls.keyForward = controls["keyForward"].getInt();
            if (controls.has("keyBackward")) settings.controls.keyBackward = controls["keyBackward"].getInt();
            if (controls.has("keyLeft")) settings.controls.keyLeft = controls["keyLeft"].getInt();
            if (controls.has("keyRight")) settings.controls.keyRight = controls["keyRight"].getInt();
            if (controls.has("keyJump")) settings.controls.keyJump = controls["keyJump"].getInt();
            if (controls.has("keySneak")) settings.controls.keySneak = controls["keySneak"].getInt();
            if (controls.has("keySprint")) settings.controls.keySprint = controls["keySprint"].getInt();
            if (controls.has("keyInventory")) settings.controls.keyInventory = controls["keyInventory"].getInt();
            if (controls.has("keyPause")) settings.controls.keyPause = controls["keyPause"].getInt();
            if (controls.has("keyToggleCursor")) settings.controls.keyToggleCursor = controls["keyToggleCursor"].getInt();
            if (controls.has("mouseButtonPlace")) settings.controls.mouseButtonPlace = controls["mouseButtonPlace"].getInt();
            if (controls.has("mouseButtonBreak")) settings.controls.mouseButtonBreak = controls["mouseButtonBreak"].getInt();
        }
        
        std::cout << "[SETTINGS] Settings loaded from " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SETTINGS] Error loading settings: " << e.what() << std::endl;
        return false;
    }
}

bool SettingsIO::saveSettings(const std::string& filename, const GameSettings& settings) {
    try {
        json_simple::Value root(json_simple::Value::OBJECT);
        
        // Сохраняем настройки отображения
        json_simple::Value display(json_simple::Value::OBJECT);
        display["width"] = json_simple::Value((double)settings.display.width);
        display["height"] = json_simple::Value((double)settings.display.height);
        display["fullscreen"] = json_simple::Value(settings.display.fullscreen);
        display["swapInterval"] = json_simple::Value((double)settings.display.swapInterval);
        root["display"] = display;
        
        // Сохраняем настройки графики
        json_simple::Value graphics(json_simple::Value::OBJECT);
        graphics["renderDistance"] = json_simple::Value((double)settings.graphics.renderDistance);
        graphics["fov"] = json_simple::Value((double)settings.graphics.fov);
        graphics["fogDistance"] = json_simple::Value((double)settings.graphics.fogDistance);
        graphics["fogEnabled"] = json_simple::Value(settings.graphics.fogEnabled);
        root["graphics"] = graphics;
        
        // Сохраняем настройки управления
        json_simple::Value controls(json_simple::Value::OBJECT);
        controls["keyForward"] = json_simple::Value((double)settings.controls.keyForward);
        controls["keyBackward"] = json_simple::Value((double)settings.controls.keyBackward);
        controls["keyLeft"] = json_simple::Value((double)settings.controls.keyLeft);
        controls["keyRight"] = json_simple::Value((double)settings.controls.keyRight);
        controls["keyJump"] = json_simple::Value((double)settings.controls.keyJump);
        controls["keySneak"] = json_simple::Value((double)settings.controls.keySneak);
        controls["keySprint"] = json_simple::Value((double)settings.controls.keySprint);
        controls["keyInventory"] = json_simple::Value((double)settings.controls.keyInventory);
        controls["keyPause"] = json_simple::Value((double)settings.controls.keyPause);
        controls["keyToggleCursor"] = json_simple::Value((double)settings.controls.keyToggleCursor);
        controls["mouseButtonPlace"] = json_simple::Value((double)settings.controls.mouseButtonPlace);
        controls["mouseButtonBreak"] = json_simple::Value((double)settings.controls.mouseButtonBreak);
        root["controls"] = controls;
        
        std::string jsonString = root.toString();
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[SETTINGS] Error opening file for writing: " << filename << std::endl;
            return false;
        }
        file << jsonString;
        file.close();
        
        std::cout << "[SETTINGS] Settings saved to " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SETTINGS] Error saving settings: " << e.what() << std::endl;
        return false;
    }
}

