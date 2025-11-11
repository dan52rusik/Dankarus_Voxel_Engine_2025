#ifndef VOXELS_PREFABMANAGER_H_
#define VOXELS_PREFABMANAGER_H_

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

// Forward declarations
class ChunkManager;

// Улучшенный менеджер префабов с проверкой расстояний и тем
// Адаптировано из 7 Days To Die PrefabManager
namespace PrefabSystem {
    
    // Теги для префабов (упрощенная версия)
    struct PrefabTags {
        std::unordered_set<std::string> tags;
        std::unordered_set<std::string> themeTags;
        
        bool IsEmpty() const { return tags.empty() && themeTags.empty(); }
        
        // Проверить, содержит ли хотя бы один из указанных тегов
        bool TestAnySet(const PrefabTags& other) const {
            for (const auto& tag : other.tags) {
                if (tags.find(tag) != tags.end()) {
                    return true;
                }
            }
            for (const auto& tag : other.themeTags) {
                if (themeTags.find(tag) != themeTags.end()) {
                    return true;
                }
            }
            return false;
        }
        
        // Проверить, содержит ли все указанные теги
        bool TestAllSet(const PrefabTags& other) const {
            for (const auto& tag : other.tags) {
                if (tags.find(tag) == tags.end()) {
                    return false;
                }
            }
            return true;
        }
    };
    
    // Данные префаба
    struct PrefabData {
        std::string name;
        glm::ivec2 size;              // Размер префаба (x, z)
        float densityScore;           // Оценка плотности
        int difficultyTier;           // Уровень сложности
        int themeRepeatDistance;      // Минимальное расстояние между префабами с одинаковой темой
        int duplicateRepeatDistance;  // Минимальное расстояние между одинаковыми префабами
        PrefabTags tags;              // Теги префаба
        PrefabTags themeTags;         // Теги темы
        
        PrefabData() : size(0, 0), densityScore(0.0f), difficultyTier(0),
                      themeRepeatDistance(0), duplicateRepeatDistance(0) {}
    };
    
    // Экземпляр размещенного префаба
    struct PrefabInstance {
        int id;
        std::string prefabName;
        glm::ivec2 centerXZ;          // Центр префаба (x, z)
        glm::ivec3 boundingBoxMin;    // Минимальная граница
        glm::ivec3 boundingBoxMax;    // Максимальная граница
        float rotation;               // Поворот
        
        PrefabInstance() : id(-1), rotation(0.0f) {}
    };
    
    // Данные весов для префабов
    struct POIWeightData {
        std::string partialPOIName;   // Частичное имя POI
        PrefabTags tags;              // Теги
        PrefabTags biomeTags;         // Теги биома
        float weight;                 // Вес
        float bias;                   // Смещение
        int minCount;                 // Минимальное количество
        int maxCount;                 // Максимальное количество
        
        POIWeightData() : weight(1.0f), bias(0.0f), minCount(0), maxCount(999) {}
    };
    
    class PrefabManager {
    private:
        ChunkManager* chunkManager;
        std::vector<PrefabData> prefabDataList;
        std::unordered_map<std::string, PrefabData> allPrefabDatas;
        std::vector<PrefabInstance> usedPrefabsWorld;
        std::unordered_map<std::string, int> worldUsedPrefabNames;
        std::unordered_map<std::string, int> streetTilesUsed;
        
        std::vector<POIWeightData> prefabWeightData;
        PrefabTags partsAndTilesTags;
        PrefabTags wildernessTags;
        PrefabTags traderTags;
        
        int prefabInstanceId;
        
    public:
        PrefabManager(ChunkManager* chunkMgr);
        ~PrefabManager();
        
        // Очистить данные
        void Clear();
        void ClearDisplayed();
        
        // Добавить префаб
        void AddPrefab(const PrefabData& prefab);
        
        // Получить префаб по имени
        PrefabData* GetPrefabByName(const std::string& name);
        
        // Получить префаб для района с проверкой расстояний и тем
        PrefabData* GetPrefabWithDistrict(const glm::ivec2& center,
                                         const PrefabTags& districtTags,
                                         const PrefabTags& markerTags,
                                         const glm::ivec2& minSize,
                                         const glm::ivec2& maxSize,
                                         float densityPointsLeft,
                                         float distanceScale);
        
        // Получить префаб для дикой природы
        PrefabData* GetWildernessPrefab(const glm::ivec2& center,
                                       const PrefabTags& withoutTags,
                                       const PrefabTags& markerTags,
                                       const glm::ivec2& minSize = glm::ivec2(0, 0),
                                       const glm::ivec2& maxSize = glm::ivec2(0, 0),
                                       bool isRetry = false);
        
        // Добавить использованный префаб
        void AddUsedPrefab(const std::string& prefabName);
        void AddUsedPrefabWorld(int instanceId, const PrefabInstance& instance);
        
        // Получить все использованные префабы
        const std::vector<PrefabInstance>& GetUsedPrefabsWorld() const { return usedPrefabsWorld; }
        
        // Проверить валидность размера
        static bool IsSizeValid(const PrefabData& prefab, const glm::ivec2& minSize, const glm::ivec2& maxSize);
        
        // Перемешать список префабов (специализация для PrefabData)
        static void Shuffle(int seed, std::vector<PrefabData>& list);
        
    private:
        // Проверить валидность темы (расстояние между префабами с одинаковой темой)
        bool IsThemeValid(const PrefabData& prefab,
                         const glm::ivec2& prefabPos,
                         const std::vector<PrefabInstance>& prefabInstances,
                         int distance) const;
        
        // Проверить валидность имени (расстояние между одинаковыми префабами)
        bool IsNameValid(const PrefabData& prefab,
                        const glm::ivec2& prefabPos,
                        const std::vector<PrefabInstance>& prefabInstances,
                        int distance) const;
        
        // Получить оценку для префаба
        float GetScoreForPrefab(const PrefabData& prefab, const glm::ivec2& center) const;
    };
    
}

#endif /* VOXELS_PREFABMANAGER_H_ */

