#pragma once

#include "frontend/GameState.h"
#include <string>

class Engine;
class Menu;
class ChunkManager;
class Camera;
class WorldSave;
class VoxelRenderer;
class Frustum;
class Shader;
class Texture;
class Font;
class Batch2D;

class Game {
public:
    Game(Engine* engine);
    
    void run();
    
private:
    void update(float delta);
    void render();
    void handleInput(float delta);
    void handleMenuActions();
    void updateWorld(float delta);
    void renderWorld();
    void renderUI();
    
    Engine* engine;
    
    // Состояние
    float camX = 0.0f;
    float camY = 0.0f;
    float lastTime = 0.0f;
};

