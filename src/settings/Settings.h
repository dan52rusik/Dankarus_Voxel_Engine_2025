#ifndef SETTINGS_SETTINGS_H_
#define SETTINGS_SETTINGS_H_

struct DisplaySettings {
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    int swapInterval = 1; // 0 = unlimited fps, 1 = vsync
};

struct GraphicsSettings {
    int renderDistance = 3; // Дальность прорисовки чанков
    float fov = 90.0f; // FOV камеры (в градусах)
    float fogDistance = 500.0f; // Дальность тумана
    bool fogEnabled = true;
};

struct ControlSettings {
    // Клавиши управления (GLFW key codes)
    int keyForward = 87;      // W
    int keyBackward = 83;     // S
    int keyLeft = 65;         // A
    int keyRight = 68;        // D
    int keyJump = 32;         // SPACE
    int keySneak = 340;       // LEFT SHIFT
    int keySprint = 341;      // LEFT CONTROL
    int keyInventory = 69;    // E
    int keyPause = 256;       // ESCAPE
    int keyToggleCursor = 258;// TAB
    
    // Кнопки мыши
    int mouseButtonPlace = 1;  // RIGHT BUTTON
    int mouseButtonBreak = 0;  // LEFT BUTTON
};

struct GameSettings {
    DisplaySettings display;
    GraphicsSettings graphics;
    ControlSettings controls;
};

#endif // SETTINGS_SETTINGS_H_

