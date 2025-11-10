#include "engine/Game.h"

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

#include "engine/Game.h"
#include "engine/Engine.h"
#include "engine/Renderer.h"
#include "engine/WorldManager.h"

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

#include "window/Window.h"
#include "window/Events.h"
#include "window/Camera.h"
#include "frontend/GameState.h"
#include "frontend/Menu.h"
#include "voxels/ChunkManager.h"
#include "voxels/MCChunk.h"
#include "voxels/voxel.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "graphics/Batch2D.h"
#include "graphics/Font.h"
#include "maths/FrustumCulling.h"
#include "graphics/VoxelRenderer.h"

using namespace glm;

Game::Game(Engine* engine) : engine(engine) {
    lastTime = glfwGetTime();
}

void Game::run() {
    while (!Window::isShouldClose()) {
        float currentTime = glfwGetTime();
        float delta = currentTime - lastTime;
        lastTime = currentTime;
        
        update(delta);
        render();
        
        Window::swapBuffers();
        Events::pullEvents();
    }
}

void Game::update(float delta) {
    Menu* menu = engine->getMenu();
    ChunkManager* chunkManager = engine->getChunkManager();
    Camera* camera = engine->getCamera();
    WorldManager* worldManager = engine->getWorldManager();
    
    // Обновляем меню
    GameState currentState = menu->update();
    
    // Обновляем список миров при переходе в окно выбора мира
    if (currentState == GameState::WORLD_SELECT && engine->previousState != GameState::WORLD_SELECT) {
        menu->refreshWorldList();
        menu->setSaveFileExists(false);
    }
    
    // Сбрасываем инициализацию мира при переходе в меню или окно выбора мира
    if ((currentState == GameState::MENU || currentState == GameState::WORLD_SELECT) && 
        (engine->previousState == GameState::PLAYING || engine->previousState == GameState::PAUSED)) {
        // Сохраняем текущий мир перед выходом
        if (engine->worldInitialized && !engine->currentWorldPath.empty()) {
            worldManager->saveWorld(engine->currentWorldPath, engine->seed, 
                                    engine->baseFreq, engine->octaves, engine->lacunarity, engine->gain,
                                    engine->baseHeight, engine->heightVariation);
            std::cout << "[GAME] World saved before exit to menu: " << engine->currentWorldPath << std::endl;
        }
        // Сбрасываем инициализацию мира
        engine->worldInitialized = false;
        engine->currentWorldPath = "";
        worldManager->unloadWorld();
    }
    
    engine->previousState = currentState;
    
    // Обрабатываем действия меню
    handleMenuActions();
    
    // Если игра на паузе или в меню, не обрабатываем игровой ввод
    bool inputLocked = (currentState == GameState::MENU || currentState == GameState::WORLD_SELECT || 
                        currentState == GameState::CREATE_WORLD || currentState == GameState::PAUSED);
    
    // Если игра не инициализирована или в меню/окне выбора мира/окне создания мира, пропускаем игровой цикл
    if (!engine->worldInitialized || currentState == GameState::MENU || 
        currentState == GameState::WORLD_SELECT || currentState == GameState::CREATE_WORLD) {
        return;
    }
    
    // Игровой ввод (только если игра не на паузе)
    if (!inputLocked) {
        handleInput(delta);
    }
    
    // Обновляем мир
    updateWorld();
}

