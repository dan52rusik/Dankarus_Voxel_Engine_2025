#include "WorldBuilder.h"
#include "../voxels/ChunkManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WorldBuilder::WorldBuilder(ChunkManager* chunkManager)
    : chunkManager(chunkManager),
      worldSize(0),
      seed(0),
      rand(nullptr) {
}

WorldBuilder::~WorldBuilder() {
    Clear();
}

void WorldBuilder::Initialize(int worldSize, int64_t seed) {
    this->worldSize = worldSize;
    this->seed = seed;
    
    // Инициализируем Rand
    rand = &Rand::GetInstance();
    rand->SetSeed(static_cast<int>(seed));
    
    // Инициализируем PathingUtils
    pathingUtils = std::make_unique<Pathfinding::PathingUtils>(chunkManager);
    pathingUtils->SetupPathingGrid(worldSize);
    
    // Инициализируем StampManager
    stampManager = std::make_unique<Stamping::StampManager>(chunkManager);
    
    // Инициализируем PrefabManager
    prefabManager = std::make_unique<PrefabSystem::PrefabManager>(chunkManager);
    
    // Инициализируем карты
    InitializeMaps();
}

void WorldBuilder::InitializeMaps() {
    int mapSize = worldSize * worldSize;
    roadMap.resize(mapSize, 0);
    heightMap.resize(mapSize, 0.0f);
    waterMap.resize(mapSize, 0.0f);
    
    // Заполняем карту высот из ChunkManager
    for (int z = 0; z < worldSize; z++) {
        for (int x = 0; x < worldSize; x++) {
            int index = x + z * worldSize;
            heightMap[index] = chunkManager->evalSurfaceHeight(static_cast<float>(x), static_cast<float>(z));
        }
    }
}

void WorldBuilder::GenerateRoads(int numHighways, int numCountryRoads) {
    if (!pathingUtils || !rand) {
        return;
    }
    
    std::cout << "[WORLDBUILDER] Generating " << numHighways << " highways and " << numCountryRoads << " country roads..." << std::endl;
    
    highways.clear();
    countryRoads.clear();
    
    // Генерируем шоссе (соединяют важные точки)
    for (int i = 0; i < numHighways; i++) {
        glm::ivec2 start = GetRandomWorldPosition();
        glm::ivec2 end = GetRandomWorldPosition();
        
        // Убеждаемся, что точки достаточно далеко друг от друга
        glm::ivec2 diff = end - start;
        int distSqr = diff.x * diff.x + diff.y * diff.y;
        if (distSqr < 10000) { // Минимум 100 единиц
            continue;
        }
        
        if (IsValidPositionForRoad(start) && IsValidPositionForRoad(end)) {
            auto highway = std::make_unique<Pathfinding::Path>(
                chunkManager, pathingUtils.get(),
                start, end,
                2,  // 2 полосы
                false  // false = шоссе
            );
            
            if (highway->IsValid()) {
                highways.push_back(std::move(highway));
                // Рисуем дорогу на карте
                highways.back()->DrawPathToRoadIds(roadMap.data(), worldSize);
                std::cout << "[WORLDBUILDER] Generated highway " << (i + 1) << "/" << numHighways << std::endl;
            }
        }
    }
    
    // Генерируем проселочные дороги (соединяют шоссе и случайные точки)
    for (int i = 0; i < numCountryRoads; i++) {
        glm::ivec2 start, end;
        
        // 50% шанс начать от шоссе
        if (!highways.empty() && rand->Float() < 0.5f) {
            const auto& highway = highways[rand->Range(static_cast<int>(highways.size()))];
            const auto& points = highway->GetFinalPathPoints();
            if (!points.empty()) {
                int pointIdx = rand->Range(static_cast<int>(points.size()));
                start = glm::ivec2(static_cast<int>(points[pointIdx].x), static_cast<int>(points[pointIdx].y));
            } else {
                start = GetRandomWorldPosition();
            }
        } else {
            start = GetRandomWorldPosition();
        }
        
        end = GetRandomWorldPosition();
        
        if (IsValidPositionForRoad(start) && IsValidPositionForRoad(end)) {
            auto countryRoad = std::make_unique<Pathfinding::Path>(
                chunkManager, pathingUtils.get(),
                start, end,
                1,  // 1 полоса
                true  // true = проселочная дорога
            );
            
            if (countryRoad->IsValid()) {
                countryRoads.push_back(std::move(countryRoad));
                // Рисуем дорогу на карте
                countryRoads.back()->DrawPathToRoadIds(roadMap.data(), worldSize);
            }
        }
    }
    
    std::cout << "[WORLDBUILDER] Generated " << highways.size() << " highways and " << countryRoads.size() << " country roads" << std::endl;
}

