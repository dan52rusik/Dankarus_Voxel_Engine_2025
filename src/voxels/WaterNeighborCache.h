#ifndef VOXELS_WATERNEIGHBORCACHE_H_
#define VOXELS_WATERNEIGHBORCACHE_H_

#include "MCChunk.h"
#include "WaterData.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

// Кэш для быстрого доступа к соседним чанкам и вокселям
// Адаптировано из 7 Days To Die WaterNeighborCacheNative
class WaterNeighborCache {
public:
	// Константы для направлений (XZ плоскости)
	static constexpr glm::ivec2 X_NEG = glm::ivec2(-1, 0);
	static constexpr glm::ivec2 X_POS = glm::ivec2(1, 0);
	static constexpr glm::ivec2 Z_NEG = glm::ivec2(0, -1);
	static constexpr glm::ivec2 Z_POS = glm::ivec2(0, 1);
	
	WaterNeighborCache();
	~WaterNeighborCache();
	
	// Инициализировать кэш со списком всех чанков
	void InitializeCache(const std::vector<MCChunk*>& allChunks);
	
	// Установить текущий чанк для работы
	void SetChunk(MCChunk* chunk);
	
	// Установить текущий воксель
	void SetVoxel(int x, int y, int z);
	
	// Получить соседний воксель (в XZ плоскости)
	// Возвращает true, если сосед найден
	bool TryGetNeighbor(glm::ivec2 xzOffset, MCChunk*& neighborChunk, 
	                    int& x, int& y, int& z);
	
	// Получить соседний воксель по Y (вверх/вниз)
	// Возвращает true, если сосед найден
	bool TryGetNeighborY(int yOffset, MCChunk*& neighborChunk,
	                    int& x, int& y, int& z);
	
	// Получить массу воды в соседнем вокселе
	// Возвращает true, если масса получена успешно
	bool TryGetMass(glm::ivec2 xzOffset, int& mass);
	bool TryGetMassY(int yOffset, int& mass);
	
	// Получить состояние вокселя соседа
	WaterVoxelState GetNeighborState(glm::ivec2 xzOffset);
	WaterVoxelState GetNeighborStateY(int yOffset);
	
	// Текущие координаты
	int voxelX;
	int voxelY;
	int voxelZ;
	MCChunk* centerChunk;
	
private:
	// Карта чанков для быстрого поиска
	std::unordered_map<std::string, MCChunk*> chunkMap;
	
	// Вспомогательная функция для получения ключа чанка
	std::string GetChunkKey(int cx, int cy, int cz) const;
	
	// Получить соседний чанк по смещению
	MCChunk* GetNeighborChunk(int dx, int dy, int dz);
};

#endif /* VOXELS_WATERNEIGHBORCACHE_H_ */