void Game::handleMenuActions() {
    Menu* menu = engine->getMenu();
    WorldManager* worldManager = engine->getWorldManager();
    
    Menu::MenuAction action = menu->getMenuAction();
    if (action == Menu::MenuAction::CREATE_WORLD) {
        // Создаем новый мир
        if (engine->worldInitialized) {
            worldManager->unloadWorld();
            engine->worldInitialized = false;
            engine->currentWorldPath = "";
        }
        if (!engine->worldInitialized) {
            // Получаем seed и название мира из меню
            engine->seed = menu->getWorldSeed();
            std::string worldName = menu->getWorldName();
            
            if (worldManager->createWorld(worldName, engine->seed,
                                          engine->baseFreq, engine->octaves, engine->lacunarity, engine->gain,
                                          engine->baseHeight, engine->heightVariation)) {
                engine->currentWorldPath = worldManager->getCurrentWorldPath();
                engine->worldInitialized = true;
            }
        }
        menu->clearMenuAction();
    } else if (action == Menu::MenuAction::LOAD_WORLD) {
        // Загружаем сохраненный мир
        if (engine->worldInitialized) {
            worldManager->unloadWorld();
            engine->worldInitialized = false;
            engine->currentWorldPath = "";
        }
        if (!engine->worldInitialized) {
            // Получаем путь к выбранному миру
            std::string saveFileName = menu->getSelectedWorldPath();
            if (saveFileName.empty()) {
                std::cout << "[LOAD] Error: No world selected, cannot load" << std::endl;
                menu->clearMenuAction();
                return;
            }
            
            if (worldManager->loadWorld(saveFileName,
                                        engine->baseFreq, engine->octaves, engine->lacunarity, engine->gain,
                                        engine->baseHeight, engine->heightVariation, engine->seed)) {
                engine->currentWorldPath = worldManager->getCurrentWorldPath();
                engine->worldInitialized = true;
            }
        }
        menu->clearMenuAction();
    } else if (action == Menu::MenuAction::QUIT) {
        // Выход из игры
        Window::setShouldClose(true);
    }
}

