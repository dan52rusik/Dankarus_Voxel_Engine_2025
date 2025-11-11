#ifndef VOXELS_WATERCLIPPINGUTILS_H_
#define VOXELS_WATERCLIPPINGUTILS_H_

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

// Утилиты для обрезки воды по плоскостям
// Адаптировано из 7 Days To Die WaterClippingUtils
namespace WaterClippingUtils {
	// Границы куба вокселя (0..1)
	static constexpr glm::vec3 CUBE_CENTER = glm::vec3(0.5f, 0.5f, 0.5f);
	static constexpr glm::vec3 CUBE_SIZE = glm::vec3(1.0f, 1.0f, 1.0f);
	
	// Вершины куба (локальные координаты 0..1)
	static const glm::vec3 CUBE_VERTS[8] = {
		glm::vec3(0.0f, 0.0f, 0.0f), // 0
		glm::vec3(0.0f, 1.0f, 0.0f), // 1
		glm::vec3(1.0f, 1.0f, 0.0f), // 2
		glm::vec3(1.0f, 0.0f, 0.0f), // 3
		glm::vec3(0.0f, 0.0f, 1.0f), // 4
		glm::vec3(0.0f, 1.0f, 1.0f), // 5
		glm::vec3(1.0f, 1.0f, 1.0f), // 6
		glm::vec3(1.0f, 0.0f, 1.0f)  // 7
	};
	
	// Ребра куба (пары индексов вершин)
	static const int CUBE_EDGES[24] = {
		0, 1,  // Ребро 0-1
		1, 2,  // Ребро 1-2
		2, 3,  // Ребро 2-3
		3, 0,  // Ребро 3-0
		4, 5,  // Ребро 4-5
		5, 6,  // Ребро 5-6
		6, 7,  // Ребро 6-7
		7, 4,  // Ребро 7-4
		0, 4,  // Ребро 0-4
		1, 5,  // Ребро 1-5
		2, 6,  // Ребро 2-6
		3, 7   // Ребро 3-7
	};
	
	// Плоскость для обрезки
	struct Plane {
		glm::vec3 normal;  // Нормаль плоскости (нормализованная)
		float distance;    // Расстояние от начала координат
		
		Plane() : normal(0.0f, 1.0f, 0.0f), distance(0.0f) {}
		Plane(const glm::vec3& n, float d) : normal(glm::normalize(n)), distance(d) {}
		
		// Расстояние от точки до плоскости (положительное = выше плоскости)
		float GetDistanceToPoint(const glm::vec3& point) const {
			return glm::dot(normal, point) - distance;
		}
		
		// Ближайшая точка на плоскости
		glm::vec3 ClosestPointOnPlane(const glm::vec3& point) const {
			float dist = GetDistanceToPoint(point);
			return point - normal * dist;
		}
	};
	
	// Проверить, находится ли точка внутри куба
	inline bool IsPointInCube(const glm::vec3& point) {
		return point.x >= 0.0f && point.x <= 1.0f &&
		       point.y >= 0.0f && point.y <= 1.0f &&
		       point.z >= 0.0f && point.z <= 1.0f;
	}
	
	// Найти пересечение плоскости с кубом (возвращает точки пересечения)
	// Возвращает true, если плоскость пересекает куб
	bool GetCubePlaneIntersectionEdgeLoop(
		const Plane& plane,
		std::vector<glm::vec3>& intersectionPoints,
		int& count);
	
	// Найти ближайшую точку на цикле ребер
	glm::vec3 NearestPointOnEdgeLoop(
		const glm::vec3& point,
		const std::vector<glm::vec3>& edgeLoop,
		int count);
}

#endif /* VOXELS_WATERCLIPPINGUTILS_H_ */

