#ifndef VOXELS_WATERSIMULATOR_H_
#define VOXELS_WATERSIMULATOR_H_

#include "WaterData.h"
#include "WaterVoxelState.h"
#include "WaterUtils.h"
#include "WaterNeighborCache.h"
#include "WaterStats.h"
#include "MCChunk.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>

// Forward declarations
class ChunkManager;

// Симулятор воды с разделением на этапы
// Адаптировано из 7 Days To Die WaterSimulationNative
class WaterSimulator {
public:
	WaterSimulator(ChunkManager* chunkManager);
	~WaterSimulator();
	
	// Обновление симуляции воды
	// Вызывается каждый кадр или с определенной частотой
	void Update(float deltaTime);
	
	// Установить частоту обновления (сколько раз в секунду)
	void SetUpdateFrequency(float frequency) { updateFrequency = frequency; }
	float GetUpdateFrequency() const { return updateFrequency; }
	
	// Включить/выключить симуляцию
	void SetEnabled(bool enabled) { this->enabled = enabled; }
	bool IsEnabled() const { return enabled; }
	
	// Получить статистику последнего обновления
	const WaterStats& GetStats() const { return currentStats; }
	
	// Включить/выключить вывод статистики
	void SetStatsLogging(bool enabled) { statsLogging = enabled; }
	bool IsStatsLogging() const { return statsLogging; }
	
private:
	ChunkManager* chunkManager;
	
	// Частота обновления (раз в секунду)
	float updateFrequency;
	float updateAccumulator;
	
	// Включена ли симуляция
	bool enabled;
	
	// Статистика симуляции
	WaterStats currentStats;
	bool statsLogging;
	
	// Кэш соседей для оптимизации
	WaterNeighborCache neighborCache;
	
	// Списки активных и измененных чанков
	std::vector<MCChunk*> activeChunks;
	std::vector<MCChunk*> modifiedChunks;
	
	// Этапы симуляции (как в 7DTD)
	void PreProcess(const std::vector<MCChunk*>& allChunks);
	void CalcFlows(const std::vector<MCChunk*>& allChunks);
	void ApplyFlows(const std::vector<MCChunk*>& allChunks);
	void PostProcess(const std::vector<MCChunk*>& allChunks);
	
	// Обработка потоков для одного чанка
	void ProcessChunkFlows(MCChunk* chunk, const std::vector<MCChunk*>& allChunks);
	
	// Вычисление потоков для одного вокселя
	int ProcessFlowBelow(MCChunk* chunk, int voxelIndex, int x, int y, int z, int mass);
	int ProcessOverfull(MCChunk* chunk, int voxelIndex, int x, int y, int z, int mass);
	int ProcessFlowSide(MCChunk* chunk, int voxelIndex, int x, int y, int z, int mass, glm::ivec2 xzOffset);
	
	// Пробуждение соседних вокселей (возвращает true, если воксель был пробужден)
	bool WakeNeighbor(MCChunk* chunk, int x, int y, int z, int yOffset);
	bool WakeNeighbor(MCChunk* chunk, int x, int y, int z, glm::ivec2 xzOffset);
	
	// Вспомогательные функции
	bool TryGetMass(MCChunk* chunk, int x, int y, int z, int& mass);
	int GetStableMassBelow(int mass, int massBelow) const;
};

#endif /* VOXELS_WATERSIMULATOR_H_ */

