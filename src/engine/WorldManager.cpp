#include "engine/WorldManager.h"

#include <iostream>
#include <string>
#include "voxels/ChunkManager.h"
#include "voxels/WorldSave.h"
#include "frontend/Menu.h"
#include "files/files.h"

WorldManager::WorldManager(ChunkManager* chunkManager, WorldSave* worldSave, Menu* menu)
    : chunkManager(chunkManager), worldSave(worldSave), menu(menu) {
}

bool WorldManager::createWorld(const std::string& worldName, int seed,
                               float baseFreq, int octaves, float lacunarity, float gain,
                               float baseHeight, float heightVariation) {
    // Формируем имя файла из названия мира (убираем недопустимые символы)
    std::string safeName = worldName;
    for (char& c : safeName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
#ifdef _WIN32
    std::string saveFileName = "worlds\\" + safeName + ".vxl";
#else
    std::string saveFileName = "worlds/" + safeName + ".vxl";
#endif
    
    currentWorldPath = saveFileName;
    
    // Очищаем старый мир, если был
    chunkManager->clear();
    chunkManager->setSeed(seed);
    chunkManager->setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
    worldLoaded = true;
    
    // Сохраняем мир сразу после создания, чтобы он появился в списке
    float bf, l, g, bh, hv;
    int o;
    chunkManager->getNoiseParams(bf, o, l, g, bh, hv);
    if (worldSave->save(saveFileName, *chunkManager, seed, bf, o, l, g, bh, hv)) {
        std::cout << "[GAME] New world created and saved: name='" << worldName << "', seed=" << seed << ", path=" << saveFileName << std::endl;
        menu->setSaveFileExists(true);
        menu->refreshWorldList();
        return true;
    } else {
        std::cout << "[GAME] New world created but failed to save: name='" << worldName << "', seed=" << seed << ", path=" << saveFileName << std::endl;
        menu->setSaveFileExists(false);
        return false;
    }
}

bool WorldManager::loadWorld(const std::string& worldPath,
                             float& baseFreq, int& octaves, float& lacunarity, float& gain,
                             float& baseHeight, float& heightVariation, int& seed) {
    if (worldPath.empty()) {
        std::cout << "[LOAD] Error: No world selected, cannot load" << std::endl;
        return false;
    }
    
    currentWorldPath = worldPath;
    
    std::cout << "[LOAD] Attempting to load world from: " << worldPath << std::endl;
    
    // Очищаем текущий мир перед загрузкой
    chunkManager->clear();
    if (worldSave->load(worldPath, *chunkManager, seed, baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation)) {
        std::cout << "[LOAD] World loaded successfully from " << worldPath << " (seed: " << seed << ")" << std::endl;
        chunkManager->setSeed(seed);
        chunkManager->setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
        worldLoaded = true;
        menu->setSaveFileExists(true);
        return true;
    } else {
        std::cout << "[LOAD] Failed to load world from " << worldPath << ", creating new world" << std::endl;
        chunkManager->setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
        worldLoaded = true;
        menu->setSaveFileExists(false);
        return false;
    }
}

bool WorldManager::saveWorld(const std::string& worldPath, int seed,
                             float baseFreq, int octaves, float lacunarity, float gain,
                             float baseHeight, float heightVariation) {
    if (worldPath.empty()) {
        std::cout << "[SAVE] No world path set, cannot save" << std::endl;
        return false;
    }
    
    float bf, l, g, bh, hv;
    int o;
    chunkManager->getNoiseParams(bf, o, l, g, bh, hv);
    if (worldSave->save(worldPath, *chunkManager, seed, bf, o, l, g, bh, hv)) {
        std::cout << "[SAVE] World saved successfully to " << worldPath << std::endl;
        menu->setSaveFileExists(true);
        menu->refreshWorldList();
        return true;
    } else {
        std::cout << "[SAVE] Failed to save world to " << worldPath << std::endl;
        return false;
    }
}

void WorldManager::unloadWorld() {
    if (worldLoaded && !currentWorldPath.empty()) {
        float bf, l, g, bh, hv;
        int o;
        chunkManager->getNoiseParams(bf, o, l, g, bh, hv);
        // Сохраняем перед выгрузкой, если нужно
        // worldSave->save(currentWorldPath, *chunkManager, ...);
        std::cout << "[GAME] World uninitialized, ready for new world" << std::endl;
    }
    worldLoaded = false;
    currentWorldPath = "";
    chunkManager->clear();
}