void WorldBuilder::GenerateWaterFeatures(int numLakes, int numRivers) {
    if (!stampManager || !rand) {
        return;
    }
    
    std::cout << "[WORLDBUILDER] Generating " << numLakes << " lakes and " << numRivers << " rivers..." << std::endl;
    
    // Создаем простые штампы для озер (можно расширить для загрузки из файлов)
    // Для демонстрации создаем простые круглые озера
    
    Stamping::StampGroup lakeGroup("lakes");
    Stamping::StampGroup riverGroup("rivers");
    
    for (int i = 0; i < numLakes; i++) {
        glm::ivec2 pos = GetRandomWorldPosition();
        
        if (IsValidPositionForWater(pos)) {
            // Создаем простой штамп озера (в реальности нужно загрузить из файла)
            // Для демонстрации используем простой круглый штамп
            auto lakeStamp = std::make_shared<Stamping::RawStamp>();
            lakeStamp->name = "lake_" + std::to_string(i);
            int lakeSize = 64 + rand->Range(64); // 64-128 единиц
            lakeStamp->width = lakeSize;
            lakeStamp->height = lakeSize;
            lakeStamp->heightConst = -5.0f; // Понижаем высоту
            lakeStamp->alphaConst = 1.0f;
            
            // Создаем простой круглый альфа-канал
            lakeStamp->alphaPixels.resize(lakeSize * lakeSize);
            int centerX = lakeSize / 2;
            int centerY = lakeSize / 2;
            float radius = static_cast<float>(lakeSize) * 0.4f;
            
            for (int y = 0; y < lakeSize; y++) {
                for (int x = 0; x < lakeSize; x++) {
                    float dist = std::sqrt(static_cast<float>((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY)));
                    float alpha = 1.0f - std::min(dist / radius, 1.0f);
                    lakeStamp->alphaPixels[x + y * lakeSize] = alpha;
                }
            }
            
            // Создаем водные пиксели
            lakeStamp->waterPixels.resize(lakeSize * lakeSize);
            float waterLevel = 20.0f; // Уровень воды
            for (int y = 0; y < lakeSize; y++) {
                for (int x = 0; x < lakeSize; x++) {
                    float dist = std::sqrt(static_cast<float>((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY)));
                    if (dist < radius) {
                        lakeStamp->waterPixels[x + y * lakeSize] = waterLevel;
                    } else {
                        lakeStamp->waterPixels[x + y * lakeSize] = 0.0f;
                    }
                }
            }
            
            stampManager->AddRawStamp(lakeStamp->name, lakeStamp);
            
            Stamping::StampTransform transform(pos.x, pos.y, rand->Angle(), 1.0f);
            Stamping::Stamp stamp = stampManager->CreateStamp(lakeStamp->name, transform, false, glm::vec4(1.0f), 0.1f, true, "lake_" + std::to_string(i));
            lakeGroup.stamps.push_back(stamp);
        }
    }
    
    // Рисуем озера на карте
    if (!lakeGroup.stamps.empty()) {
        stampManager->DrawStampGroup(lakeGroup, heightMap.data(), waterMap.data(), worldSize);
        std::cout << "[WORLDBUILDER] Generated " << lakeGroup.stamps.size() << " lakes" << std::endl;
    }
}

