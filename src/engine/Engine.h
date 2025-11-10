#pragma once

#include <string>
#include <glm/glm.hpp>
#include "frontend/GameState.h"

class Window;
class Events;
class Shader;
class Texture;
class Font;
class Batch2D;
class Menu;
class ChunkManager;
class Camera;
class WorldSave;
class VoxelRenderer;
class Frustum;
class WorldManager;

namespace lighting {
    class LightingSystem;
}

struct GameSettings;

class Engine {
public:
    Engine();
    ~Engine();
    
    bool initialize();
    void run();
    void shutdown();
    
    // Геттеры для доступа к системам
    Shader* getShader() const { return shader; }
    Shader* getVoxelShader() const { return voxelShader; }
    Shader* getUIShader() const { return uiShader; }
    Shader* getLinesShader() const { return linesShader; }
    Texture* getTexture() const { return texture; }
    Font* getFont() const { return font; }
    Batch2D* getBatch() const { return batch; }
    VoxelRenderer* getVoxelRenderer() const { return voxelRenderer; }
    Menu* getMenu() const { return menu; }
    ChunkManager* getChunkManager() const { return chunkManager; }
    Camera* getCamera() const { return camera; }
    WorldSave* getWorldSave() const { return worldSave; }
    Frustum* getFrustum() const { return frustum; }
    WorldManager* getWorldManager() const { return worldManager; }
    lighting::LightingSystem* getLightingSystem() const { return lightingSystem; }
    
    // Настройки
    GameSettings* settings = nullptr;
    
    // Геттеры/сеттеры для настроек
    GameSettings* getSettings() const { return settings; }
    void applySettings(); // Применить настройки к системам (камера, renderDistance, etc.)
    
    // Параметры (legacy, будут заменены на settings)
    int WIDTH = 1280;
    int HEIGHT = 720;
    float baseFreq = 0.03f;
    int octaves = 4;
    float lacunarity = 2.0f;
    float gain = 0.5f;
    float baseHeight = 12.0f;
    float heightVariation = 4.0f;
    int seed = 1337;
    int renderDistance = 3;
    float speed = 5.0f;
    
    // Состояние
    std::string currentWorldPath = "";
    bool worldInitialized = false;
    GameState previousState = GameState::MENU;
    
    // Raycast
    bool hasTargetBlock = false;
    int targetBlockX = 0;
    int targetBlockY = 0;
    int targetBlockZ = 0;
    glm::vec3 targetBlockNormal;
    
    // Hotbar (9 слотов)
    static const int HOTBAR_SIZE = 9;
    int hotbarSlots[HOTBAR_SIZE] = {1, 2, 3, 4, 0, 0, 0, 0, 0}; // Инициализируем первые 4 слота
    int selectedHotbarSlot = 0; // Текущий выбранный слот (0-8)
    
    // Выбранный блок (получается из hotbar)
    int getSelectedBlockId() const { return hotbarSlots[selectedHotbarSlot]; }
    
private:
    bool loadResources();
    void cleanupResources();
    
    // Ресурсы
    Shader* shader = nullptr;
    Shader* voxelShader = nullptr;
    Shader* uiShader = nullptr;
    Shader* linesShader = nullptr;
    Texture* texture = nullptr;
    Font* font = nullptr;
    Batch2D* batch = nullptr;
    VoxelRenderer* voxelRenderer = nullptr;
    
    // Системы
    Menu* menu = nullptr;
    ChunkManager* chunkManager = nullptr;
    Camera* camera = nullptr;
    WorldSave* worldSave = nullptr;
    Frustum* frustum = nullptr;
    WorldManager* worldManager = nullptr;
    lighting::LightingSystem* lightingSystem = nullptr;
};

