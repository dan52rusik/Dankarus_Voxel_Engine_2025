#ifndef VOXELS_WORLDBUILDER_H_
#define VOXELS_WORLDBUILDER_H_

#include "PathingUtils.h"
#include "Path.h"
#include "StampManager.h"
#include "PrefabManager.h"
#include "../utils/Rand.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class ChunkManager;

// Главный класс для генерации мира
// Координирует все системы генерации (дороги, штампы, префабы)
// Адаптировано из 7 Days To Die WorldBuilder
class WorldBuilder {
public:
    WorldBuilder(ChunkManager* chunkManager);
    ~WorldBuilder();
    
    // Инициализация генерации мира
    void Initialize(int worldSize, int64_t seed);
    
    // Генерация основных элементов мира
    void GenerateRoads(int numHighways = 5, int numCountryRoads = 20);
    void GenerateWaterFeatures(int numLakes = 10, int numRivers = 5);
    void GeneratePrefabs(int numPrefabs = 50);
    
    // Получить карту дорог (для визуализации или использования)
    const std::vector<uint8_t>& GetRoadMap() const { return roadMap; }
    
    // Получить карту воды (для визуализации)
    const std::vector<float>& GetWaterMap() const { return waterMap; }
    
    // Очистить все данные
    void Clear();
    
    // Получить размер мира
    int GetWorldSize() const { return worldSize; }
    
private:
    ChunkManager* chunkManager;
    
    // Системы генерации
    std::unique_ptr<Pathfinding::PathingUtils> pathingUtils;
    std::unique_ptr<Stamping::StampManager> stampManager;
    std::unique_ptr<PrefabSystem::PrefabManager> prefabManager;
    Rand::Rand* rand;
    
    // Параметры мира
    int worldSize;
    int64_t seed;
    
    // Карты
    std::vector<uint8_t> roadMap;      // Карта дорог (worldSize * worldSize)
    std::vector<float> heightMap;       // Карта высот (worldSize * worldSize)
    std::vector<float> waterMap;        // Карта воды (worldSize * worldSize)
    
    // Сгенерированные пути
    std::vector<std::unique_ptr<Pathfinding::Path>> highways;
    std::vector<std::unique_ptr<Pathfinding::Path>> countryRoads;
    
    // Вспомогательные функции
    void InitializeMaps();
    glm::ivec2 GetRandomWorldPosition();
    bool IsValidPositionForRoad(const glm::ivec2& pos) const;
    bool IsValidPositionForWater(const glm::ivec2& pos) const;
    
    // ИСПРАВЛЕНО: конвертеры между мировыми координатами и координатами сетки
    // Мировые координаты: [-worldSize/2 .. +worldSize/2]
    // Координаты сетки (GRID): [0 .. worldSize-1] (для PathingUtils, heightMap, waterMap)
    glm::ivec2 WorldToGrid(const glm::ivec2& world) const;
    glm::ivec2 GridToWorld(const glm::ivec2& grid) const;
};

#endif /* VOXELS_WORLDBUILDER_H_ */