void WorldBuilder::GeneratePrefabs(int numPrefabs) {
    if (!prefabManager || !rand) {
        return;
    }
    
    std::cout << "[WORLDBUILDER] Generating " << numPrefabs << " prefabs..." << std::endl;
    
    // Добавляем несколько примеров префабов
    for (int i = 0; i < 10; i++) {
        PrefabSystem::PrefabData prefab;
        prefab.name = "prefab_" + std::to_string(i);
        prefab.size = glm::ivec2(10 + rand->Range(20), 10 + rand->Range(20));
        prefab.densityScore = rand->Float();
        prefab.difficultyTier = rand->Range(1, 5);
        prefab.themeRepeatDistance = 100 + rand->Range(200);
        prefab.duplicateRepeatDistance = 200 + rand->Range(300);
        
        // Добавляем теги
        if (rand->Float() < 0.5f) {
            prefab.tags.tags.insert("residential");
        } else {
            prefab.tags.tags.insert("commercial");
        }
        
        prefabManager->AddPrefab(prefab);
    }
    
    // Размещаем префабы
    PrefabSystem::PrefabTags districtTags;
    districtTags.tags.insert("residential");
    
    PrefabSystem::PrefabTags markerTags;
    
    int placed = 0;
    for (int i = 0; i < numPrefabs; i++) {
        glm::ivec2 center = GetRandomWorldPosition();
        
        PrefabSystem::PrefabData* prefab = prefabManager->GetPrefabWithDistrict(
            center,
            districtTags,
            markerTags,
            glm::ivec2(5, 5),
            glm::ivec2(30, 30),
            1.0f,
            1.0f
        );
        
        if (prefab) {
            PrefabSystem::PrefabInstance instance;
            instance.id = placed++;
            instance.prefabName = prefab->name;
            instance.centerXZ = center;
            instance.boundingBoxMin = glm::ivec3(center.x - prefab->size.x / 2, 0, center.y - prefab->size.y / 2);
            instance.boundingBoxMax = glm::ivec3(center.x + prefab->size.x / 2, 50, center.y + prefab->size.y / 2);
            instance.rotation = rand->Angle();
            
            prefabManager->AddUsedPrefabWorld(instance.id, instance);
        }
    }
    
    std::cout << "[WORLDBUILDER] Placed " << placed << " prefabs" << std::endl;
}

glm::ivec2 WorldBuilder::GetRandomWorldPosition() {
    if (!rand) {
        return glm::ivec2(0, 0);
    }
    
    // Оставляем отступ от краев
    int margin = worldSize / 10;
    int x = margin + rand->Range(worldSize - 2 * margin);
    int y = margin + rand->Range(worldSize - 2 * margin);
    return glm::ivec2(x, y);
}

bool WorldBuilder::IsValidPositionForRoad(const glm::ivec2& pos) const {
    if (pos.x < 0 || pos.x >= worldSize || pos.y < 0 || pos.y >= worldSize) {
        return false;
    }
    
    // Проверяем, что позиция не в воде (упрощенная проверка)
    int index = pos.x + pos.y * worldSize;
    if (index >= 0 && index < static_cast<int>(waterMap.size())) {
        if (waterMap[index] > 0.0f) {
            return false;
        }
    }
    
    return true;
}

bool WorldBuilder::IsValidPositionForWater(const glm::ivec2& pos) const {
    if (pos.x < 0 || pos.x >= worldSize || pos.y < 0 || pos.y >= worldSize) {
        return false;
    }
    
    // Проверяем, что позиция не слишком высокая (озера в низинах)
    int index = pos.x + pos.y * worldSize;
    if (index >= 0 && index < static_cast<int>(heightMap.size())) {
        float height = heightMap[index];
        // Озера только в низинах (ниже среднего уровня)
        if (height > 60.0f) { // Примерный порог
            return false;
        }
    }
    
    return true;
}

void WorldBuilder::Clear() {
    highways.clear();
    countryRoads.clear();
    roadMap.clear();
    heightMap.clear();
    waterMap.clear();
    
    if (pathingUtils) {
        pathingUtils->Cleanup();
    }
    if (stampManager) {
        stampManager->ClearStamps();
    }
    if (prefabManager) {
        prefabManager->Clear();
    }
}

