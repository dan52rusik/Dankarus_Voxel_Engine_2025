#include "WorldBuilder.h"
#include "../voxels/ChunkManager.h"
#include "Path.h"
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
    
    // ИСПРАВЛЕНО: заполняем карту высот из ChunkManager для всех координат мира
    // Мир простирается от -worldSize/2 до +worldSize/2, но карта индексируется от 0 до worldSize-1
    for (int z = 0; z < worldSize; z++) {
        for (int x = 0; x < worldSize; x++) {
            int index = x + z * worldSize;
            // Преобразуем координаты карты в мировые координаты
            int worldX = x - worldSize / 2;
            int worldZ = z - worldSize / 2;
            heightMap[index] = chunkManager->evalSurfaceHeight(static_cast<float>(worldX), static_cast<float>(worldZ));
        }
    }
}

void WorldBuilder::GenerateRoads(int numHighways, int numCountryRoads) {
    // ВРЕМЕННО ОТКЛЮЧЕНО: дороги вызывают segfault из-за несоответствия координат
    // TODO: исправить координаты для PathingUtils (GRID vs WORLD)
    std::cout << "[WORLDBUILDER] Roads TEMPORARILY DISABLED (fixing coordinate system)" << std::endl;
    return;
    
    if (!pathingUtils || !rand || !chunkManager) {
        return;
    }
    
    std::cout << "[WORLDBUILDER] Generating " << numHighways << " highways and " << numCountryRoads << " country roads..." << std::endl;
    
    highways.clear();
    countryRoads.clear();
    
    // Находим "важные точки" для соединения дорогами
    // Это могут быть центры биомов, низины (для дорог), высокие точки (для обзора)
    std::vector<glm::ivec2> importantPoints;
    
    // Генерируем важные точки на основе биомов и рельефа
    for (int i = 0; i < numHighways * 2; i++) {
        glm::ivec2 pos = GetRandomWorldPosition();
        int idx = pos.x + pos.y * worldSize;
        if (idx >= 0 && idx < static_cast<int>(heightMap.size())) {
            float height = heightMap[idx];
            // Предпочитаем умеренные высоты (не слишком высоко, не слишком низко)
            if (height > 40.0f && height < 80.0f) {
                importantPoints.push_back(pos);
            }
        }
    }
    
    // Генерируем шоссе (соединяют важные точки)
    int highwaysPlaced = 0;
    for (int i = 0; i < numHighways * 2 && highwaysPlaced < numHighways; i++) {
        glm::ivec2 start, end;
        
        if (!importantPoints.empty() && rand->Float() < 0.7f) {
            // Используем важные точки
            start = importantPoints[rand->Range(static_cast<int>(importantPoints.size()))];
            if (importantPoints.size() > 1) {
                end = importantPoints[rand->Range(static_cast<int>(importantPoints.size()))];
                while (end == start && importantPoints.size() > 1) {
                    end = importantPoints[rand->Range(static_cast<int>(importantPoints.size()))];
                }
            } else {
                end = GetRandomWorldPosition();
            }
        } else {
            start = GetRandomWorldPosition();
            end = GetRandomWorldPosition();
        }
        
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
                highwaysPlaced++;
                std::cout << "[WORLDBUILDER] Generated highway " << highwaysPlaced << "/" << numHighways << std::endl;
            }
        }
    }
    
    // Генерируем проселочные дороги (соединяют шоссе и случайные точки)
    int countryRoadsPlaced = 0;
    for (int i = 0; i < numCountryRoads * 2 && countryRoadsPlaced < numCountryRoads; i++) {
        glm::ivec2 start, end;
        
        // 60% шанс начать от шоссе
        if (!highways.empty() && rand->Float() < 0.6f) {
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
        
        // Конечная точка - либо другая дорога, либо важная точка, либо случайная
        if (!highways.empty() && rand->Float() < 0.4f) {
            const auto& highway = highways[rand->Range(static_cast<int>(highways.size()))];
            const auto& points = highway->GetFinalPathPoints();
            if (!points.empty()) {
                int pointIdx = rand->Range(static_cast<int>(points.size()));
                end = glm::ivec2(static_cast<int>(points[pointIdx].x), static_cast<int>(points[pointIdx].y));
            } else {
                end = GetRandomWorldPosition();
            }
        } else if (!importantPoints.empty() && rand->Float() < 0.3f) {
            end = importantPoints[rand->Range(static_cast<int>(importantPoints.size()))];
        } else {
            end = GetRandomWorldPosition();
        }
        
        // Минимальное расстояние
        glm::ivec2 diff = end - start;
        int distSqr = diff.x * diff.x + diff.y * diff.y;
        if (distSqr < 2500) { // Минимум 50 единиц
            continue;
        }
        
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
                countryRoadsPlaced++;
            }
        }
    }
    
    std::cout << "[WORLDBUILDER] Generated " << highways.size() << " highways and " << countryRoads.size() << " country roads" << std::endl;
}

