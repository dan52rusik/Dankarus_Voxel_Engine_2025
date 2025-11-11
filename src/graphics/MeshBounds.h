#ifndef GRAPHICS_MESHBOUNDS_H_
#define GRAPHICS_MESHBOUNDS_H_

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

// Утилиты для вычисления границ (Bounds) мешей
// Адаптировано из 7 Days To Die DynamicMeshFile::GetBoundsFromVerts
namespace MeshBounds {
	
	// Структура для хранения границ меша (аналог Unity Bounds)
	struct Bounds {
		glm::vec3 center;
		glm::vec3 size;
		glm::vec3 min;
		glm::vec3 max;
		
		Bounds() : center(0.0f), size(0.0f), min(0.0f), max(0.0f) {}
		
		Bounds(const glm::vec3& minBounds, const glm::vec3& maxBounds) 
			: min(minBounds), max(maxBounds) {
			size = max - min;
			center = min + size * 0.5f;
		}
		
		// Установить границы из min/max
		void setMinMax(const glm::vec3& minBounds, const glm::vec3& maxBounds) {
			min = minBounds;
			max = maxBounds;
			size = max - min;
			center = min + size * 0.5f;
		}
		
		// Проверить, содержит ли границы точку
		bool contains(const glm::vec3& point) const {
			return point.x >= min.x && point.x <= max.x &&
			       point.y >= min.y && point.y <= max.y &&
			       point.z >= min.z && point.z <= max.z;
		}
		
		// Получить объем границ
		float getVolume() const {
			return size.x * size.y * size.z;
		}
	};
	
	// Вычислить границы из массива вершин (Vector3[])
	template<typename Container>
	Bounds getBoundsFromVerts(const Container& vertices) {
		if (vertices.empty()) {
			return Bounds();
		}
		
		float minX = vertices[0].x;
		float maxX = vertices[0].x;
		float minY = vertices[0].y;
		float maxY = vertices[0].y;
		float minZ = vertices[0].z;
		float maxZ = vertices[0].z;
		
		for (size_t i = 0; i < vertices.size(); i++) {
			const glm::vec3& v = vertices[i];
			minX = std::min(minX, v.x);
			maxX = std::max(maxX, v.x);
			minY = std::min(minY, v.y);
			maxY = std::max(maxY, v.y);
			minZ = std::min(minZ, v.z);
			maxZ = std::max(maxZ, v.z);
		}
		
		return Bounds(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));
	}
	
	// Вычислить границы только по Y (для оптимизации)
	template<typename Container>
	void getBoundsFromVertsJustY(const Container& vertices, Bounds& bounds) {
		if (vertices.empty()) {
			bounds.setMinMax(glm::vec3(0.0f), glm::vec3(0.0f));
			return;
		}
		
		float minY = vertices[0].y;
		float maxY = vertices[0].y;
		
		for (size_t i = 0; i < vertices.size(); i++) {
			const glm::vec3& v = vertices[i];
			minY = std::min(minY, v.y);
			maxY = std::max(maxY, v.y);
		}
		
		bounds.setMinMax(glm::vec3(0.0f, minY, 0.0f), glm::vec3(0.0f, maxY, 0.0f));
	}
	
	// Вычислить границы из массива вершин с предварительно выделенным списком bounds
	template<typename Container>
	Bounds getBoundsFromVerts(const Container& vertices, std::vector<Bounds>& boundsList, size_t index) {
		if (index >= boundsList.size()) {
			boundsList.resize(index + 1);
		}
		
		Bounds& bounds = boundsList[index];
		
		if (vertices.empty()) {
			bounds.setMinMax(glm::vec3(0.0f), glm::vec3(0.0f));
			return bounds;
		}
		
		float minX = vertices[0].x;
		float maxX = vertices[0].x;
		float minY = vertices[0].y;
		float maxY = vertices[0].y;
		float minZ = vertices[0].z;
		float maxZ = vertices[0].z;
		
		for (size_t i = 0; i < vertices.size(); i++) {
			const glm::vec3& v = vertices[i];
			minX = std::min(minX, v.x);
			maxX = std::max(maxX, v.x);
			minY = std::min(minY, v.y);
			maxY = std::max(maxY, v.y);
			minZ = std::min(minZ, v.z);
			maxZ = std::max(maxZ, v.z);
		}
		
		bounds.setMinMax(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ));
		return bounds;
	}
	
} // namespace MeshBounds

#endif /* GRAPHICS_MESHBOUNDS_H_ */

