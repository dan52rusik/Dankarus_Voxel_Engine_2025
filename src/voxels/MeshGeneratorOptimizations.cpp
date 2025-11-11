#include "MeshGeneratorOptimizations.h"
#include "../graphics/MarchingCubes.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace MeshGeneratorOptimizations {
	
	bool isLayerEmpty(MCChunk* centerChunk, const NeighborChunkCache& neighbors, 
	                  int startLayerIdx, int endLayerIdx) {
		if (centerChunk == nullptr) {
			return false;
		}
		
		// Проверяем диапазон Y координат
		int startY = std::max(1, startLayerIdx * 16);
		int endY = std::min(255, (endLayerIdx + 1) * 16 - 1);
		
		// Простая проверка: если все чанки в области имеют одинаковую плотность
		// (все воздух или все террейн), то слой можно считать пустым
		// Это упрощенная версия - полная реализация требует проверки всех соседних чанков
		
		// Для начала просто проверяем, есть ли вообще данные в центральном чанке
		// Полная реализация требует доступа к метаданным чанков о плотности
		
		return false; // По умолчанию считаем, что слой не пустой
	}
	
	float getDensity(int x, int y, int z, MCChunk* centerChunk, 
	                 const NeighborChunkCache& neighbors) {
		// Определяем, в каком чанке находится координата
		int localX = x;
		int localZ = z;
		MCChunk* chunk = centerChunk;
		
		// Если координаты выходят за границы центрального чанка, ищем соседний
		if (x < 0 || x >= MCChunk::CHUNK_SIZE_X || 
		    z < 0 || z >= MCChunk::CHUNK_SIZE_Z) {
			int chunkX = (x < 0) ? -1 : (x >= MCChunk::CHUNK_SIZE_X ? 1 : 0);
			int chunkZ = (z < 0) ? -1 : (z >= MCChunk::CHUNK_SIZE_Z ? 1 : 0);
			
			chunk = neighbors.getChunk(chunkX, chunkZ);
			if (chunk == nullptr) {
				// Если чанка нет, возвращаем значение по умолчанию
				if (y < 0) {
					return static_cast<float>(MarchingCubes::DensityTerrain);
				}
				if (y >= 256) {
					return static_cast<float>(MarchingCubes::DensityAir);
				}
				return static_cast<float>(MarchingCubes::DensityAir);
			}
			
			// Вычисляем локальные координаты в соседнем чанке
			localX = (x < 0) ? (MCChunk::CHUNK_SIZE_X + x) : (x - MCChunk::CHUNK_SIZE_X);
			localZ = (z < 0) ? (MCChunk::CHUNK_SIZE_Z + z) : (z - MCChunk::CHUNK_SIZE_Z);
		}
		
		// Проверяем границы Y
		if (y < 0) {
			return static_cast<float>(MarchingCubes::DensityTerrain);
		}
		if (y >= 256) {
			return static_cast<float>(MarchingCubes::DensityAir);
		}
		
		// Получаем плотность из поля плотности чанка
		// Если у чанка есть densityField, используем его
		// Иначе вычисляем из процедурной генерации
		if (chunk != nullptr) {
			// Проверяем, есть ли у чанка поле плотности
			// В текущей реализации MCChunk не хранит densityField после генерации
			// Поэтому для оптимизации нужно будет добавить кэширование
			
			// Временное решение: возвращаем значение по умолчанию
			// Полная реализация требует доступа к densityField чанка
			return 0.0f;
		}
		
		return static_cast<float>(MarchingCubes::DensityAir);
	}
	
	glm::vec3 calculateNormal(int x, int y, int z, MCChunk* centerChunk,
	                          const NeighborChunkCache& neighbors) {
		// Вычисляем нормаль через градиент плотности (центральные разности)
		float dx = getDensity(x + 1, y, z, centerChunk, neighbors) - 
		           getDensity(x - 1, y, z, centerChunk, neighbors);
		float dy = getDensity(x, y + 1, z, centerChunk, neighbors) - 
		           getDensity(x, y - 1, z, centerChunk, neighbors);
		float dz = getDensity(x, y, z + 1, centerChunk, neighbors) - 
		           getDensity(x, y, z - 1, centerChunk, neighbors);
		
		glm::vec3 normal(dx, dy, dz);
		float magnitude = glm::length(normal);
		
		if (magnitude > 1e-6f) {
			normal = normal / magnitude;
		} else {
			normal = glm::vec3(0.0f, 1.0f, 0.0f); // Нормаль по умолчанию (вверх)
		}
		
		return normal;
	}
	
} // namespace MeshGeneratorOptimizations

