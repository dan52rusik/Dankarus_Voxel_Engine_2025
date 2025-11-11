#ifndef VOXELS_WATEREVAPORATIONMANAGER_H_
#define VOXELS_WATEREVAPORATIONMANAGER_H_

#include "MCChunk.h"
#include "WaterData.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <glm/glm.hpp>

// Forward declarations
class ChunkManager;
class MCChunk;

// Менеджер испарения воды
// Адаптировано из 7 Days To Die WaterEvaporationManager
class WaterEvaporationManager {
public:
	WaterEvaporationManager(ChunkManager* chunkManager);
	~WaterEvaporationManager();
	
	// Обновление испарения (вызывается периодически)
	void Update(float deltaTime);
	
	// Добавить воксель в список испарения
	void AddToEvaporationList(int chunkX, int chunkY, int chunkZ, int voxelX, int voxelY, int voxelZ);
	
	// Удалить воксель из списка испарения
	void RemoveFromEvaporationList(int chunkX, int chunkY, int chunkZ, int voxelX, int voxelY, int voxelZ);
	
	// Очистить все списки
	void Clear();
	
	// Установить скорость испарения (в секундах до полного испарения)
	void SetEvaporationTime(float time) { evaporationTime = time; }
	float GetEvaporationTime() const { return evaporationTime; }
	
	// Включить/выключить испарение
	void SetEnabled(bool enabled) { this->enabled = enabled; }
	bool IsEnabled() const { return enabled; }
	
private:
	ChunkManager* chunkManager;
	// Структура для данных о вокселе на испарении
	struct EvaporationData {
		uint64_t time;  // Время добавления (в тиках или секундах)
		uint64_t id;    // Уникальный ID
		glm::ivec3 chunkPos;  // Позиция чанка
		glm::ivec3 voxelPos;  // Позиция вокселя в чанке
		
		EvaporationData() : time(0), id(0) {}
		EvaporationData(uint64_t t, uint64_t i, const glm::ivec3& cp, const glm::ivec3& vp)
			: time(t), id(i), chunkPos(cp), voxelPos(vp) {}
	};
	
	// Ключ для карты испарения (chunkX, chunkY, chunkZ, voxelX, voxelY, voxelZ)
	struct EvaporationKey {
		int chunkX, chunkY, chunkZ;
		int voxelX, voxelY, voxelZ;
		
		bool operator==(const EvaporationKey& other) const {
			return chunkX == other.chunkX && chunkY == other.chunkY && chunkZ == other.chunkZ &&
			       voxelX == other.voxelX && voxelY == other.voxelY && voxelZ == other.voxelZ;
		}
	};
	
	struct EvaporationKeyHash {
		size_t operator()(const EvaporationKey& key) const {
			return std::hash<int>()(key.chunkX) ^
			       (std::hash<int>()(key.chunkY) << 1) ^
			       (std::hash<int>()(key.chunkZ) << 2) ^
			       (std::hash<int>()(key.voxelX) << 3) ^
			       (std::hash<int>()(key.voxelY) << 4) ^
			       (std::hash<int>()(key.voxelZ) << 5);
		}
	};
	
	// Карта испарения
	std::unordered_map<EvaporationKey, EvaporationData, EvaporationKeyHash> evaporationMap;
	std::vector<EvaporationData> evaporationWalkList;  // Список для обхода
	
	// Индексы для обхода (для распределения нагрузки)
	int evapWalkIndex;
	uint64_t uniqueIndex;
	
	// Время испарения (в секундах)
	float evaporationTime;
	float timeAccumulator;
	
	// Включено ли испарение
	bool enabled;
	
	// Вспомогательные функции
	uint64_t GetCurrentTime() const;  // Получить текущее время (в тиках)
	void ProcessEvaporation(MCChunk* chunk, const EvaporationData& data);
};

#endif /* VOXELS_WATEREVAPORATIONMANAGER_H_ */

