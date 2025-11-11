#ifndef VOXELS_WATERDATA_H_
#define VOXELS_WATERDATA_H_

#include "WaterVoxelState.h"
#include "WaterUtils.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <glm/glm.hpp>

// Структура для потока воды
struct WaterFlow {
	int voxelIndex;
	int flow;
	
	WaterFlow() : voxelIndex(0), flow(0) {}
	WaterFlow(int index, int f) : voxelIndex(index), flow(f) {}
};

// Границы грунтовых вод для колонки XZ
struct GroundWaterBounds {
	uint8_t bottom;      // Нижняя граница грунтовых вод
	uint8_t waterHeight; // Высота воды
	bool isGroundWater;  // Есть ли грунтовые воды
	
	GroundWaterBounds() : bottom(0), waterHeight(0), isGroundWater(false) {}
	GroundWaterBounds(uint8_t bot, uint8_t height) 
		: bottom(bot), waterHeight(height), isGroundWater(true) {}
};

// Данные о воде в чанке
// Адаптировано из 7 Days To Die WaterDataHandle
class WaterData {
public:
	// Размеры чанка (адаптировано под наш проект: 32x32x32)
	// В 7DTD было 16x256x16, но у нас размеры чанка 32x32x32
	static constexpr int CHUNK_SIZE_X = 32;
	static constexpr int CHUNK_SIZE_Y = 32;
	static constexpr int CHUNK_SIZE_Z = 32;
	static constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
	
	WaterData();
	~WaterData();
	
	// Инициализация и очистка
	void allocate();
	void clear();
	
	// Проверка наличия активной воды
	bool hasActiveWater() const;
	
	// Проверка наличия потоков
	bool hasFlows() const { return !flowVoxels.empty(); }
	
	// Работа с активными вокселями
	void setVoxelActive(int x, int y, int z);
	void setVoxelActive(int index);
	void setVoxelInactive(int index);
	bool isVoxelActive(int index) const;
	
	// Работа с массой воды
	void setVoxelMass(int x, int y, int z, int mass);
	void setVoxelMass(int index, int mass);
	int getVoxelMass(int index) const;
	
	// Работа с состоянием вокселя (твердые грани)
	void setVoxelSolid(int x, int y, int z, uint8_t flags);
	WaterVoxelState getVoxelState(int index) const;
	
	// Работа с потоками
	void applyFlow(int x, int y, int z, int flow);
	void applyFlow(int index, int flow);
	void enqueueFlow(int voxelIndex, int flow);
	void applyEnqueuedFlows();
	
	// Работа с активациями из других чанков
	void enqueueVoxelActive(int x, int y, int z);
	void enqueueVoxelActive(int index);
	void applyEnqueuedActivations();
	
	// Работа с пробуждением вокселей
	void enqueueVoxelWakeup(int x, int y, int z);
	void enqueueVoxelWakeup(int index);
	std::unordered_set<int>& getVoxelsToWakeup() { return voxelsToWakeup; }
	
	// Грунтовые воды
	bool isInGroundWater(int x, int y, int z) const;
	void setGroundWaterBounds(int x, int z, const GroundWaterBounds& bounds);
	GroundWaterBounds getGroundWaterBounds(int x, int z) const;
	
	// Итераторы
	std::unordered_map<int, int>::iterator flowVoxelsBegin() { return flowVoxels.begin(); }
	std::unordered_map<int, int>::iterator flowVoxelsEnd() { return flowVoxels.end(); }
	
	// Получить все активные индексы вокселей
	std::vector<int> getActiveVoxelIndices() const;
	
private:
	// Данные о воде
	std::vector<int> voxelData;           // Масса воды для каждого вокселя
	std::vector<WaterVoxelState> voxelState; // Состояние вокселей (твердые грани)
	
	// Активные воксели (битовая маска)
	std::vector<bool> activeVoxels;
	
	// Потоки воды
	std::unordered_map<int, int> flowVoxels; // voxelIndex -> flow
	
	// Очереди для межчанковых операций
	std::vector<WaterFlow> flowsFromOtherChunks;
	std::vector<int> activationsFromOtherChunks;
	std::unordered_set<int> voxelsToWakeup;
	
	// Грунтовые воды (карта по XZ)
	std::vector<GroundWaterBounds> groundWaterHeights; // Индекс: x + z * CHUNK_SIZE_X
	
	// Вспомогательные функции
	int findGroundWaterBottom(int fromIndex) const;
	int getGroundWaterIndex(int x, int z) const { return x + z * CHUNK_SIZE_X; }
};

#endif /* VOXELS_WATERDATA_H_ */