void Game::handleInput(float delta) {
    Menu* menu = engine->getMenu();
    ChunkManager* chunkManager = engine->getChunkManager();
    Camera* camera = engine->getCamera();
    WorldManager* worldManager = engine->getWorldManager();
    
    if (Events::jpressed(GLFW_KEY_TAB)) {
        Events::toggleCursor();
    }
    
    // Тестовая установка блока по нажатию клавиши T
    if (Events::jpressed(GLFW_KEY_T)) {
        int bx = (int)camera->position.x;
        int by = (int)camera->position.y;
        int bz = (int)camera->position.z;
        std::cout << "Тестовая установка блока в позиции камеры: (" << bx << ", " << by << ", " << bz << ")" << std::endl;
        chunkManager->setVoxel(bx, by, bz, 2);
    }
    
    // Сохранение мира по нажатию F5
    if (Events::jpressed(GLFW_KEY_F5)) {
        if (!engine->currentWorldPath.empty()) {
            worldManager->saveWorld(engine->currentWorldPath, engine->seed,
                                    engine->baseFreq, engine->octaves, engine->lacunarity, engine->gain,
                                    engine->baseHeight, engine->heightVariation);
        }
    }
    
    // Движение камеры
    if (Events::pressed(GLFW_KEY_W)) {
        camera->position += camera->front * delta * engine->speed;
    }
    if (Events::pressed(GLFW_KEY_S)) {
        camera->position -= camera->front * delta * engine->speed;
    }
    if (Events::pressed(GLFW_KEY_D)) {
        camera->position -= camera->right * delta * engine->speed;
    }
    if (Events::pressed(GLFW_KEY_A)) {
        camera->position += camera->right * delta * engine->speed;
    }
    
    // Вращение камеры
    if (Events::_cursor_locked) {
        camY += Events::deltaY / Window::height * 2;
        camX += -Events::deltaX / Window::height * 2;
        
        if (camY < -radians(89.0f)) {
            camY = -radians(89.0f);
        }
        if (camY > radians(89.0f)) {
            camY = radians(89.0f);
        }
        
        camera->rotation = mat4(1.0f);
        camera->rotate(camY, camX, 0);
    }
    
    // Обработка кликов мыши для установки/удаления блоков
    vec3 end;
    vec3 norm;
    vec3 iend;
    
    // Сначала проверяем воксельные блоки
    voxel* vox = chunkManager->rayCast(camera->position, camera->front, 15.0f, end, norm, iend);
    
    // Сохраняем информацию о блоке под курсором для отрисовки контура
    if (vox != nullptr && vox->id != 0) {
        engine->hasTargetBlock = true;
        engine->targetBlockX = (int)iend.x;
        engine->targetBlockY = (int)iend.y;
        engine->targetBlockZ = (int)iend.z;
        engine->targetBlockNormal = norm;
    } else {
        engine->hasTargetBlock = false;
    }
    
    if (vox != nullptr) {
        // Нашли воксельный блок
        if (Events::jclicked(GLFW_MOUSE_BUTTON_1)) {
            // Удаление блока (левая кнопка мыши)
            int bx = (int)iend.x;
            int by = (int)iend.y;
            int bz = (int)iend.z;
            std::cout << "[DELETE] Block removed at (" << bx << ", " << by << ", " << bz << ")" << std::endl;
            chunkManager->setVoxel(bx, by, bz, 0);
        }
        if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
            // Установка блока рядом с найденным (правая кнопка мыши)
            int bx = (int)(iend.x) + (int)(norm.x);
            int by = (int)(iend.y) + (int)(norm.y);
            int bz = (int)(iend.z) + (int)(norm.z);
            std::cout << "[PLACE] Attempting to place block next to voxel at (" 
                      << bx << ", " << by << ", " << bz << ")" << std::endl;
            chunkManager->setVoxel(bx, by, bz, 2);
            
            // Проверяем, установился ли блок
            voxel* checkVox = chunkManager->getVoxel(bx, by, bz);
            if (checkVox != nullptr && checkVox->id == 2) {
                std::cout << "[SUCCESS] Block placed at (" << bx << ", " << by << ", " << bz << ")" << std::endl;
            } else {
                std::cout << "[ERROR] Block NOT placed at (" << bx << ", " << by << ", " << bz << ")" << std::endl;
            }
        }
    } else {
        // Не нашли воксельный блок - ищем поверхность земли (Marching Cubes)
        vec3 surfacePos;
        vec3 surfaceNorm;
        if (chunkManager->rayCastSurface(camera->position, camera->front, 10.0f, surfacePos, surfaceNorm)) {
            if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
                // Установка блока на поверхность земли
                std::cout << "[SURFACE] Found ground surface at (" 
                          << surfacePos.x << ", " << surfacePos.y << ", " << surfacePos.z 
                          << ") normal: (" << surfaceNorm.x << ", " << surfaceNorm.y << ", " << surfaceNorm.z << ")" << std::endl;
                
                // Округляем до целых координат (сетка блоков)
                int bx = (int)std::round(surfacePos.x);
                int by = (int)std::round(surfacePos.y);
                int bz = (int)std::round(surfacePos.z);
                
                // Если поверхность выше, ставим блок на поверхность, иначе немного выше
                if (surfaceNorm.y > 0.5f) {
                    // Поверхность сверху - ставим на неё
                    by = (int)std::floor(surfacePos.y) + 1;
                    std::cout << "[PLACE] Surface on top, placing block on surface" << std::endl;
                } else {
                    // Поверхность сбоку - ставим рядом
                    by = (int)std::round(surfacePos.y);
                    std::cout << "[PLACE] Surface on side, placing block nearby" << std::endl;
                }
                
                std::cout << "[PLACE] Attempting to place block on ground surface at (" 
                          << bx << ", " << by << ", " << bz << ")" << std::endl;
                
                chunkManager->setVoxel(bx, by, bz, 2);
                
                // Проверяем, установился ли блок
                voxel* checkVox = chunkManager->getVoxel(bx, by, bz);
                if (checkVox != nullptr && checkVox->id == 2) {
                    std::cout << "[SUCCESS] Block placed on ground surface at (" 
                              << bx << ", " << by << ", " << bz << ")" << std::endl;
                } else {
                    std::cout << "[ERROR] Block NOT placed on ground surface at (" 
                              << bx << ", " << by << ", " << bz << ")" << std::endl;
                }
            }
        } else {
            // Отладочный вывод, если не нашли поверхность
            if (Events::jclicked(GLFW_MOUSE_BUTTON_2)) {
                std::cout << "[ERROR] Ground surface not found. Camera pos: (" 
                          << camera->position.x << ", " << camera->position.y << ", " << camera->position.z 
                          << ") Direction: (" << camera->front.x << ", " << camera->front.y << ", " << camera->front.z << ")" << std::endl;
            }
        }
    }
}

