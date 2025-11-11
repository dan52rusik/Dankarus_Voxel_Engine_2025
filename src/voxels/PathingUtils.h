#ifndef VOXELS_PATHINGUTILS_H_
#define VOXELS_PATHINGUTILS_H_

#include "PathNode.h"
#include "PathNodePool.h"
#include <glm/glm.hpp>
#include <vector>
#include <queue>
#include <functional>
#include <cstdint>

// Forward declaration
class ChunkManager;

// Утилиты для pathfinding (A* алгоритм для генерации дорог)
// Адаптировано из 7 Days To Die PathingUtils
namespace Pathfinding {
    // Константы
    constexpr int PATHING_GRID_TILE_SIZE = 10;  // Размер тайла в pathfinding сетке (в мировых единицах)
    constexpr int STEP_SIZE = 10;
    constexpr glm::ivec2 STEP_HALF(5, 5);
    
    // Максимальные высоты для дорог
    constexpr float ROAD_COUNTRY_MAX_STEP_H = 12.0f;  // Максимальная разница высот для проселочной дороги
    constexpr float ROAD_HIGHWAY_MAX_STEP_H = 11.0f;  // Максимальная разница высот для шоссе
    constexpr float HEIGHT_COST_SCALE = 10.0f;        // Множитель штрафа за высоту
    
    // Соседи для pathfinding (8-направленный)
    constexpr glm::ivec2 NORMAL_NEIGHBORS[8] = {
        glm::ivec2(0, 1),   // Вверх
        glm::ivec2(1, 1),   // Вверх-вправо
        glm::ivec2(1, 0),   // Вправо
        glm::ivec2(1, -1),  // Вниз-вправо
        glm::ivec2(0, -1),  // Вниз
        glm::ivec2(-1, -1), // Вниз-влево
        glm::ivec2(-1, 0),  // Влево
        glm::ivec2(-1, 1)   // Вверх-влево
    };
    
    // Соседи для pathfinding (4-направленный)
    constexpr glm::ivec2 NORMAL_NEIGHBORS_4WAY[4] = {
        glm::ivec2(0, 1),   // Вверх
        glm::ivec2(1, 0),   // Вправо
        glm::ivec2(0, -1),  // Вниз
        glm::ivec2(-1, 0)   // Влево
    };
    
    class PathingUtils {
    private:
        ChunkManager* chunkManager;
        int8_t* pathingGrid;        // Сетка блокировок для pathfinding
        int pathingGridSize;         // Размер сетки
        std::vector<bool> closedList; // Закрытый список для A*
        int closedListWidth;
        int closedListMinY;
        int closedListMaxY;
        PathNodePool nodePool;       // Пул узлов
        
        // Минимальная куча для A* (используем std::priority_queue)
        struct PathNodeComparator {
            bool operator()(const PathNode* a, const PathNode* b) const {
                return a->pathCost > b->pathCost; // Min heap
            }
        };
        std::priority_queue<PathNode*, std::vector<PathNode*>, PathNodeComparator> openList;
        
        mutable glm::ivec2 wPos; // Временная позиция для оптимизации (mutable для использования в const методах)
        
    public:
        PathingUtils(ChunkManager* chunkMgr);
        ~PathingUtils();
        
        // Получить путь между двумя точками
        // start, end - мировые координаты
        // isCountryRoad - true для проселочной дороги, false для шоссе
        std::vector<glm::vec2> GetPath(const glm::ivec2& start, const glm::ivec2& end, bool isCountryRoad = false);
        
        // Получить стоимость пути (без построения пути)
        int GetPathCost(const glm::ivec2& start, const glm::ivec2& end, bool isCountryRoad = false);
        
        // Найти ближайшую точку на пути
        static float FindClosestPathPoint(const std::vector<glm::vec2>& path, const glm::vec2& startPos, glm::vec2& destPoint);
        
        // Проверить, находится ли точка на пути
        static bool IsPointOnPath(const std::vector<glm::ivec2>& path, const glm::ivec2& point);
        
        // Установить блокировку в позиции
        void SetPathBlocked(int pathX, int pathY, bool isBlocked);
        void SetPathBlocked(const glm::ivec2& pos, bool isBlocked);
        
        // Проверить, заблокирована ли позиция
        bool IsPathBlocked(int pathX, int pathY) const;
        bool IsPathBlocked(const glm::ivec2& pos) const;
        
        // Проверить, заблокирована ли позиция (с учетом воды, городов и т.д.)
        bool IsBlocked(int pathX, int pathY, bool isRiver = false) const;
        bool IsBlocked(const glm::ivec2& pos, bool isRiver = false) const;
        
        // Получить высоту в позиции pathfinding сетки
        float GetHeight(int pathX, int pathY) const;
        float GetHeight(const glm::ivec2& pos) const;
        
        // Инициализировать pathfinding сетку
        void SetupPathingGrid(int worldSize);
        
        // Очистить ресурсы
        void Cleanup();
        
    private:
        // A* алгоритм поиска пути
        PathNode* FindDetailedPath(const glm::ivec2& startPos, const glm::ivec2& endPos, bool isCountryRoad, bool isRiver = false);
        
        // Преобразовать позицию pathfinding сетки в мировые координаты (центр тайла)
        glm::ivec2 PathPositionToWorldCenter(int pathX, int pathY) const;
        
        // Преобразовать позицию pathfinding сетки в мировые координаты (минимум тайла)
        glm::ivec2 PathPositionToWorldMin(int pathX, int pathY) const;
        
        // Проверить границы мира
        bool InWorldBounds(int x, int y) const;
        bool InBounds(int pathX, int pathY) const;
        
        // Проверить наличие воды
        bool IsWater(int pathX, int pathY) const;
    };
}

#endif /* VOXELS_PATHINGUTILS_H_ */

