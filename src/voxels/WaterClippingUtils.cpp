#include "WaterClippingUtils.h"
#include <algorithm>
#include <cmath>

namespace WaterClippingUtils {

bool GetCubePlaneIntersectionEdgeLoop(
	const Plane& plane,
	std::vector<glm::vec3>& intersectionPoints,
	int& count) {
	
	count = 0;
	intersectionPoints.clear();
	intersectionPoints.resize(6); // Максимум 6 точек пересечения
	
	// Вычисляем расстояния от вершин куба до плоскости
	float cubeVertDistances[8];
	for (int i = 0; i < 8; i++) {
		cubeVertDistances[i] = plane.GetDistanceToPoint(CUBE_VERTS[i]);
	}
	
	// Находим пересечения с ребрами
	glm::vec3 centerSum(0.0f);
	
	for (int i = 0; i < 24; i += 2) {
		int v0Idx = CUBE_EDGES[i];
		int v1Idx = CUBE_EDGES[i + 1];
		
		float dist0 = cubeVertDistances[v0Idx];
		float dist1 = cubeVertDistances[v1Idx];
		
		// Если знаки разные, плоскость пересекает ребро
		if ((dist0 > 0.0f) != (dist1 > 0.0f)) {
			const glm::vec3& v0 = CUBE_VERTS[v0Idx];
			const glm::vec3& v1 = CUBE_VERTS[v1Idx];
			
			// Линейная интерполяция для нахождения точки пересечения
			float t = -dist1 / (dist0 - dist1);
			glm::vec3 intersection = glm::mix(v1, v0, t);
			
			intersectionPoints[count] = intersection;
			centerSum += intersection;
			count++;
		}
	}
	
	if (count < 3) {
		return false; // Недостаточно точек для полигона
	}
	
	// Сортируем точки по углу вокруг центра для правильного порядка
	glm::vec3 center = centerSum / static_cast<float>(count);
	glm::vec3 refDir = intersectionPoints[0] - center;
	
	// Создаем пары (угол, индекс) для сортировки
	std::vector<std::pair<float, int>> angleIndexPairs;
	angleIndexPairs.reserve(count);
	angleIndexPairs.push_back({0.0f, 0});
	
	for (int i = 1; i < count; i++) {
		glm::vec3 dir = intersectionPoints[i] - center;
		float angle = std::atan2(
			glm::dot(glm::cross(refDir, dir), plane.normal),
			glm::dot(refDir, dir)
		);
		if (angle < 0.0f) {
			angle += 2.0f * 3.14159265359f;
		}
		angleIndexPairs.push_back({angle, i});
	}
	
	// Сортируем по углу
	std::sort(angleIndexPairs.begin(), angleIndexPairs.end(),
		[](const std::pair<float, int>& a, const std::pair<float, int>& b) {
			return a.first < b.first;
		});
	
	// Переупорядочиваем точки
	std::vector<glm::vec3> sortedPoints(count);
	for (int i = 0; i < count; i++) {
		sortedPoints[i] = intersectionPoints[angleIndexPairs[i].second];
	}
	
	for (int i = 0; i < count; i++) {
		intersectionPoints[i] = sortedPoints[i];
	}
	
	return true;
}

glm::vec3 NearestPointOnEdgeLoop(
	const glm::vec3& point,
	const std::vector<glm::vec3>& edgeLoop,
	int count) {
	
	if (count < 2) {
		return point;
	}
	
	float minDistSq = std::numeric_limits<float>::max();
	glm::vec3 nearestPoint = point;
	
	// Проверяем каждое ребро цикла
	for (int i = 0; i < count; i++) {
		int nextIdx = (i + 1) % count;
		const glm::vec3& p0 = edgeLoop[i];
		const glm::vec3& p1 = edgeLoop[nextIdx];
		
		// Находим ближайшую точку на отрезке
		glm::vec3 edge = p1 - p0;
		float edgeLenSq = glm::dot(edge, edge);
		
		if (edgeLenSq < 0.0001f) {
			// Вырожденное ребро
			glm::vec3 diff = point - p0;
			float distSq = glm::dot(diff, diff);
			if (distSq < minDistSq) {
				minDistSq = distSq;
				nearestPoint = p0;
			}
			continue;
		}
		
		glm::vec3 toPoint = point - p0;
		float t = glm::clamp(glm::dot(toPoint, edge) / edgeLenSq, 0.0f, 1.0f);
		glm::vec3 closestOnEdge = p0 + edge * t;
		
		glm::vec3 diff2 = point - closestOnEdge;
		float distSq = glm::dot(diff2, diff2);
		if (distSq < minDistSq) {
			minDistSq = distSq;
			nearestPoint = closestOnEdge;
		}
	}
	
	return nearestPoint;
}

} // namespace WaterClippingUtils