void WorldBuilder::GenerateWaterFeatures(int numLakes, int numRivers) {
    if (!stampManager || !rand || !chunkManager) {
        return;
    }
    
    std::cout << "[WORLDBUILDER] Generating " << numLakes << " lakes and " << numRivers << " rivers..." << std::endl;
    
    Stamping::StampGroup lakeGroup("lakes");
    Stamping::StampGroup riverGroup("rivers");
    
    // Генерируем озера в низинах
    int lakesPlaced = 0;
    for (int i = 0; i < numLakes * 3 && lakesPlaced < numLakes; i++) {
        glm::ivec2 pos = GetRandomWorldPosition();
        
        if (!IsValidPositionForWater(pos)) {
            continue;
        }
        
        // ИСПРАВЛЕНО: преобразуем мировые координаты в индексы карты (смещение на worldSize/2)
        int mapX = pos.x + worldSize / 2;
        int mapZ = pos.y + worldSize / 2;
        if (mapX < 0 || mapX >= worldSize || mapZ < 0 || mapZ >= worldSize) {
            continue;
        }
        int index = mapX + mapZ * worldSize;
        if (index < 0 || index >= static_cast<int>(heightMap.size())) {
            continue;
        }
        
        float centerHeight = heightMap[index];
        
        // Проверяем среднюю высоту вокруг (в радиусе 32 единицы)
        float avgHeight = 0.0f;
        int count = 0;
        for (int dz = -16; dz <= 16; dz++) {
            for (int dx = -16; dx <= 16; dx++) {
                int nx = pos.x + dx;
                int nz = pos.y + dz;
                // Преобразуем мировые координаты в координаты карты
                int nmapX = nx + worldSize / 2;
                int nmapZ = nz + worldSize / 2;
                if (nmapX >= 0 && nmapX < worldSize && nmapZ >= 0 && nmapZ < worldSize) {
                    int nidx = nmapX + nmapZ * worldSize;
                    if (nidx >= 0 && nidx < static_cast<int>(heightMap.size())) {
                        avgHeight += heightMap[nidx];
                        count++;
                    }
                }
            }
        }
        if (count > 0) {
            avgHeight /= count;
        }
        
        // ИСПРАВЛЕНО: озеро даже при небольшой низине (мир теперь почти плоский)
        if (centerHeight < avgHeight - 1.5f) { // было 5.0f
            // Гарантируем ощутимую глубину даже при маленьких перепадах
            float depth = (avgHeight - centerHeight) + 1.5f;
            
            int lakeSize = static_cast<int>(32.0f + depth * 2.0f + rand->Range(32));
            lakeSize = std::min(lakeSize, 128); // Максимум 128
            
            auto lakeStamp = std::make_shared<Stamping::RawStamp>();
            lakeStamp->name = "lake_" + std::to_string(lakesPlaced);
            lakeStamp->width = lakeSize;
            lakeStamp->height = lakeSize;
            lakeStamp->heightConst = -depth * 0.6f; // ИСПРАВЛЕНО: чуть глубже, чтобы прям "ямы" были (было 0.5f)
            lakeStamp->alphaConst = 1.0f;
            
            // Создаем более естественную форму озера (не идеальный круг)
            lakeStamp->alphaPixels.resize(lakeSize * lakeSize);
            lakeStamp->waterPixels.resize(lakeSize * lakeSize);
            int centerX = lakeSize / 2;
            int centerY = lakeSize / 2;
            float baseRadius = static_cast<float>(lakeSize) * 0.35f;
            
            for (int y = 0; y < lakeSize; y++) {
                for (int x = 0; x < lakeSize; x++) {
                    float dx = static_cast<float>(x - centerX);
                    float dy = static_cast<float>(y - centerY);
                    float dist = std::sqrt(dx * dx + dy * dy);
                    
                    // Добавляем шум для более естественной формы
                    float noiseX = static_cast<float>(pos.x + x - centerX) * 0.1f;
                    float noiseZ = static_cast<float>(pos.y + y - centerY) * 0.1f;
                    float noise = chunkManager->evalSurfaceHeight(noiseX, noiseZ) * 0.1f;
                    float radius = baseRadius + noise;
                    
                    float alpha = 1.0f - std::min(dist / radius, 1.0f);
                    alpha = std::max(0.0f, alpha);
                    lakeStamp->alphaPixels[x + y * lakeSize] = alpha;
                    
                    // Вода только внутри радиуса
                    if (dist < radius) {
                        // ИСПРАВЛЕНО: вода ощутимо выше дна, но не вровень с краями
                        float waterLevel = centerHeight + depth * 0.4f; // было 0.3f
                        lakeStamp->waterPixels[x + y * lakeSize] = waterLevel;
                    } else {
                        lakeStamp->waterPixels[x + y * lakeSize] = 0.0f;
                    }
                }
            }
            
            stampManager->AddRawStamp(lakeStamp->name, lakeStamp);
            
            Stamping::StampTransform transform(pos.x, pos.y, rand->Angle(), 1.0f);
            Stamping::Stamp stamp = stampManager->CreateStamp(lakeStamp->name, transform, false, glm::vec4(1.0f), 0.1f, true, "lake_" + std::to_string(lakesPlaced));
            lakeGroup.stamps.push_back(stamp);
            lakesPlaced++;
        }
    }
    
    // Генерируем реки (текут по низинам от высоких точек к низким)
    int riversPlaced = 0;
    for (int i = 0; i < numRivers * 2 && riversPlaced < numRivers; i++) {
        // Начинаем реку с высокой точки
        glm::ivec2 start = GetRandomWorldPosition();
        // ИСПРАВЛЕНО: преобразуем мировые координаты в индексы карты
        int startMapX = start.x + worldSize / 2;
        int startMapZ = start.y + worldSize / 2;
        if (startMapX < 0 || startMapX >= worldSize || startMapZ < 0 || startMapZ >= worldSize) {
            continue;
        }
        int startIdx = startMapX + startMapZ * worldSize;
        if (startIdx < 0 || startIdx >= static_cast<int>(heightMap.size())) {
            continue;
        }
        
        float startHeight = heightMap[startIdx];
        
        // ИСПРАВЛЕНО: мир почти плоский – разрешаем начинать реку с любой небольшой возвышенности
        // Минимальная высота для старта реки - чуть выше уровня воды
        if (startHeight < 46.0f) {    // было 47.0f, теперь ещё ниже для плоского мира
            continue;
        }
        
        // Находим конечную точку (низкая точка в направлении потока)
        glm::ivec2 end = start;
        float minHeight = startHeight;
        
        // Ищем низкую точку в случайном направлении
        float angle = rand->Angle();
        for (int step = 0; step < 200; step++) {
            glm::ivec2 next = start + glm::ivec2(
                static_cast<int>(std::cos(angle) * step * 2.0f),
                static_cast<int>(std::sin(angle) * step * 2.0f)
            );
            
            // ИСПРАВЛЕНО: преобразуем мировые координаты в индексы карты
            int nextMapX = next.x + worldSize / 2;
            int nextMapZ = next.y + worldSize / 2;
            if (nextMapX < 0 || nextMapX >= worldSize || nextMapZ < 0 || nextMapZ >= worldSize) {
                break;
            }
            
            int nextIdx = nextMapX + nextMapZ * worldSize;
            if (nextIdx >= 0 && nextIdx < static_cast<int>(heightMap.size())) {
                float h = heightMap[nextIdx];
                if (h < minHeight) {
                    minHeight = h;
                    end = next;
                }
            }
        }
        
        // ИСПРАВЛЕНО: если нашли конечную точку хотя бы немного ниже старта
        if (end != start && minHeight < startHeight - 3.0f) {   // было -10.0f
            // Создаем реку как путь от start до end
            auto riverPath = std::make_unique<Pathfinding::Path>(
                chunkManager, pathingUtils.get(),
                start, end,
                1,  // 1 полоса для реки
                true  // country road type
            );
            
            if (riverPath->IsValid()) {
                // Рисуем реку на карте воды
                const auto& points = riverPath->GetFinalPathPoints();
                for (const auto& point : points) {
                    int px = static_cast<int>(point.x);
                    int pz = static_cast<int>(point.y);
                    // ИСПРАВЛЕНО: преобразуем мировые координаты в индексы карты
                    int pmapX = px + worldSize / 2;
                    int pmapZ = pz + worldSize / 2;
                    if (pmapX >= 0 && pmapX < worldSize && pmapZ >= 0 && pmapZ < worldSize) {
                        int pidx = pmapX + pmapZ * worldSize;
                        if (pidx >= 0 && pidx < static_cast<int>(waterMap.size())) {
                            // ИСПРАВЛЕНО: река имеет уровень воды немного ниже поверхности
                            float surfaceH = heightMap[pidx];
                            float riverWater = surfaceH - 2.0f;
                            
                            // Стягиваем уровень рек к морю, чтобы они логично впадали в большой водоём
                            const float targetSeaLevel = 45.0f;          // не забудь: если поменяешь waterLevel – обнови и тут
                            riverWater = std::max(riverWater, targetSeaLevel - 1.0f);
                            
                            // Записываем итоговый уровень воды реки
                            waterMap[pidx] = std::max(waterMap[pidx], riverWater);
                            
                            // Расширяем реку в стороны (ширина 3-5 единиц)
                            for (int dx = -2; dx <= 2; dx++) {
                                for (int dz = -2; dz <= 2; dz++) {
                                    int nx = px + dx;
                                    int nz = pz + dz;
                                    // ИСПРАВЛЕНО: преобразуем мировые координаты в индексы карты
                                    int nmapX = nx + worldSize / 2;
                                    int nmapZ = nz + worldSize / 2;
                                    if (nmapX >= 0 && nmapX < worldSize && nmapZ >= 0 && nmapZ < worldSize) {
                                        int nidx = nmapX + nmapZ * worldSize;
                                        if (nidx >= 0 && nidx < static_cast<int>(waterMap.size())) {
                                            float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));
                                            if (dist < 2.5f) {
                                                // ИСПРАВЛЕНО: стягиваем уровень рек к морю
                                                float surfaceHN = heightMap[nidx];
                                                float riverWaterN = surfaceHN - 2.0f;
                                                const float targetSeaLevel = 45.0f;
                                                riverWaterN = std::max(riverWaterN, targetSeaLevel - 1.0f);
                                                waterMap[nidx] = std::max(waterMap[nidx], riverWaterN);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                riversPlaced++;
            }
        }
    }
    
    // Рисуем озера на карте
    if (!lakeGroup.stamps.empty()) {
        stampManager->DrawStampGroup(lakeGroup, heightMap.data(), waterMap.data(), worldSize);
        std::cout << "[WORLDBUILDER] Generated " << lakeGroup.stamps.size() << " lakes" << std::endl;
    }
    
    std::cout << "[WORLDBUILDER] Generated " << riversPlaced << " rivers" << std::endl;
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
    
    // ИСПРАВЛЕНО: генерируем позиции по всему миру, включая отрицательные координаты
    // Мир простирается от -worldSize/2 до +worldSize/2, но waterMap индексируется от 0 до worldSize-1
    // Поэтому генерируем позиции в диапазоне от -worldSize/2 до +worldSize/2
    int halfSize = worldSize / 2;
    int margin = worldSize / 10;
    int x = -halfSize + margin + rand->Range(worldSize - 2 * margin);
    int y = -halfSize + margin + rand->Range(worldSize - 2 * margin);
    return glm::ivec2(x, y);
}

bool WorldBuilder::IsValidPositionForRoad(const glm::ivec2& pos) const {
    // ИСПРАВЛЕНО: преобразуем мировые координаты в индексы карты
    int mapX = pos.x + worldSize / 2;
    int mapZ = pos.y + worldSize / 2;
    if (mapX < 0 || mapX >= worldSize || mapZ < 0 || mapZ >= worldSize) {
        return false;
    }
    
    // Проверяем, что позиция не в воде (упрощенная проверка)
    int index = mapX + mapZ * worldSize;
    if (index >= 0 && index < static_cast<int>(waterMap.size())) {
        if (waterMap[index] > 0.0f) {
            return false;
        }
    }
    
    return true;
}

bool WorldBuilder::IsValidPositionForWater(const glm::ivec2& pos) const {
    // ИСПРАВЛЕНО: преобразуем мировые координаты в индексы карты
    int mapX = pos.x + worldSize / 2;
    int mapZ = pos.y + worldSize / 2;
    if (mapX < 0 || mapX >= worldSize || mapZ < 0 || mapZ >= worldSize) {
        return false;
    }
    
    // ИСПРАВЛЕНО: в плоском мире озёра могут быть везде, где есть небольшая впадина
    // Убираем проверку на высоту - в плоском мире нет высоких гор
    // int index = mapX + mapZ * worldSize;
    // if (index >= 0 && index < static_cast<int>(heightMap.size())) {
    //     float height = heightMap[index];
    //     // Озера только в низинах (ниже среднего уровня)
    //     if (height > 60.0f) { // Примерный порог
    //         return false;
    //     }
    // }
    
    return true;
}

glm::ivec2 WorldBuilder::WorldToGrid(const glm::ivec2& world) const {
    int half = worldSize / 2;
    return glm::ivec2(world.x + half, world.y + half);
}

glm::ivec2 WorldBuilder::GridToWorld(const glm::ivec2& grid) const {
    int half = worldSize / 2;
    return glm::ivec2(grid.x - half, grid.y - half);
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

