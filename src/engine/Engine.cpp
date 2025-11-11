#include "engine/Engine.h"

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "window/Window.h"
#include "window/Events.h"
#include "window/Camera.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "graphics/Font.h"
#include "graphics/Batch2D.h"
#include "graphics/VoxelRenderer.h"
#include "graphics/WaterRenderer.h"
#include "voxels/ChunkManager.h"
#include "voxels/WorldSave.h"
#include "frontend/Menu.h"
#include "maths/FrustumCulling.h"
#include "loaders/png_loading.h"
#include "engine/Game.h"
#include "engine/Renderer.h"
#include "engine/WorldManager.h"
#include "lighting/LightingSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsIO.h"

using namespace glm;

Engine::Engine() {
}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize() {
    // Загружаем настройки
    settings = new GameSettings();
    std::string settingsPath = SettingsIO::getSettingsPath();
    SettingsIO::loadSettings(settingsPath, *settings);
    
    // Применяем настройки отображения
    WIDTH = settings->display.width;
    HEIGHT = settings->display.height;
    
    Window::initialize(WIDTH, HEIGHT, "Window 2.0");
    Events::initialize();
    
    if (!loadResources()) {
        return false;
    }
    
    // Инициализация систем
    menu = new Menu();
    menu->setSettings(settings); // Передаем настройки в меню
    menu->setEngine(this); // Передаем указатель на Engine для применения настроек
    chunkManager = new ChunkManager();
    
    // Применяем FOV из настроек
    float fovRad = glm::radians(settings->graphics.fov);
    camera = new Camera(vec3(0.0f, baseHeight + 8.0f, 20.0f), fovRad);
    worldSave = new WorldSave();
    frustum = new Frustum();
    voxelRenderer = new VoxelRenderer(1024 * 1024 * 8);
    waterRenderer = new WaterRenderer(1024 * 1024 * 4); // Меньший буфер для воды
    worldManager = new WorldManager(chunkManager, worldSave, menu);
    lightingSystem = new lighting::LightingSystem();
    lightingSystem->initialize(chunkManager);
    
    // Применяем настройки
    applySettings();
    
    // Настройка OpenGL
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Убеждаемся, что курсор виден при запуске (в меню)
    Window::setCursorMode(GLFW_CURSOR_NORMAL);
    Events::_cursor_locked = false;
    
    return true;
}

bool Engine::loadResources() {
    // Загрузка шейдеров
    shader = load_shader("res/shaders/mc_vert.glsl", "res/shaders/mc_frag.glsl");
    if (shader == nullptr) {
        std::cerr << "failed to load shader" << std::endl;
        return false;
    }
    
    voxelShader = load_shader("res/shaders/voxel_vert.glsl", "res/shaders/voxel_frag.glsl");
    if (voxelShader == nullptr) {
        std::cerr << "failed to load voxel shader" << std::endl;
        return false;
    }
    
    waterShader = load_shader("res/shaders/water_vert.glsl", "res/shaders/water_frag.glsl");
    if (waterShader == nullptr) {
        std::cerr << "failed to load water shader" << std::endl;
        return false;
    }
    
    uiShader = load_shader("res/shaders/ui.glslv", "res/shaders/ui.glslf");
    if (uiShader == nullptr) {
        std::cerr << "failed to load UI shader" << std::endl;
        return false;
    }
    
    linesShader = load_shader("res/shaders/lines.glslv", "res/shaders/lines.glslf");
    if (linesShader == nullptr) {
        std::cerr << "failed to load lines shader" << std::endl;
        return false;
    }
    
    // Настройка UI шейдера
    glActiveTexture(GL_TEXTURE0);
    uiShader->use();
    uiShader->uniform1i("u_texture", 0);
    
    GLint loc_texture = glGetUniformLocation(uiShader->id, "u_texture");
    std::cout << "[UI] loc u_texture = " << loc_texture << std::endl;
    if (loc_texture == -1) {
        std::cerr << "[UI] WARNING: u_texture uniform location = -1!" << std::endl;
    }
    
    // Загрузка текстуры
    texture = load_texture("res/textures/blocks/stone.png");
    if (texture == nullptr) {
        std::cerr << "failed to load texture" << std::endl;
        return false;
    }
    
    // Загрузка шрифта
    std::vector<Texture*> fontPages;
    for (int i = 0; i <= 4; i++) {
        std::string fontFile = "res/fonts/font_" + std::to_string(i) + ".png";
        Texture* fontPage = load_texture(fontFile);
        if (fontPage == nullptr) {
            std::cerr << "failed to load font page " << i << std::endl;
            return false;
        }
        fontPages.push_back(fontPage);
    }
    
    // Настройка фильтров для пиксельного шрифта
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (auto* t : fontPages) {
        t->bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    font = new Font(fontPages, fontPages[0]->height / 16);
    batch = new Batch2D(1024);
    
    return true;
}

void Engine::run() {
    Game game(this);
    game.run();
}

void Engine::shutdown() {
    // Автосохранение при выходе
    if (worldInitialized && !currentWorldPath.empty() && worldManager) {
        std::string worldName = worldManager->getCurrentWorldName();
        int64_t seed = worldManager->getCurrentSeed();
        if (worldManager->saveWorld(currentWorldPath, worldName, seed, baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation, camera)) {
            std::cout << "[SAVE] Auto-saved world on exit to " << currentWorldPath << std::endl;
        } else {
            std::cout << "[SAVE] Failed to auto-save world on exit" << std::endl;
        }
    }
    
    // Сохраняем настройки
    if (settings) {
        std::string settingsPath = SettingsIO::getSettingsPath();
        SettingsIO::saveSettings(settingsPath, *settings);
        delete settings;
        settings = nullptr;
    }
    
    cleanupResources();
    
    if (lightingSystem) delete lightingSystem;
    if (worldManager) delete worldManager;
    if (menu) delete menu;
    if (chunkManager) delete chunkManager;
    if (camera) delete camera;
    if (worldSave) delete worldSave;
    if (frustum) delete frustum;
    if (voxelRenderer) delete voxelRenderer;
    if (waterRenderer) delete waterRenderer;
    
    Events::finalize();
    Window::terminate();
}

void Engine::applySettings() {
    if (!settings || !camera) return;
    
    // Применяем FOV к камере
    camera->fov = glm::radians(settings->graphics.fov);
    
    // Применяем renderDistance
    renderDistance = settings->graphics.renderDistance;
    
    // Применяем swap interval (VSync)
    glfwSwapInterval(settings->display.swapInterval);
    
    // Применяем размер окна (если изменился)
    if (settings->display.width != WIDTH || settings->display.height != HEIGHT) {
        WIDTH = settings->display.width;
        HEIGHT = settings->display.height;
        // Окно уже создано, поэтому не изменяем его размер здесь
        // Это можно сделать позже через glfwSetWindowSize, но пока оставим так
    }
}

void Engine::cleanupResources() {
    if (shader) delete shader;
    if (voxelShader) delete voxelShader;
    if (waterShader) delete waterShader;
    if (uiShader) delete uiShader;
    if (linesShader) delete linesShader;
    if (texture) delete texture;
    if (font) delete font;
    if (batch) delete batch;
}

