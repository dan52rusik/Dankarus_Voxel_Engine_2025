#include "PrefabManager.h"
#include "../voxels/ChunkManager.h"
#include "../utils/Rand.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>

namespace PrefabSystem {
    
    PrefabManager::PrefabManager(ChunkManager* chunkMgr)
        : chunkManager(chunkMgr), prefabInstanceId(0) {
    }
    
    PrefabManager::~PrefabManager() {
        Clear();
    }
    
    void PrefabManager::Clear() {
        streetTilesUsed.clear();
    }
    
    void PrefabManager::ClearDisplayed() {
        usedPrefabsWorld.clear();
        worldUsedPrefabNames.clear();
    }
    
    void PrefabManager::AddPrefab(const PrefabData& prefab) {
        prefabDataList.push_back(prefab);
        allPrefabDatas[prefab.name] = prefab;
    }
    
    PrefabData* PrefabManager::GetPrefabByName(const std::string& name) {
        auto it = allPrefabDatas.find(name);
        if (it != allPrefabDatas.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    bool PrefabManager::IsSizeValid(const PrefabData& prefab, const glm::ivec2& minSize, const glm::ivec2& maxSize) {
        if (maxSize.x > 0 || maxSize.y > 0) {
            if ((prefab.size.x > maxSize.x || prefab.size.y > maxSize.y) &&
                (prefab.size.y > maxSize.x || prefab.size.x > maxSize.y)) {
                return false;
            }
        }
        if (minSize.x > 0 || minSize.y > 0) {
            if ((prefab.size.x >= minSize.x && prefab.size.y >= minSize.y) ||
                (prefab.size.y >= minSize.x && prefab.size.x >= minSize.y)) {
                return true;
            }
            return false;
        }
        return true;
    }
    
    bool PrefabManager::IsThemeValid(const PrefabData& prefab,
                                     const glm::ivec2& prefabPos,
                                     const std::vector<PrefabInstance>& prefabInstances,
                                     int distance) const {
        if (prefab.themeTags.IsEmpty()) {
            return true;
        }
        
        int distanceSqr = distance * distance;
        
        for (const auto& instance : prefabInstances) {
            // Получаем префаб для этого экземпляра
            auto it = allPrefabDatas.find(instance.prefabName);
            if (it == allPrefabDatas.end()) {
                continue;
            }
            
            const PrefabData& otherPrefab = it->second;
            if (!otherPrefab.themeTags.IsEmpty() &&
                prefab.themeTags.TestAnySet(otherPrefab.themeTags)) {
                glm::ivec2 diff = prefabPos - instance.centerXZ;
                int distSqr = diff.x * diff.x + diff.y * diff.y;
                if (distSqr < distanceSqr) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    bool PrefabManager::IsNameValid(const PrefabData& prefab,
                                    const glm::ivec2& prefabPos,
                                    const std::vector<PrefabInstance>& prefabInstances,
                                    int distance) const {
        int distanceSqr = distance * distance;
        
        for (const auto& instance : prefabInstances) {
            if (instance.prefabName == prefab.name) {
                glm::ivec2 diff = prefabPos - instance.centerXZ;
                int distSqr = diff.x * diff.x + diff.y * diff.y;
                if (distSqr < distanceSqr) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    float PrefabManager::GetScoreForPrefab(const PrefabData& prefab, const glm::ivec2& center) const {
        float baseScore = 1.0f;
        float weight = 1.0f;
        
        // TODO: Получить биом из ChunkManager
        // BiomeDefinition::BiomeType biome = chunkManager->getBiomeAt(center.x, center.y);
        
        // Ищем данные весов для этого префаба
        const POIWeightData* weightData = nullptr;
        for (const auto& wd : prefabWeightData) {
            bool matches = false;
            
            // Проверяем частичное имя
            if (!wd.partialPOIName.empty() &&
                prefab.name.find(wd.partialPOIName) != std::string::npos) {
                matches = true;
            }
            
            // Проверяем теги
            if (!wd.tags.IsEmpty() &&
                (!prefab.tags.IsEmpty() && prefab.tags.TestAnySet(wd.tags) ||
                 !prefab.themeTags.IsEmpty() && prefab.themeTags.TestAnySet(wd.tags))) {
                matches = true;
            }
            
            if (matches) {
                // TODO: Проверка биома
                // if (!wd.biomeTags.IsEmpty() && !wd.biomeTags.TestAnySet(biomeTags)) {
                //     return std::numeric_limits<float>::lowest();
                // }
                
                weightData = &wd;
                break;
            }
        }
        
        if (weightData) {
            weight = weightData->weight;
            baseScore += weightData->bias;
            
            // Проверяем минимальное количество
            auto it = worldUsedPrefabNames.find(prefab.name);
            int count = (it != worldUsedPrefabNames.end()) ? it->second : 0;
            
            if (count < weightData->minCount) {
                baseScore += static_cast<float>(weightData->minCount - count);
            }
            
            // Проверяем максимальное количество
            if (count >= weightData->maxCount) {
                weight = 0.0f;
            }
        }
        
        // Добавляем оценку за сложность
        float difficultyScore = baseScore + static_cast<float>(prefab.difficultyTier) / 5.0f;
        
        // Уменьшаем оценку за дубликаты
        auto it = worldUsedPrefabNames.find(prefab.name);
        if (it != worldUsedPrefabNames.end()) {
            difficultyScore /= static_cast<float>(it->second + 1);
        }
        
        return difficultyScore * weight;
    }
    
    PrefabData* PrefabManager::GetPrefabWithDistrict(const glm::ivec2& center,
                                                     const PrefabTags& districtTags,
                                                     const PrefabTags& markerTags,
                                                     const glm::ivec2& minSize,
                                                     const glm::ivec2& maxSize,
                                                     float densityPointsLeft,
                                                     float distanceScale) {
        bool hasDistrictTags = !districtTags.IsEmpty();
        bool hasMarkerTags = !markerTags.IsEmpty();
        
        PrefabData* bestPrefab = nullptr;
        float bestScore = std::numeric_limits<float>::lowest();
        
        for (auto& prefab : prefabDataList) {
            // Проверяем плотность
            if (prefab.densityScore > densityPointsLeft) {
                continue;
            }
            
            // Проверяем теги частей и тайлов
            if (prefab.tags.TestAnySet(partsAndTilesTags)) {
                continue;
            }
            
            // Проверяем теги района
            if (hasDistrictTags && !prefab.tags.TestAllSet(districtTags)) {
                continue;
            }
            
            // Проверяем маркерные теги
            if (hasMarkerTags && !prefab.tags.TestAnySet(markerTags)) {
                continue;
            }
            
            // Проверяем размер
            if (!IsSizeValid(prefab, minSize, maxSize)) {
                continue;
            }
            
            // Проверяем тему
            int themeRepeatDist = prefab.themeRepeatDistance;
            if (!IsThemeValid(prefab, center, usedPrefabsWorld, themeRepeatDist)) {
                continue;
            }
            
            // Проверяем имя (дубликаты)
            int duplicateDist = static_cast<int>(prefab.duplicateRepeatDistance * distanceScale);
            if (distanceScale > 0.0f && !IsNameValid(prefab, center, usedPrefabsWorld, duplicateDist)) {
                continue;
            }
            
            // Вычисляем оценку
            float score = GetScoreForPrefab(prefab, center);
            if (score > bestScore) {
                bestScore = score;
                bestPrefab = &prefab;
            }
        }
        
        return bestPrefab;
    }
    
    PrefabData* PrefabManager::GetWildernessPrefab(const glm::ivec2& center,
                                                   const PrefabTags& withoutTags,
                                                   const PrefabTags& markerTags,
                                                   const glm::ivec2& minSize,
                                                   const glm::ivec2& maxSize,
                                                   bool isRetry) {
        PrefabData* bestPrefab = nullptr;
        float bestScore = std::numeric_limits<float>::lowest();
        
        for (auto& prefab : prefabDataList) {
            // Проверяем теги частей и тайлов
            if (prefab.tags.TestAnySet(partsAndTilesTags)) {
                continue;
            }
            
            // Проверяем теги дикой природы или торговца
            if (!prefab.tags.TestAnySet(wildernessTags) && !prefab.tags.TestAnySet(traderTags)) {
                continue;
            }
            
            // Проверяем маркерные теги
            if (!markerTags.IsEmpty() &&
                !prefab.tags.TestAnySet(markerTags) &&
                !prefab.themeTags.TestAnySet(markerTags)) {
                continue;
            }
            
            // Проверяем размер
            if (!IsSizeValid(prefab, minSize, maxSize)) {
                continue;
            }
            
            // Проверяем тему
            if (!IsThemeValid(prefab, center, usedPrefabsWorld, prefab.themeRepeatDistance)) {
                continue;
            }
            
            // Проверяем имя (если не повторная попытка)
            if (!isRetry && !IsNameValid(prefab, center, usedPrefabsWorld, prefab.duplicateRepeatDistance)) {
                continue;
            }
            
            // Вычисляем оценку
            float score = GetScoreForPrefab(prefab, center);
            if (score > bestScore) {
                bestScore = score;
                bestPrefab = &prefab;
            }
        }
        
        // Если не нашли и это не повторная попытка, пробуем еще раз без проверки имени
        if (bestPrefab == nullptr && !isRetry) {
            return GetWildernessPrefab(center, withoutTags, markerTags, minSize, maxSize, true);
        }
        
        return bestPrefab;
    }
    
    void PrefabManager::AddUsedPrefab(const std::string& prefabName) {
        auto it = worldUsedPrefabNames.find(prefabName);
        if (it != worldUsedPrefabNames.end()) {
            it->second++;
        } else {
            worldUsedPrefabNames[prefabName] = 1;
        }
    }
    
    void PrefabManager::AddUsedPrefabWorld(int instanceId, const PrefabInstance& instance) {
        usedPrefabsWorld.push_back(instance);
        AddUsedPrefab(instance.prefabName);
    }
    
    void PrefabManager::Shuffle(int seed, std::vector<PrefabData>& list) {
        std::mt19937 gen(seed);
        for (size_t i = list.size(); i > 1; i--) {
            std::uniform_int_distribution<size_t> dist(0, i - 1);
            size_t j = dist(gen);
            std::swap(list[i - 1], list[j]);
        }
    }
}

