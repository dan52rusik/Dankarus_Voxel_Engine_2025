#include "engine/WorldManager.h"

#include <iostream>
#include <string>
#include "voxels/ChunkManager.h"
#include "voxels/DecoManager.h"
#include "voxels/WorldSave.h"
#include "voxels/WorldBuilder.h"
#include "voxels/WaterSimulator.h"
#include "voxels/WaterEvaporationManager.h"
#include "frontend/Menu.h"
#include "files/files.h"
#include "window/Camera.h"

WorldManager::WorldManager(ChunkManager* chunkManager, WorldSave* worldSave, Menu* menu)
    : chunkManager(chunkManager), worldSave(worldSave), menu(menu), decoManager(nullptr), worldBuilder(nullptr) {
}

bool WorldManager::createWorld(const std::string& worldName, int64_t seed,
                               float baseFreq, int octaves, float lacunarity, float gain,
                               float baseHeight, float heightVariation) {
    // Формируем имя папки из названия мира (убираем недопустимые символы)
    std::string safeName = worldName;
    for (char& c : safeName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
#ifdef _WIN32
    std::string worldPath = "worlds\\" + safeName;
#else
    std::string worldPath = "worlds/" + safeName;
#endif
    
    currentWorldPath = worldPath;
    currentWorldName = worldName;
    currentSeed = seed;
    
    // Очищаем старый мир, если был
    chunkManager->clear();
    chunkManager->setSeed(seed);
	chunkManager->setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
	
	// Устанавливаем уровень воды относительно базовой высоты (вода в низинах)
	// Используем примерно 60-70% от baseHeight, чтобы вода была в низинах, но не слишком низко
	float waterLevel = baseHeight * 0.65f;
	chunkManager->setWaterLevel(waterLevel);
	std::cout << "[WORLD] Set water level to " << waterLevel << " (baseHeight=" << baseHeight << ")" << std::endl;
	
	// Устанавливаем WorldSave и путь к миру для автосохранения чанков
	chunkManager->setWorldSave(worldSave, worldPath);
	
	// Создаём и инициализируем менеджер декораций
	decoManager = new DecoManager();
	// Размеры мира для декораций (можно настроить)
	int worldWidth = 4096;  // Размер мира по X
	int worldHeight = 4096; // Размер мира по Z
	decoManager->onWorldLoaded(worldWidth, worldHeight, chunkManager, worldPath, seed);
	chunkManager->setDecoManager(decoManager);
	
	// Создаём и инициализируем WorldBuilder для генерации дорог, озер и префабов
	worldBuilder = new WorldBuilder(chunkManager);
	worldBuilder->Initialize(worldWidth, seed);
	
	// Передаем WorldBuilder в ChunkManager для применения модификаций при генерации чанков
	chunkManager->setWorldBuilder(worldBuilder);
	
	// Создаём и инициализируем симулятор воды
	WaterSimulator* waterSimulator = new WaterSimulator(chunkManager);
	chunkManager->setWaterSimulator(waterSimulator);
	
	// Создаём и инициализируем менеджер испарения воды
	WaterEvaporationManager* waterEvaporationManager = new WaterEvaporationManager(chunkManager);
	chunkManager->setWaterEvaporationManager(waterEvaporationManager);
	
	// Генерируем элементы мира
	std::cout << "[WORLDBUILDER] Starting world generation..." << std::endl;
	worldBuilder->GenerateRoads(5, 20);  // 5 шоссе, 20 проселочных дорог
	worldBuilder->GenerateWaterFeatures(10, 5);  // 10 озер, 5 рек
	worldBuilder->GeneratePrefabs(50);  // 50 префабов
	std::cout << "[WORLDBUILDER] World generation completed!" << std::endl;
	
	worldLoaded = true;
    
    // Сохраняем мир сразу после создания, чтобы он появился в списке
    // camera передадим как nullptr, т.к. при создании мира камера еще не инициализирована
    if (worldSave->save(worldPath, *chunkManager, worldName, seed, baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation, nullptr)) {
        std::cout << "[GAME] New world created and saved: name='" << worldName << "', seed=" << seed << ", path=" << worldPath << std::endl;
        menu->setSaveFileExists(true);
        menu->refreshWorldList();
        return true;
    } else {
        std::cout << "[GAME] New world created but failed to save: name='" << worldName << "', seed=" << seed << ", path=" << worldPath << std::endl;
        menu->setSaveFileExists(false);
        return false;
    }
}

bool WorldManager::loadWorld(const std::string& worldPath,
                             float& baseFreq, int& octaves, float& lacunarity, float& gain,
                             float& baseHeight, float& heightVariation, int64_t& seed,
                             std::string& worldName, Camera* camera) {
    if (worldPath.empty()) {
        std::cout << "[LOAD] Error: No world selected, cannot load" << std::endl;
        return false;
    }
    
    currentWorldPath = worldPath;
    
    std::cout << "[LOAD] Attempting to load world from: " << worldPath << std::endl;
    
    // Очищаем текущий мир перед загрузкой
	chunkManager->clear();
	
	// Устанавливаем WorldSave и путь к миру для автосохранения чанков
	chunkManager->setWorldSave(worldSave, worldPath);
	
	// Создаём и инициализируем менеджер декораций
	if (decoManager != nullptr) {
		decoManager->onWorldUnloaded();
		delete decoManager;
	}
	decoManager = new DecoManager();
	int worldWidth = 4096;  // Размер мира по X
	int worldHeight = 4096; // Размер мира по Z
	decoManager->onWorldLoaded(worldWidth, worldHeight, chunkManager, worldPath, seed);
	chunkManager->setDecoManager(decoManager);
	
	// Создаём и инициализируем WorldBuilder (для загруженных миров тоже нужен)
	if (worldBuilder != nullptr) {
		worldBuilder->Clear();
		delete worldBuilder;
	}
	worldBuilder = new WorldBuilder(chunkManager);
	worldBuilder->Initialize(worldWidth, seed);
	// Передаем WorldBuilder в ChunkManager для применения модификаций при генерации чанков
	chunkManager->setWorldBuilder(worldBuilder);
	
	// Создаём и инициализируем симулятор воды
	WaterSimulator* waterSimulator = new WaterSimulator(chunkManager);
	chunkManager->setWaterSimulator(waterSimulator);
	
	// Создаём и инициализируем менеджер испарения воды
	WaterEvaporationManager* waterEvaporationManager = new WaterEvaporationManager(chunkManager);
	chunkManager->setWaterEvaporationManager(waterEvaporationManager);
	
	// При загрузке не генерируем заново, только инициализируем системы
	
	if (worldSave->load(worldPath, *chunkManager, worldName, seed, baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation, camera)) {
        std::cout << "[LOAD] World loaded successfully from " << worldPath << " (name: " << worldName << ", seed: " << seed << ")" << std::endl;
        std::cout << "[LOAD] Generator params: baseFreq=" << baseFreq << ", octaves=" << octaves 
                  << ", lacunarity=" << lacunarity << ", gain=" << gain 
                  << ", baseHeight=" << baseHeight << ", heightVariation=" << heightVariation << std::endl;
        chunkManager->setSeed(seed);
        chunkManager->setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
        
        // Устанавливаем уровень воды относительно базовой высоты
        float waterLevel = baseHeight * 0.65f;
        chunkManager->setWaterLevel(waterLevel);
        std::cout << "[LOAD] Set water level to " << waterLevel << " (baseHeight=" << baseHeight << ")" << std::endl;
        
        worldLoaded = true;
        currentWorldName = worldName;
        currentSeed = seed;
        menu->setSaveFileExists(true);
        return true;
    } else {
        std::cout << "[LOAD] Failed to load world from " << worldPath << ", creating new world" << std::endl;
        chunkManager->setNoiseParams(baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
        
        // Устанавливаем уровень воды относительно базовой высоты
        float waterLevel = baseHeight * 0.65f;
        chunkManager->setWaterLevel(waterLevel);
        std::cout << "[LOAD] Set water level to " << waterLevel << " (baseHeight=" << baseHeight << ")" << std::endl;
        
        worldLoaded = true;
        menu->setSaveFileExists(false);
        return false;
    }
}

bool WorldManager::saveWorld(const std::string& worldPath, const std::string& worldName, int64_t seed,
                             float baseFreq, int octaves, float lacunarity, float gain,
                             float baseHeight, float heightVariation, Camera* camera) {
    if (worldPath.empty()) {
        std::cout << "[SAVE] No world path set, cannot save" << std::endl;
        return false;
    }
    
    float bf, l, g, bh, hv;
    int o;
    chunkManager->getNoiseParams(bf, o, l, g, bh, hv);
    if (worldSave->save(worldPath, *chunkManager, worldName, seed, bf, o, l, g, bh, hv, camera)) {
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
        // Сохраняем все измененные чанки перед выгрузкой
        chunkManager->saveDirtyChunks();
        
        // Сохраняем и выгружаем менеджер декораций
        if (decoManager != nullptr) {
            decoManager->onWorldUnloaded();
            delete decoManager;
            decoManager = nullptr;
        }
        
        // Очищаем WorldBuilder
        if (worldBuilder != nullptr) {
            worldBuilder->Clear();
            delete worldBuilder;
            worldBuilder = nullptr;
        }
        
        // Очищаем симулятор воды
        WaterSimulator* waterSimulator = chunkManager->getWaterSimulator();
        if (waterSimulator != nullptr) {
            delete waterSimulator;
            chunkManager->setWaterSimulator(nullptr);
        }
        
        // Очищаем менеджер испарения воды
        WaterEvaporationManager* waterEvaporationManager = chunkManager->getWaterEvaporationManager();
        if (waterEvaporationManager != nullptr) {
            delete waterEvaporationManager;
            chunkManager->setWaterEvaporationManager(nullptr);
        }
        
        std::cout << "[GAME] World uninitialized, ready for new world" << std::endl;
    }
    worldLoaded = false;
    currentWorldPath = "";
    chunkManager->clear();
}

