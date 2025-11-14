#include "engine/WorldManager.h"

#include <iostream>
#include <string>
#include <cmath>
#include "voxels/ChunkManager.h"
#include "voxels/MCChunk.h"
#include "voxels/DecoManager.h"
#include "voxels/WorldSave.h"
#include "voxels/WorldBuilder.h"
#include "voxels/WaterSimulator.h"
#include "voxels/WaterEvaporationManager.h"
#include "voxels/GeneratorParams.h"
#include "frontend/Menu.h"
#include "files/files.h"
#include "window/Camera.h"
#include <glm/glm.hpp>

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
    
	// Используем единую структуру параметров GeneratorParams
	GeneratorParams gp;
	gp.baseFreq = baseFreq;
	gp.octaves = octaves;
	gp.lacunarity = lacunarity;
	gp.gain = gain;
	gp.baseHeight = baseHeight;
	gp.heightVariation = heightVariation;
	gp.waterLevel = baseHeight - 2.0f;  // Уровень воды чуть ниже базовой высоты
	gp.seed = seed;
	
	// Конфигурируем ChunkManager единой структурой
	chunkManager->configure(gp);
	
	// Устанавливаем WorldSave и путь к миру для автосохранения чанков
	chunkManager->setWorldSave(worldSave, worldPath);
	
	// Устанавливаем границы мира: 10к×10к метров
	const int worldSize = 10000;
	chunkManager->setWorldBoundsByMeters(worldSize);
	
	// Создаём и инициализируем менеджер декораций
	decoManager = new DecoManager();
	// ВРЕМЕННО ОТКЛЮЧЕНО: генерация декораций замедляет загрузку мира
	// decoManager->isEnabled = false; // Отключаем генерацию декораций
	// Размеры мира для декораций: 10к×10к
	// decoManager->onWorldLoaded(worldSize, worldSize, chunkManager, worldPath, seed);
	chunkManager->setDecoManager(decoManager);
	
	// Создаём и инициализируем WorldBuilder для генерации дорог, озер и префабов
	worldBuilder = new WorldBuilder(chunkManager);
	worldBuilder->Initialize(worldSize, seed);
	
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
	worldBuilder->GenerateWaterFeatures(80, 0);  // ИСПРАВЛЕНО: 80 озер (было 10), 0 рек - больше озёр для видимости
	worldBuilder->GeneratePrefabs(50);  // 50 префабов
	std::cout << "[WORLDBUILDER] World generation completed!" << std::endl;
	
	worldLoaded = true;
    
    // Сохраняем мир сразу после создания, чтобы он появился в списке
    // camera передадим как nullptr, т.к. при создании мира камера еще не инициализирована
    // Сохраняем мир с параметрами из GeneratorParams
    if (worldSave->save(worldPath, *chunkManager, worldName, gp.seed, gp.baseFreq, gp.octaves, gp.lacunarity, gp.gain, gp.baseHeight, gp.heightVariation, nullptr)) {
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
	
	// ВАЖНО: пересоздаём waterMap так же, как при создании мира
	// Иначе в загруженном мире waterMap пустой и озёра не создаются
	std::cout << "[WORLDBUILDER] Regenerating water features for loaded world..." << std::endl;
	worldBuilder->GenerateWaterFeatures(80, 0);  // Те же параметры, что и в createWorld
	
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
        // WorldSave::load() уже вызвал chunkManager->configure(gp), поэтому параметры применены
        std::cout << "[LOAD] World loaded successfully from " << worldPath << " (name: " << worldName << ", seed: " << seed << ")" << std::endl;
        
        // ЖЕСТКАЯ ПРИВЯЗКА: Проверяем и корректируем позицию игрока после загрузки
        if (camera != nullptr) {
            if (!worldSave->loadPlayer(worldPath, camera)) {
                // Нет player.json → ставим игрока в безопасную точку
                spawnPlayerSafely(camera, 0.0f, 0.0f, 3.0f, 512.0f);
            } else {
                // Есть player.json → подстрахуемся: если под водой/под землёй/на отвесе — поправим
                spawnPlayerSafely(camera, camera->position.x, camera->position.z, 3.0f, 256.0f);
            }
        }
        
        worldLoaded = true;
        currentWorldName = worldName;
        currentSeed = seed;
        menu->setSaveFileExists(true);
        return true;
    } else {
        std::cout << "[LOAD] Failed to load world from " << worldPath << ", creating new world" << std::endl;
        // Используем параметры по умолчанию через GeneratorParams
        GeneratorParams gp;
        gp.baseFreq = baseFreq;
        gp.octaves = octaves;
        gp.lacunarity = lacunarity;
        gp.gain = gain;
        gp.baseHeight = baseHeight;
        gp.heightVariation = heightVariation;
        gp.waterLevel = baseHeight - 2.0f;
        gp.seed = seed;
        chunkManager->configure(gp);
        
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

// Оценить крутизну склона в точке (x, z) - возвращает 0..1 (0 = ровно, 1 = отвес)
static float estimateSlope01(ChunkManager* cm, float x, float z, float sample = 2.0f) {
	float hC = cm->evalSurfaceHeight(x, z);
	float hx = cm->evalSurfaceHeight(x + sample, z) - hC;
	float hz = cm->evalSurfaceHeight(x, z + sample) - hC;
	// угол к вертикали → 0..1
	float slope = glm::clamp(std::sqrt(hx*hx + hz*hz) / (sample * 1.5f), 0.0f, 1.0f);
	return slope;
}

void WorldManager::spawnPlayerSafely(Camera* camera, float preferX, float preferZ,
                                     float heightOffset, float searchRadiusMeters) {
	if (!camera || !chunkManager) return;
	
	// 1) Границы мира (в метрах) — чтобы не вылететь за край
	const float worldMinX = static_cast<float>(chunkManager->getMinChunkX() * MCChunk::CHUNK_SIZE_X);
	const float worldMaxX = static_cast<float>((chunkManager->getMaxChunkX() + 1) * MCChunk::CHUNK_SIZE_X - 1.0f);
	const float worldMinZ = static_cast<float>(chunkManager->getMinChunkZ() * MCChunk::CHUNK_SIZE_Z);
	const float worldMaxZ = static_cast<float>((chunkManager->getMaxChunkZ() + 1) * MCChunk::CHUNK_SIZE_Z - 1.0f);
	
	auto clampXZ = [&](float& x, float& z){
		x = glm::clamp(x, worldMinX + 4.0f, worldMaxX - 4.0f);
		z = glm::clamp(z, worldMinZ + 4.0f, worldMaxZ - 4.0f);
	};
	
	float bestX = preferX, bestZ = preferZ;
	clampXZ(bestX, bestZ);
	
	// 2) Критерии «хорошего места»
	const float maxSlope = 0.6f; // 0..1, где 0 — ровно; 0.6 — уже крутовато
	const float waterLevel = chunkManager->getWaterLevel();
	
	auto good = [&](float x, float z){
		float h = chunkManager->evalSurfaceHeight(x, z);
		if (h < waterLevel + 1.0f) return false;          // не под водой и не у самой кромки
		if (estimateSlope01(chunkManager, x, z) > maxSlope) return false; // не на отвесе
		return true;
	};
	
	// 3) Спиральный поиск вокруг preferX/Z
	if (!good(bestX, bestZ)) {
		const float step = 8.0f; // метров между пробами
		const int   rings = static_cast<int>(searchRadiusMeters / step);
		bool found = false;
		for (int r = 1; r <= rings && !found; ++r) {
			for (int i = 0; i < 8 * r; ++i) { // дискретная спираль
				float angle = (i / static_cast<float>(8*r)) * 2.0f * 3.14159265359f; // 2π
				float x = preferX + r * step * std::cos(angle);
				float z = preferZ + r * step * std::sin(angle);
				clampXZ(x, z);
				if (good(x, z)) { bestX = x; bestZ = z; found = true; break; }
			}
		}
		// если не нашли — просто зажимаем к границе и берём, что есть
	}
	
	// 4) Установить позицию
	float h = chunkManager->evalSurfaceHeight(bestX, bestZ);
	camera->position = glm::vec3(bestX, h + heightOffset, bestZ);
	std::cout << "[SPAWN] Player at (" << bestX << "," << h+heightOffset << "," << bestZ
	          << ") surface=" << h << " water=" << waterLevel << std::endl;
}

