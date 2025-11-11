#ifndef VOXELS_PATH_H_
#define VOXELS_PATH_H_

#include "PathingUtils.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

// Forward declaration
class ChunkManager;

// Класс для представления дороги (highway, country road, river)
// Адаптировано из 7 Days To Die Path
namespace Pathfinding {
    // Константы для типов дорог
    constexpr uint8_t PATH_FREE = 0;
    constexpr uint8_t PATH_COUNTRY = 1;
    constexpr uint8_t PATH_HIGHWAY = 2;
    constexpr uint8_t PATH_HIGHWAY_DIRT = 3;
    constexpr uint8_t PATH_WATER = 4;
    constexpr uint8_t PATH_HIGHWAY_BLEND_MASK = 128; // 0x80
    
    // Константы для радиусов дорог
    constexpr float SINGLE_LANE_RADIUS = 4.5f;
    constexpr int SHOULDER_WIDTH = 1;
    constexpr float BLEND_DIST_COUNTRY = 6.0f;
    constexpr float BLEND_DIST_HIGHWAY = 10.0f;
    constexpr float HEIGHT_SMOOTH_AVERAGE_BIAS = 8.0f;
    constexpr float HEIGHT_SMOOTH_DECREASE_PER = 0.3f;
    
    class Path {
    private:
        ChunkManager* chunkManager;
        PathingUtils* pathingUtils;
        
        glm::ivec2 startPosition;      // Начальная позиция (мировые координаты)
        glm::ivec2 endPosition;         // Конечная позиция (мировые координаты)
        
        int lanes;                      // Количество полос
        float radius;                   // Радиус дороги
        bool isCountryRoad;             // true = проселочная дорога, false = шоссе
        bool isRiver;                   // true = река
        bool connectsToHighway;         // Соединена ли с шоссе
        bool isValid;                   // Валиден ли путь
        
        std::vector<glm::vec2> finalPathPoints;  // Финальные точки пути (2D)
        std::vector<glm::vec3> pathPoints3d;      // Точки пути с высотами (3D)
        
        int cost;                       // Стоимость пути
        int startPointID;               // ID начальной точки
        int endPointID;                 // ID конечной точки
        
        bool validityTestOnly;          // Только проверка валидности (без построения)
        
    public:
        // Конструкторы
        Path(ChunkManager* chunkMgr, PathingUtils* pathUtils,
             const glm::ivec2& startPos, const glm::ivec2& endPos,
             int lanes, bool isCountryRoad, bool validityTest = false);
        
        Path(ChunkManager* chunkMgr, PathingUtils* pathUtils,
             const glm::ivec2& startPos, const glm::ivec2& endPos,
             float radius, bool isCountryRoad, bool validityTest = false);
        
        Path(ChunkManager* chunkMgr, PathingUtils* pathUtils,
             bool isCountryRoad = false, float radius = 8.0f, bool validityTest = false);
        
        ~Path();
        
        // Обработать путь (сглаживание углов, высот)
        void ProcessPath();
        
        // Нарисовать путь на карте дорог (roadIds - массив ID дорог)
        // roadIds должен быть размером worldSize * worldSize
        void DrawPathToRoadIds(uint8_t* roadIds, int worldSize);
        
        // Проверить, пересекается ли путь с другим путем
        bool Crosses(const Path& path) const;
        
        // Проверить, соединен ли путь с другим путем
        bool IsConnectedTo(const Path& path) const;
        
        // Проверить, соединен ли путь с шоссе
        bool IsConnectedToHighway() const;
        
        // Геттеры
        const glm::ivec2& GetStartPosition() const { return startPosition; }
        const glm::ivec2& GetEndPosition() const { return endPosition; }
        const std::vector<glm::vec2>& GetFinalPathPoints() const { return finalPathPoints; }
        const std::vector<glm::vec3>& GetPathPoints3D() const { return pathPoints3d; }
        bool IsValid() const { return isValid; }
        bool IsCountryRoad() const { return isCountryRoad; }
        bool IsRiver() const { return isRiver; }
        int GetCost() const { return cost; }
        float GetRadius() const { return radius; }
        int GetLanes() const { return lanes; }
        
        // Сеттеры
        void SetStartPosition(const glm::ivec2& pos) { startPosition = pos; }
        void SetEndPosition(const glm::ivec2& pos) { endPosition = pos; }
        void SetIsRiver(bool river) { isRiver = river; }
        void SetConnectsToHighway(bool connects) { connectsToHighway = connects; }
        
    private:
        // Сгладить углы пути
        void RoundOffCorners();
        
        // Сгладить высоты пути
        void SmoothHeights(std::vector<float>& heights);
        
        // Получить расстояние от точки до линии (в квадрате)
        float GetPointToLineDistance2(const glm::vec2& point,
                                       const glm::vec2& lineStart,
                                       const glm::vec2& lineEnd,
                                       glm::vec2& pointOnLine) const;
        
        // Вычислить расстояние в квадрате
        float DistanceSqr(const glm::vec2& a, const glm::vec2& b) const;
        float DistanceSqr(const glm::ivec2& a, const glm::ivec2& b) const;
        
        // Создать путь (вызывается из конструктора)
        void CreatePath();
    };
}

#endif /* VOXELS_PATH_H_ */

