#include "PathingUtils.h"
#include "../voxels/ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Pathfinding {
    
    PathingUtils::PathingUtils(ChunkManager* chunkMgr)
        : chunkManager(chunkMgr),
          pathingGrid(nullptr),
          pathingGridSize(0),
          closedListWidth(0),
          closedListMinY(0),
          closedListMaxY(0),
          nodePool(100000),
          wPos(0, 0) {
    }
    
    PathingUtils::~PathingUtils() {
        Cleanup();
    }
    
    void PathingUtils::SetupPathingGrid(int worldSize) {
        pathingGridSize = worldSize / PATHING_GRID_TILE_SIZE;
        if (pathingGrid != nullptr) {
            delete[] pathingGrid;
        }
        pathingGrid = new int8_t[pathingGridSize * pathingGridSize];
        std::fill(pathingGrid, pathingGrid + pathingGridSize * pathingGridSize, 0);
    }
    
    void PathingUtils::Cleanup() {
        if (pathingGrid != nullptr) {
            delete[] pathingGrid;
            pathingGrid = nullptr;
        }
        pathingGridSize = 0;
        closedList.clear();
        closedListWidth = 0;
        nodePool.Cleanup();
        // Очистить priority_queue
        while (!openList.empty()) {
            openList.pop();
        }
    }
    
    glm::ivec2 PathingUtils::PathPositionToWorldCenter(int pathX, int pathY) const {
        wPos.x = pathX * PATHING_GRID_TILE_SIZE + STEP_HALF.x;
        wPos.y = pathY * PATHING_GRID_TILE_SIZE + STEP_HALF.y;
        return wPos;
    }
    
    glm::ivec2 PathingUtils::PathPositionToWorldMin(int pathX, int pathY) const {
        wPos.x = pathX * PATHING_GRID_TILE_SIZE;
        wPos.y = pathY * PATHING_GRID_TILE_SIZE;
        return wPos;
    }
    
    bool PathingUtils::InWorldBounds(int x, int y) const {
        // Предполагаем, что мир квадратный и начинается с (0,0)
        // В реальной реализации нужно получить размер мира из ChunkManager
        return x >= 0 && y >= 0 && x < pathingGridSize * PATHING_GRID_TILE_SIZE && 
               y < pathingGridSize * PATHING_GRID_TILE_SIZE;
    }
    
    bool PathingUtils::InBounds(int pathX, int pathY) const {
        glm::ivec2 worldCenter = PathPositionToWorldCenter(pathX, pathY);
        return InWorldBounds(worldCenter.x, worldCenter.y);
    }
    
    float PathingUtils::GetHeight(int pathX, int pathY) const {
        if (chunkManager == nullptr) {
            return 0.0f;
        }
        glm::ivec2 worldCenter = PathPositionToWorldCenter(pathX, pathY);
        // Используем evalSurfaceHeight для получения высоты поверхности
        return chunkManager->evalSurfaceHeight(static_cast<float>(worldCenter.x), static_cast<float>(worldCenter.y));
    }
    
    float PathingUtils::GetHeight(const glm::ivec2& pos) const {
        return GetHeight(pos.x, pos.y);
    }
    
    bool PathingUtils::IsWater(int pathX, int pathY) const {
        if (chunkManager == nullptr) {
            return false;
        }
        glm::ivec2 worldMin = PathPositionToWorldMin(pathX, pathY);
        // Проверяем несколько точек в тайле
        for (int y = worldMin.y; y < worldMin.y + PATHING_GRID_TILE_SIZE; y += 2) {
            for (int x = worldMin.x; x < worldMin.x + PATHING_GRID_TILE_SIZE; x += 2) {
                float waterLevel = chunkManager->getWaterLevelAt(x, y);
                float surfaceHeight = chunkManager->evalSurfaceHeight(static_cast<float>(x), static_cast<float>(y));
                if (waterLevel > surfaceHeight) {
                    return true;
                }
            }
        }
        return false;
    }
    
    bool PathingUtils::IsPathBlocked(int pathX, int pathY) const {
        if (pathingGrid == nullptr || pathX < 0 || pathY < 0 || 
            pathX >= pathingGridSize || pathY >= pathingGridSize) {
            return true;
        }
        int index = pathX + pathY * pathingGridSize;
        return pathingGrid[index] == std::numeric_limits<int8_t>::min();
    }
    
    bool PathingUtils::IsPathBlocked(const glm::ivec2& pos) const {
        return IsPathBlocked(pos.x, pos.y);
    }
    
    bool PathingUtils::IsBlocked(int pathX, int pathY, bool isRiver) const {
        if (!InBounds(pathX, pathY)) {
            return true;
        }
        // Проверяем блокировку в pathingGrid
        if (IsPathBlocked(pathX, pathY)) {
            return true;
        }
        // Для рек разрешаем воду, для дорог - нет
        if (!isRiver && IsWater(pathX, pathY)) {
            return true;
        }
        // TODO: Добавить проверки на города, радиацию и т.д.
        return false;
    }
    
    bool PathingUtils::IsBlocked(const glm::ivec2& pos, bool isRiver) const {
        return IsBlocked(pos.x, pos.y, isRiver);
    }
    
    void PathingUtils::SetPathBlocked(int pathX, int pathY, bool isBlocked) {
        if (pathingGrid == nullptr) {
            return;
        }
        if (pathX < 0 || pathY < 0 || pathX >= pathingGridSize || pathY >= pathingGridSize) {
            return;
        }
        int index = pathX + pathY * pathingGridSize;
        pathingGrid[index] = isBlocked ? std::numeric_limits<int8_t>::min() : 0;
    }
    
    void PathingUtils::SetPathBlocked(const glm::ivec2& pos, bool isBlocked) {
        SetPathBlocked(pos.x, pos.y, isBlocked);
    }
    
    PathNode* PathingUtils::FindDetailedPath(const glm::ivec2& startPos, const glm::ivec2& endPos, bool isCountryRoad, bool isRiver) {
        if (chunkManager == nullptr) {
            return nullptr;
        }
        
        // Инициализируем закрытый список
        int gridSize = pathingGridSize + 1;
        if (closedList.empty() || closedListWidth != gridSize) {
            closedListWidth = gridSize;
            closedList.resize(gridSize * gridSize, false);
        } else {
            // Очищаем только использованную область
            int minIndex = closedListMinY * gridSize;
            int maxIndex = closedListMaxY * gridSize + gridSize;
            std::fill(closedList.begin() + minIndex, closedList.begin() + maxIndex, false);
        }
        
        // Очищаем открытый список
        while (!openList.empty()) {
            openList.pop();
        }
        
        // Очищаем пул узлов
        nodePool.ReturnAll();
        
        // Начальный узел
        PathNode* startNode = nodePool.Alloc();
        startNode->Set(startPos, 0.0f, nullptr);
        openList.push(startNode);
        
        int startIndex = startPos.x + startPos.y * closedListWidth;
        closedList[startIndex] = true;
        closedListMinY = startPos.y;
        closedListMaxY = startPos.y;
        
        // Максимальная разница высот
        float maxStepH = isCountryRoad ? ROAD_COUNTRY_MAX_STEP_H : ROAD_HIGHWAY_MAX_STEP_H;
        
        // Максимальное количество итераций
        int maxIterations = 20000;
        
        while (!openList.empty() && maxIterations-- > 0) {
            PathNode* current = openList.top();
            openList.pop();
            
            // Проверяем, достигли ли цели
            if (current->position.x == endPos.x && current->position.y == endPos.y) {
                return current;
            }
            
            // Проверяем соседей
            for (int i = 0; i < 8; i++) {
                glm::ivec2 neighborPos = current->position + NORMAL_NEIGHBORS[i];
                
                // Проверяем границы
                if (!InBounds(neighborPos.x, neighborPos.y)) {
                    continue;
                }
                
                int neighborIndex = neighborPos.x + neighborPos.y * closedListWidth;
                if (closedList[neighborIndex]) {
                    continue;
                }
                
                // Проверяем блокировку
                if (IsBlocked(neighborPos.x, neighborPos.y, isRiver)) {
                    closedList[neighborIndex] = true;
                    closedListMinY = std::min(closedListMinY, neighborPos.y);
                    closedListMaxY = std::max(closedListMaxY, neighborPos.y);
                    continue;
                }
                
                // Проверяем разницу высот
                float heightDiff = std::abs(GetHeight(current->position) - GetHeight(neighborPos));
                if (heightDiff > maxStepH) {
                    closedList[neighborIndex] = true;
                    closedListMinY = std::min(closedListMinY, neighborPos.y);
                    closedListMaxY = std::max(closedListMaxY, neighborPos.y);
                    continue;
                }
                
                // Вычисляем стоимость
                float heightCost = heightDiff * HEIGHT_COST_SCALE;
                glm::vec2 diff = glm::vec2(neighborPos) - glm::vec2(endPos);
                float distance = glm::length(diff);
                float cost = current->pathCost + distance + heightCost;
                
                // Штраф за диагональное движение
                if (NORMAL_NEIGHBORS[i].x != 0 && NORMAL_NEIGHBORS[i].y != 0) {
                    cost *= 1.2f;
                }
                
                // Штраф из pathingGrid
                if (pathingGrid != nullptr) {
                    int gridIndex = neighborPos.x + neighborPos.y * pathingGridSize;
                    if (gridIndex >= 0 && gridIndex < pathingGridSize * pathingGridSize) {
                        int8_t penalty = pathingGrid[gridIndex];
                        if (penalty > 0) {
                            cost *= static_cast<float>(penalty);
                        }
                    }
                }
                
                closedList[neighborIndex] = true;
                closedListMinY = std::min(closedListMinY, neighborPos.y);
                closedListMaxY = std::max(closedListMaxY, neighborPos.y);
                
                PathNode* neighborNode = nodePool.Alloc();
                neighborNode->Set(neighborPos, cost, current);
                openList.push(neighborNode);
            }
        }
        
        return nullptr; // Путь не найден
    }
    
    std::vector<glm::vec2> PathingUtils::GetPath(const glm::ivec2& start, const glm::ivec2& end, bool isCountryRoad) {
        std::vector<glm::vec2> result;
        
        // Преобразуем мировые координаты в pathfinding координаты
        glm::ivec2 startPath = start / PATHING_GRID_TILE_SIZE;
        glm::ivec2 endPath = end / PATHING_GRID_TILE_SIZE;
        
        PathNode* pathNode = FindDetailedPath(startPath, endPath, isCountryRoad, false);
        
        if (pathNode == nullptr) {
            return result; // Путь не найден
        }
        
        // Реконструируем путь
        std::vector<glm::ivec2> pathPoints;
        PathNode* current = pathNode;
        while (current != nullptr) {
            pathPoints.push_back(current->position);
            current = current->next;
        }
        std::reverse(pathPoints.begin(), pathPoints.end());
        
        // Преобразуем в мировые координаты
        for (const auto& point : pathPoints) {
            glm::ivec2 worldCenter = PathPositionToWorldCenter(point.x, point.y);
            result.push_back(glm::vec2(worldCenter.x, worldCenter.y));
        }
        
        nodePool.ReturnAll();
        return result;
    }
    
    int PathingUtils::GetPathCost(const glm::ivec2& start, const glm::ivec2& end, bool isCountryRoad) {
        glm::ivec2 startPath = start / PATHING_GRID_TILE_SIZE;
        glm::ivec2 endPath = end / PATHING_GRID_TILE_SIZE;
        
        PathNode* pathNode = FindDetailedPath(startPath, endPath, isCountryRoad, false);
        
        if (pathNode == nullptr) {
            return -1; // Путь не найден
        }
        
        int cost = static_cast<int>(pathNode->pathCost);
        nodePool.ReturnAll();
        return cost;
    }
    
    float PathingUtils::FindClosestPathPoint(const std::vector<glm::vec2>& path, const glm::vec2& startPos, glm::vec2& destPoint) {
        float closestDist = std::numeric_limits<float>::max();
        destPoint = glm::vec2(0.0f);
        
        for (const auto& point : path) {
            glm::vec2 diff = startPos - point;
            float dist = glm::dot(diff, diff);
            if (dist < closestDist) {
                closestDist = dist;
                destPoint = point;
            }
        }
        
        return closestDist;
    }
    
    bool PathingUtils::IsPointOnPath(const std::vector<glm::ivec2>& path, const glm::ivec2& point) {
        for (const auto& p : path) {
            if (p.x == point.x && p.y == point.y) {
                return true;
            }
        }
        return false;
    }
}

