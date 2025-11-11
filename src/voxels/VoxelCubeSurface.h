#ifndef VOXELS_VOXELCUBESURFACE_H_
#define VOXELS_VOXELCUBESURFACE_H_

#include <glm/glm.hpp>
#include <vector>
#include <functional>

// Генерация позиций на поверхности куба
// Адаптировано из 7 Days To Die GenerateVoxelCubeSurface
namespace VoxelCubeSurface {
	
	// Генерирует все позиции на поверхности куба вокруг origin с заданным radius
	// radius = 0 возвращает только origin
	// radius = 1 генерирует куб 3x3x3 (только поверхность)
	// radius = 2 генерирует куб 5x5x5 (только поверхность)
	// callback вызывается для каждой позиции: callback(glm::ivec3 position)
	template<typename Callback>
	void generateCubeSurfacePositions(const glm::ivec3& origin, int radius, Callback callback) {
		if (radius <= 0) {
			callback(origin);
			return;
		}
		
		// Генерируем все 6 граней куба
		generateCubeSurfaceTop(origin, radius, callback);
		generateCubeSurfaceBottom(origin, radius, callback);
		generateCubeSurfaceLeft(origin, radius, callback);
		generateCubeSurfaceRight(origin, radius, callback);
		generateCubeSurfaceFront(origin, radius, callback);
		generateCubeSurfaceBack(origin, radius, callback);
	}
	
	// Генерирует позиции и возвращает их в векторе
	std::vector<glm::ivec3> generateCubeSurfacePositions(const glm::ivec3& origin, int radius);
	
	// Генерирует позиции на поверхности куба с фильтром по Y координате
	// minY и maxY - границы для фильтрации (например, 0-255 для воксельного мира)
	template<typename Callback>
	void generateCubeSurfacePositions(const glm::ivec3& origin, int radius, int minY, int maxY, Callback callback) {
		if (radius <= 0) {
			if (origin.y >= minY && origin.y <= maxY) {
				callback(origin);
			}
			return;
		}
		
		generateCubeSurfaceTop(origin, radius, minY, maxY, callback);
		generateCubeSurfaceBottom(origin, radius, minY, maxY, callback);
		generateCubeSurfaceLeft(origin, radius, minY, maxY, callback);
		generateCubeSurfaceRight(origin, radius, minY, maxY, callback);
		generateCubeSurfaceFront(origin, radius, minY, maxY, callback);
		generateCubeSurfaceBack(origin, radius, minY, maxY, callback);
	}
	
	// Вспомогательные функции для генерации граней (внутренние, не экспортируются)
	
	// Верхняя грань (Y = origin.y + radius)
	template<typename Callback>
	inline void generateCubeSurfaceTop(const glm::ivec3& origin, int radius, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius;
		min.y += radius;
		min.z -= radius;
		int xLength = radius * 2 + 1;
		int zLength = radius * 2 + 1;
		generateXZ(min, xLength, zLength, callback);
	}
	
	// Нижняя грань (Y = origin.y - radius)
	template<typename Callback>
	inline void generateCubeSurfaceBottom(const glm::ivec3& origin, int radius, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius;
		min.y -= radius;
		min.z -= radius;
		int xLength = radius * 2 + 1;
		int zLength = radius * 2 + 1;
		generateXZ(min, xLength, zLength, callback);
	}
	
	// Левая грань (X = origin.x - radius)
	template<typename Callback>
	inline void generateCubeSurfaceLeft(const glm::ivec3& origin, int radius, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius;
		min.y -= radius - 1;
		min.z -= radius;
		int yLength = radius * 2 - 1;
		int zLength = radius * 2 + 1;
		generateYZ(min, yLength, zLength, callback);
	}
	
	// Правая грань (X = origin.x + radius)
	template<typename Callback>
	inline void generateCubeSurfaceRight(const glm::ivec3& origin, int radius, Callback callback) {
		glm::ivec3 min = origin;
		min.x += radius;
		min.y -= radius - 1;
		min.z -= radius;
		int yLength = radius * 2 - 1;
		int zLength = radius * 2 + 1;
		generateYZ(min, yLength, zLength, callback);
	}
	
	// Передняя грань (Z = origin.z + radius)
	template<typename Callback>
	inline void generateCubeSurfaceFront(const glm::ivec3& origin, int radius, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius - 1;
		min.y -= radius - 1;
		min.z += radius;
		int xLength = radius * 2 - 1;
		int yLength = radius * 2 - 1;
		generateXY(min, xLength, yLength, callback);
	}
	
	// Задняя грань (Z = origin.z - radius)
	template<typename Callback>
	inline void generateCubeSurfaceBack(const glm::ivec3& origin, int radius, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius - 1;
		min.y -= radius - 1;
		min.z -= radius;
		int xLength = radius * 2 - 1;
		int yLength = radius * 2 - 1;
		generateXY(min, xLength, yLength, callback);
	}
	
	// Вспомогательные функции для генерации плоскостей
	