void Game::updateWorld() {
    ChunkManager* chunkManager = engine->getChunkManager();
    Camera* camera = engine->getCamera();
    VoxelRenderer* voxelRenderer = engine->getVoxelRenderer();
    
    // Обновляем чанки вокруг камеры
    chunkManager->update(camera->position, engine->renderDistance);
    
    // Пересобираем меши вокселей для измененных чанков
    std::vector<MCChunk*> visibleChunks = chunkManager->getVisibleChunks();
    for (MCChunk* chunk : visibleChunks) {
        if (chunk->voxelMeshModified) {
            // Удаляем старый меш
            if (chunk->voxelMesh != nullptr) {
                delete chunk->voxelMesh;
                chunk->voxelMesh = nullptr;
            }
            
            // Собираем соседние чанки для face culling
            MCChunk* nearbyChunks[27] = {nullptr};
            for (MCChunk* other : visibleChunks) {
                glm::ivec3 diff = other->chunkPos - chunk->chunkPos;
                if (std::abs(diff.x) <= 1 && std::abs(diff.y) <= 1 && std::abs(diff.z) <= 1) {
                    int index = (diff.y + 1) * 9 + (diff.z + 1) * 3 + (diff.x + 1);
                    nearbyChunks[index] = other;
                }
            }
            
            // Генерируем новый меш
            chunk->voxelMesh = voxelRenderer->render(chunk, nearbyChunks);
            chunk->voxelMeshModified = false;
        }
    }
}

