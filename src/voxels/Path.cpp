#include "Path.h"
#include "../voxels/ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Pathfinding {
    
    Path::Path(ChunkManager* chunkMgr, PathingUtils* pathUtils,
               const glm::ivec2& startPos, const glm::ivec2& endPos,
               int lanes, bool isCountryRoad, bool validityTest)
        : chunkManager(chunkMgr),
          pathingUtils(pathUtils),
          startPosition(startPos),
          endPosition(endPos),
          lanes(lanes),
          radius(static_cast<float>(lanes) * 0.5f * SINGLE_LANE_RADIUS),
          isCountryRoad(isCountryRoad),
          isRiver(false),
          connectsToHighway(false),
          isValid(false),
          cost(0),
          startPointID(-1),
          endPointID(-1),
          validityTestOnly(validityTest) {
        CreatePath();
    }
    
    Path::Path(ChunkManager* chunkMgr, PathingUtils* pathUtils,
               const glm::ivec2& startPos, const glm::ivec2& endPos,
               float radius, bool isCountryRoad, bool validityTest)
        : chunkManager(chunkMgr),
          pathingUtils(pathUtils),
          startPosition(startPos),
          endPosition(endPos),
          lanes(static_cast<int>(std::ceil(radius / SINGLE_LANE_RADIUS * 2.0f))),
          radius(radius),
          isCountryRoad(isCountryRoad),
          isRiver(false),
          connectsToHighway(false),
          isValid(false),
          cost(0),
          startPointID(-1),
          endPointID(-1),
          validityTestOnly(validityTest) {
        CreatePath();
    }
    
    Path::Path(ChunkManager* chunkMgr, PathingUtils* pathUtils,
               bool isCountryRoad, float radius, bool validityTest)
        : chunkManager(chunkMgr),
          pathingUtils(pathUtils),
          startPosition(0, 0),
          endPosition(0, 0),
          lanes(static_cast<int>(std::ceil(radius / SINGLE_LANE_RADIUS * 2.0f))),
          radius(radius),
          isCountryRoad(isCountryRoad),
          isRiver(false),
          connectsToHighway(false),
          isValid(false),
          cost(0),
          startPointID(-1),
          endPointID(-1),
          validityTestOnly(validityTest) {
        finalPathPoints.clear();
    }
    
    Path::~Path() {
        // Очистка не требуется, так как используем стандартные контейнеры
    }
    
    void Path::CreatePath() {
        if (pathingUtils == nullptr) {
            isValid = false;
            return;
        }
        
        // Получаем путь через PathingUtils
        finalPathPoints = pathingUtils->GetPath(startPosition, endPosition, isCountryRoad);
        
        if (finalPathPoints.empty()) {
            isValid = false;
            return;
        }
        
        isValid = true;
        
        // Если только проверка валидности, не обрабатываем путь
        if (validityTestOnly) {
            return;
        }
        
        // Обрабатываем путь
        ProcessPath();
    }
    
    void Path::ProcessPath() {
        if (!isValid || finalPathPoints.empty()) {
            return;
        }
        
        // Сглаживаем углы
        int smoothIterations = isCountryRoad ? 2 : 3;
        for (int i = 0; i < smoothIterations; i++) {
            RoundOffCorners();
        }
        
        // Преобразуем в 3D точки с высотами
        pathPoints3d.clear();
        float totalDistance = 0.0f;
        
        for (size_t i = 0; i < finalPathPoints.size(); i++) {
            glm::vec2 point = finalPathPoints[i];
            
            // Проверяем границы мира (предполагаем, что мир квадратный)
            // В реальной реализации нужно получить размер мира из ChunkManager
            if (point.x < 0.0f || point.y < 0.0f) {
                isValid = false;
                return;
            }
            
            // Получаем высоту
            float height = 0.0f;
            if (chunkManager != nullptr) {
                height = chunkManager->evalSurfaceHeight(point.x, point.y);
            }
            
            pathPoints3d.push_back(glm::vec3(point.x, height, point.y));
            
            if (i > 0) {
                totalDistance += glm::distance(finalPathPoints[i - 1], finalPathPoints[i]);
            }
        }
        
        cost = static_cast<int>(std::round(totalDistance));
        
        // Сглаживаем высоты
        std::vector<float> heights(pathPoints3d.size());
        if (isCountryRoad) {
            for (int i = 0; i < 4; i++) {
                SmoothHeights(heights);
            }
        } else {
            // Для шоссе более агрессивное сглаживание
            float avgHeight = 0.0f;
            for (const auto& point : pathPoints3d) {
                avgHeight += point.y;
            }
            avgHeight = avgHeight / static_cast<float>(pathPoints3d.size()) + HEIGHT_SMOOTH_AVERAGE_BIAS;
            
            // Понижаем слишком высокие точки
            for (auto& point : pathPoints3d) {
                if (point.y > avgHeight) {
                    point.y = avgHeight * 0.3f + point.y * 0.7f;
                }
            }
            
            // Многократное сглаживание
            for (int i = 0; i < 50; i++) {
                SmoothHeights(heights);
            }
        }
        
        // Обновляем финальные точки пути (округленные)
        finalPathPoints.clear();
        for (const auto& point : pathPoints3d) {
            finalPathPoints.push_back(glm::vec2(
                std::round(point.x),
                std::round(point.z)
            ));
        }
    }
    
    void Path::RoundOffCorners() {
        if (finalPathPoints.size() < 3) {
            return;
        }
        
        std::vector<glm::vec2> smoothed;
        smoothed.reserve(finalPathPoints.size());
        
        // Первая точка без изменений
        smoothed.push_back(finalPathPoints[0]);
        
        // Сглаживаем средние точки
        for (size_t i = 1; i < finalPathPoints.size() - 1; i++) {
            glm::vec2 prev = finalPathPoints[i - 1];
            glm::vec2 curr = finalPathPoints[i];
            glm::vec2 next = finalPathPoints[i + 1];
            
            // Простое сглаживание: среднее значение
            glm::vec2 smoothedPoint = (prev + curr + next) / 3.0f;
            smoothed.push_back(smoothedPoint);
        }
        
        // Последняя точка без изменений
        smoothed.push_back(finalPathPoints.back());
        
        finalPathPoints = smoothed;
    }
    
    void Path::SmoothHeights(std::vector<float>& heights) {
        if (pathPoints3d.size() < 3) {
            return;
        }
        
        heights.resize(pathPoints3d.size());
        
        // Копируем текущие высоты
        for (size_t i = 0; i < pathPoints3d.size(); i++) {
            heights[i] = pathPoints3d[i].y;
        }
        
        // Сглаживаем средние точки
        for (size_t i = 1; i < pathPoints3d.size() - 1; i++) {
            float prev = pathPoints3d[i - 1].y;
            float curr = pathPoints3d[i].y;
            float next = pathPoints3d[i + 1].y;
            
            // Взвешенное среднее
            pathPoints3d[i].y = prev * 0.25f + curr * 0.5f + next * 0.25f;
        }
    }
    
    float Path::GetPointToLineDistance2(const glm::vec2& point,
                                         const glm::vec2& lineStart,
                                         const glm::vec2& lineEnd,
                                         glm::vec2& pointOnLine) const {
        glm::vec2 dir = lineEnd - lineStart;
        float length = glm::length(dir);
        
        if (length < 0.0001f) {
            pointOnLine = glm::vec2(100000.0f, 100000.0f);
            return std::numeric_limits<float>::max();
        }
        
        dir = dir / length;
        float t = glm::dot(point - lineStart, dir);
        
        if (t < 0.0f || t > length) {
            pointOnLine = glm::vec2(100000.0f, 100000.0f);
            return std::numeric_limits<float>::max();
        }
        
        pointOnLine = lineStart + dir * t;
        glm::vec2 diff = point - pointOnLine;
        return glm::dot(diff, diff);
    }
    
    float Path::DistanceSqr(const glm::vec2& a, const glm::vec2& b) const {
        glm::vec2 diff = a - b;
        return glm::dot(diff, diff);
    }
    
    float Path::DistanceSqr(const glm::ivec2& a, const glm::ivec2& b) const {
        glm::ivec2 diff = a - b;
        return static_cast<float>(diff.x * diff.x + diff.y * diff.y);
    }
    
    void Path::DrawPathToRoadIds(uint8_t* roadIds, int worldSize) {
        if (!isValid || finalPathPoints.empty() || roadIds == nullptr) {
            return;
        }
        
        float roadRadius = radius;
        float blendRadius = isCountryRoad ? roadRadius + BLEND_DIST_COUNTRY : roadRadius + BLEND_DIST_HIGHWAY;
        float innerRadius = (lanes >= 2) ? roadRadius - 1.0f : roadRadius;
        
        float roadRadiusSqr = roadRadius * roadRadius;
        float blendRadiusSqr = blendRadius * blendRadius;
        float innerRadiusSqr = innerRadius * innerRadius;
        
        for (size_t i = 0; i < finalPathPoints.size() - 1; i++) {
            glm::vec2 p1 = finalPathPoints[i];
            glm::vec2 p2 = finalPathPoints[i + 1];
            
            // Определяем границы для обработки
            int minX = static_cast<int>(std::max(0.0f, std::min(p1.x, p2.x) - blendRadius - 1.5f));
            int maxX = static_cast<int>(std::min(static_cast<float>(worldSize - 1), std::max(p1.x, p2.x) + blendRadius + 1.5f));
            int minY = static_cast<int>(std::max(0.0f, std::min(p1.y, p2.y) - blendRadius - 1.5f));
            int maxY = static_cast<int>(std::min(static_cast<float>(worldSize - 1), std::max(p1.y, p2.y) + blendRadius + 1.5f));
            
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    glm::vec2 point(static_cast<float>(x), static_cast<float>(y));
                    glm::vec2 pointOnLine;
                    float distSqr = GetPointToLineDistance2(point, p1, p2, pointOnLine);
                    
                    float t = -1.0f;
                    if (distSqr < blendRadiusSqr) {
                        float lineLength = glm::distance(p1, p2);
                        if (lineLength > 0.0001f) {
                            float distToStart = glm::distance(p1, pointOnLine);
                            t = std::clamp(distToStart / lineLength, 0.0f, 1.0f);
                        }
                    } else {
                        // Проверяем расстояние до конечных точек
                        float distToStart = DistanceSqr(point, p1);
                        float distToEnd = DistanceSqr(point, p2);
                        
                        if (distToStart < blendRadiusSqr && distToStart <= distToEnd) {
                            t = -1.0f; // Начальная точка
                        } else if (distToEnd < blendRadiusSqr) {
                            continue; // Конечная точка (уже обработана)
                        } else {
                            continue; // Вне радиуса
                        }
                    }
                    
                    int index = x + y * worldSize;
                    
                    if (isRiver) {
                        if (distSqr <= roadRadiusSqr) {
                            roadIds[index] = PATH_WATER;
                            // TODO: Установить воду в ChunkManager
                        }
                    } else {
                        uint8_t currentId = roadIds[index];
                        
                        if (currentId != PATH_HIGHWAY && (distSqr <= roadRadiusSqr || (currentId & PATH_HIGHWAY_BLEND_MASK) == 0)) {
                            if (!isCountryRoad) {
                                // Шоссе
                                if (distSqr > roadRadiusSqr) {
                                    roadIds[index] |= PATH_HIGHWAY_BLEND_MASK;
                                } else {
                                    roadIds[index] = (distSqr <= innerRadiusSqr) ? PATH_HIGHWAY : PATH_HIGHWAY_DIRT;
                                }
                            } else {
                                // Проселочная дорога
                                if (distSqr <= roadRadiusSqr) {
                                    roadIds[index] = PATH_COUNTRY;
                                }
                            }
                            
                            // Обновляем высоту (упрощенная версия)
                            if (chunkManager != nullptr && i < pathPoints3d.size() - 1) {
                                float height = pathPoints3d[i].y;
                                if (t > 0.0f && i + 1 < pathPoints3d.size()) {
                                    height = glm::mix(pathPoints3d[i].y, pathPoints3d[i + 1].y, t);
                                }
                                // TODO: Установить высоту в ChunkManager
                            }
                        }
                    }
                }
            }
        }
    }
    
    bool Path::Crosses(const Path& path) const {
        for (const auto& point1 : finalPathPoints) {
            for (const auto& point2 : path.finalPathPoints) {
                if (DistanceSqr(point1, point2) < 100.0f) {
                    return true;
                }
            }
        }
        return false;
    }
    
    bool Path::IsConnectedTo(const Path& path) const {
        if (Crosses(path)) {
            return true;
        }
        
        glm::vec2 startVec(static_cast<float>(startPosition.x), static_cast<float>(startPosition.y));
        glm::vec2 endVec(static_cast<float>(endPosition.x), static_cast<float>(endPosition.y));
        
        for (const auto& point : path.finalPathPoints) {
            if (DistanceSqr(startVec, point) < 100.0f || DistanceSqr(endVec, point) < 100.0f) {
                return true;
            }
        }
        return false;
    }
    
    bool Path::IsConnectedToHighway() const {
        // TODO: Реализовать проверку соединения с шоссе
        // Это требует интеграции с системой отслеживания шоссе
        return connectsToHighway;
    }
}