	// Генерирует позиции в плоскости XZ (для top/bottom граней)
	template<typename Callback>
	inline void generateXZ(const glm::ivec3& min, int xLength, int zLength, Callback callback) {
		int xEnd = min.x + xLength;
		int zEnd = min.z + zLength;
		for (int x = min.x; x < xEnd; ++x) {
			for (int z = min.z; z < zEnd; ++z) {
				callback(glm::ivec3(x, min.y, z));
			}
		}
	}
	
	// Генерирует позиции в плоскости XZ с фильтром по Y
	template<typename Callback>
	inline void generateXZ(const glm::ivec3& min, int xLength, int zLength, int minY, int maxY, Callback callback) {
		if (min.y >= minY && min.y <= maxY) {
			generateXZ(min, xLength, zLength, callback);
		}
	}
	
	// Генерирует позиции в плоскости YZ (для left/right граней)
	template<typename Callback>
	inline void generateYZ(const glm::ivec3& min, int yLength, int zLength, Callback callback) {
		int yEnd = min.y + yLength;
		int zEnd = min.z + zLength;
		for (int y = min.y; y < yEnd; ++y) {
			for (int z = min.z; z < zEnd; ++z) {
				callback(glm::ivec3(min.x, y, z));
			}
		}
	}
	
	// Генерирует позиции в плоскости YZ с фильтром по Y
	template<typename Callback>
	inline void generateYZ(const glm::ivec3& min, int yLength, int zLength, int minY, int maxY, Callback callback) {
		int yEnd = min.y + yLength;
		int zEnd = min.z + zLength;
		for (int y = min.y; y < yEnd; ++y) {
			if (y >= minY && y < maxY) {
				for (int z = min.z; z < zEnd; ++z) {
					callback(glm::ivec3(min.x, y, z));
				}
			}
		}
	}
	
	// Генерирует позиции в плоскости XY (для front/back граней)
	template<typename Callback>
	inline void generateXY(const glm::ivec3& min, int xLength, int yLength, Callback callback) {
		int xEnd = min.x + xLength;
		int yEnd = min.y + yLength;
		for (int y = min.y; y < yEnd; ++y) {
			for (int x = min.x; x < xEnd; ++x) {
				callback(glm::ivec3(x, y, min.z));
			}
		}
	}
	
	// Генерирует позиции в плоскости XY с фильтром по Y
	template<typename Callback>
	inline void generateXY(const glm::ivec3& min, int xLength, int yLength, int minY, int maxY, Callback callback) {
		int xEnd = min.x + xLength;
		int yEnd = min.y + yLength;
		for (int y = min.y; y < yEnd; ++y) {
			if (y >= minY && y < maxY) {
				for (int x = min.x; x < xEnd; ++x) {
					callback(glm::ivec3(x, y, min.z));
				}
			}
		}
	}
	
	// Перегрузки с фильтром по Y для граней
	template<typename Callback>
	inline void generateCubeSurfaceTop(const glm::ivec3& origin, int radius, int minY, int maxY, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius;
		min.y += radius;
		min.z -= radius;
		int xLength = radius * 2 + 1;
		int zLength = radius * 2 + 1;
		generateXZ(min, xLength, zLength, minY, maxY, callback);
	}
	
	template<typename Callback>
	inline void generateCubeSurfaceBottom(const glm::ivec3& origin, int radius, int minY, int maxY, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius;
		min.y -= radius;
		min.z -= radius;
		int xLength = radius * 2 + 1;
		int zLength = radius * 2 + 1;
		generateXZ(min, xLength, zLength, minY, maxY, callback);
	}
	
	template<typename Callback>
	inline void generateCubeSurfaceLeft(const glm::ivec3& origin, int radius, int minY, int maxY, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius;
		min.y -= radius - 1;
		min.z -= radius;
		int yLength = radius * 2 - 1;
		int zLength = radius * 2 + 1;
		generateYZ(min, yLength, zLength, minY, maxY, callback);
	}
	
	template<typename Callback>
	inline void generateCubeSurfaceRight(const glm::ivec3& origin, int radius, int minY, int maxY, Callback callback) {
		glm::ivec3 min = origin;
		min.x += radius;
		min.y -= radius - 1;
		min.z -= radius;
		int yLength = radius * 2 - 1;
		int zLength = radius * 2 + 1;
		generateYZ(min, yLength, zLength, minY, maxY, callback);
	}
	
	template<typename Callback>
	inline void generateCubeSurfaceFront(const glm::ivec3& origin, int radius, int minY, int maxY, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius - 1;
		min.y -= radius - 1;
		min.z += radius;
		int xLength = radius * 2 - 1;
		int yLength = radius * 2 - 1;
		generateXY(min, xLength, yLength, minY, maxY, callback);
	}
	
	template<typename Callback>
	inline void generateCubeSurfaceBack(const glm::ivec3& origin, int radius, int minY, int maxY, Callback callback) {
		glm::ivec3 min = origin;
		min.x -= radius - 1;
		min.y -= radius - 1;
		min.z -= radius;
		int xLength = radius * 2 - 1;
		int yLength = radius * 2 - 1;
		generateXY(min, xLength, yLength, minY, maxY, callback);
	}
}

#endif /* VOXELS_VOXELCUBESURFACE_H_ */