void Game::render() {
    Menu* menu = engine->getMenu();
    ChunkManager* chunkManager = engine->getChunkManager();
    Camera* camera = engine->getCamera();
    Frustum* frustum = engine->getFrustum();
    Shader* shader = engine->getShader();
    Shader* voxelShader = engine->getVoxelShader();
    Shader* uiShader = engine->getUIShader();
    Shader* linesShader = engine->getLinesShader();
    Texture* texture = engine->getTexture();
    Font* font = engine->getFont();
    Batch2D* batch = engine->getBatch();
    
    GameState currentState = menu->getState();
    
    // Если игра не инициализирована или в меню/окне выбора мира/окне создания мира, отрисовываем только меню
    if (!engine->worldInitialized || currentState == GameState::MENU || 
        currentState == GameState::WORLD_SELECT || currentState == GameState::CREATE_WORLD) {
        if (currentState == GameState::MENU || currentState == GameState::WORLD_SELECT || 
            currentState == GameState::CREATE_WORLD) {
            // Убеждаемся, что viewport установлен правильно
            glViewport(0, 0, Window::fbWidth > 0 ? Window::fbWidth : Window::width, 
                      Window::fbHeight > 0 ? Window::fbHeight : Window::height);
            
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // UI state: отключаем cull и depth, включаем blend
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            
            // Гарантируем нужный текстурный юнит перед UI-рендером
            glActiveTexture(GL_TEXTURE0);
            uiShader->use();
            uiShader->uniform1i("u_texture", 0);
            
            menu->draw(batch, font, uiShader, Window::width, Window::height);
            
            // Восстанавливаем состояние
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
        }
        return;
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Обновляем frustum для culling
    mat4 projview = camera->getProjection() * camera->getView();
    frustum->update(projview);
    
    // Отрисовываем мир
    renderWorld();
    
    // Отрисовываем контур блока под курсором (только во время игры и если есть цель)
    if (currentState == GameState::PLAYING && engine->hasTargetBlock) {
        Renderer::drawBlockOutline(linesShader, projview, 
                                    engine->targetBlockX, engine->targetBlockY, engine->targetBlockZ, 
                                    engine->targetBlockNormal);
    }
    
    // Отрисовываем меню паузы поверх мира
    if (currentState == GameState::PAUSED) {
        // UI state: отключаем cull и depth, включаем blend
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        
        // Гарантируем нужный текстурный юнит перед UI-рендером
        glActiveTexture(GL_TEXTURE0);
        uiShader->use();
        uiShader->uniform1i("u_texture", 0);
        
        menu->draw(batch, font, uiShader, Window::width, Window::height);
        
        // Восстанавливаем состояние
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }
    
    // Отрисовываем прицел во время игры (не в меню и не на паузе)
    if (currentState == GameState::PLAYING) {
        Renderer::drawCrosshair(batch, uiShader, Window::width, Window::height);
    }
}

void Game::renderWorld() {
    ChunkManager* chunkManager = engine->getChunkManager();
    Camera* camera = engine->getCamera();
    Frustum* frustum = engine->getFrustum();
    Shader* shader = engine->getShader();
    Shader* voxelShader = engine->getVoxelShader();
    Texture* texture = engine->getTexture();
    
    mat4 projview = camera->getProjection() * camera->getView();
    mat4 model(1.0f);
    
    // Отрисовываем Marching Cubes (поверхность земли)
    shader->use();
    shader->uniformMatrix("projview", projview);
    // Передаем параметры для процедурного текстурирования
    shader->uniform1f("u_baseHeight", engine->baseHeight);
    shader->uniform1f("u_heightVariation", engine->heightVariation);
    // Нормализуем направление света на CPU перед передачей
    vec3 lightDir = normalize(vec3(0.4f, 0.8f, 0.4f));
    shader->uniform3f("u_lightDir", lightDir.x, lightDir.y, lightDir.z);
    
    std::vector<MCChunk*> visibleChunks = chunkManager->getVisibleChunks();
    for (MCChunk* chunk : visibleChunks) {
        if (chunk->mesh != nullptr) {
            // Frustum culling для чанка
            vec3 chunkMin = chunk->worldPos - vec3(
                MCChunk::CHUNK_SIZE_X / 2.0f,
                MCChunk::CHUNK_SIZE_Y / 2.0f,
                MCChunk::CHUNK_SIZE_Z / 2.0f
            );
            vec3 chunkMax = chunk->worldPos + vec3(
                MCChunk::CHUNK_SIZE_X / 2.0f,
                MCChunk::CHUNK_SIZE_Y / 2.0f,
                MCChunk::CHUNK_SIZE_Z / 2.0f
            );
            
            if (!frustum->IsBoxVisible(chunkMin, chunkMax)) {
                continue; // Пропускаем чанк вне frustum
            }
            
            // Вычисляем матрицу модели для позиционирования чанка в мире
            mat4 chunkModel = translate(model, chunk->worldPos - vec3(
                MCChunk::CHUNK_SIZE_X / 2.0f,
                MCChunk::CHUNK_SIZE_Y / 2.0f,
                MCChunk::CHUNK_SIZE_Z / 2.0f
            ));
            shader->uniformMatrix("model", chunkModel);
            chunk->mesh->draw(GL_TRIANGLES);
        }
    }
    
    // Отрисовываем воксельные блоки
    voxelShader->use();
    voxelShader->uniformMatrix("projview", projview);
    texture->bind();
    
    for (MCChunk* chunk : visibleChunks) {
        if (chunk->voxelMesh != nullptr) {
            // Frustum culling для воксельного чанка
            vec3 chunkMin = chunk->worldPos - vec3(
                MCChunk::CHUNK_SIZE_X / 2.0f,
                MCChunk::CHUNK_SIZE_Y / 2.0f,
                MCChunk::CHUNK_SIZE_Z / 2.0f
            );
            vec3 chunkMax = chunk->worldPos + vec3(
                MCChunk::CHUNK_SIZE_X / 2.0f,
                MCChunk::CHUNK_SIZE_Y / 2.0f,
                MCChunk::CHUNK_SIZE_Z / 2.0f
            );
            
            if (!frustum->IsBoxVisible(chunkMin, chunkMax)) {
                continue; // Пропускаем чанк вне frustum
            }
            
            mat4 chunkModel(1.0f); // Воксели уже в мировых координатах
            voxelShader->uniformMatrix("model", chunkModel);
            chunk->voxelMesh->draw(GL_TRIANGLES);
        }
    }
}

