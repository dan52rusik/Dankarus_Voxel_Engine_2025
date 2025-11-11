#ifndef VOXELS_MESHGENERATOROPTIMIZATIONS_H_
#define VOXELS_MESHGENERATOROPTIMIZATIONS_H_

#include "MCChunk.h"
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>
#include <algorithm>

// Оптимизации для генерации мешей
// Адаптировано из 7 Days To Die MeshGeneratorMC2
namespace MeshGeneratorOptimizations {
	
	// Кэш соседних чанков для генерации меша
	// Размер: 19x19 = 361 чанк (текущий + соседи в радиусе 1)
	class NeighborChunkCache {
	public:
		static constexpr int CACHE_DIM = 19; // Размер кэша (19x19)
		static constexpr int CACHE_SIZE = CACHE_DIM * CACHE_DIM; // 361
		
		NeighborChunkCache() {
			chunks.resize(CACHE_SIZE, nullptr);
		}
		
		// Очистить кэш
		void clear() {
			std::fill(chunks.begin(), chunks.end(), nullptr);
		}
		
		// Установить чанк в кэш (координаты относительно центра)
		// centerX, centerZ - координаты центрального чанка
		// offsetX, offsetZ - смещение от центра (-1..+1 для соседей)
		void setChunk(int offsetX, int offsetZ, MCChunk* chunk) {
			int index = (offsetX + 1) + (offsetZ + 1) * CACHE_DIM;
			if (index >= 0 && index < CACHE_SIZE) {
				chunks[index] = chunk;
			}
		}
		
		// Получить чанк из кэша (координаты относительно центра)
		MCChunk* getChunk(int offsetX, int offsetZ) const {
			int index = (offsetX + 1) + (offsetZ + 1) * CACHE_DIM;
			if (index >= 0 && index < CACHE_SIZE) {
				return chunks[index];
			}
			return nullptr;
		}
		
		// Получить чанк по абсолютным координатам (x, z в локальных координатах чанка)
		// x, z могут быть отрицательными или >= 16 для доступа к соседним чанкам
		MCChunk* getChunkAt(int x, int z) const {
			// Определяем, в каком чанке находится координата
			int chunkX = (x < 0) ? -1 : (x >= MCChunk::CHUNK_SIZE_X ? 1 : 0);
			int chunkZ = (z < 0) ? -1 : (z >= MCChunk::CHUNK_SIZE_Z ? 1 : 0);
			return getChunk(chunkX, chunkZ);
		}
		
	private:
		std::vector<MCChunk*> chunks;
	};
	
	// Проверка, является ли слой пустым (для оптимизации)
	// Проверяет, можно ли пропустить генерацию меша для слоя
	// Адаптировано из MeshGeneratorMC2::mc2LayerIsEmpty
	bool isLayerEmpty(MCChunk* centerChunk, const NeighborChunkCache& neighbors, 
	                  int startLayerIdx, int endLayerIdx);
	
	// Получить плотность из чанка (с поддержкой соседних чанков)
	// x, y, z - локальные координаты (могут выходить за границы чанка)
	// Возвращает плотность или 0, если координаты вне доступных чанков
	float getDensity(int x, int y, int z, MCChunk* centerChunk, 
	                 const NeighborChunkCache& neighbors);
	
	// Вычислить нормаль с улучшенной интерполяцией
	// Адаптировано из MeshGeneratorMC2::CalculateNormal
	glm::vec3 calculateNormal(int x, int y, int z, MCChunk* centerChunk,
	                          const NeighborChunkCache& neighbors);
	
} // namespace MeshGeneratorOptimizations

#endif /* VOXELS_MESHGENERATOROPTIMIZATIONS_H_ */

