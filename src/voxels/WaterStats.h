#ifndef VOXELS_WATERSTATS_H_
#define VOXELS_WATERSTATS_H_

#include <string>
#include <cstdio>

// Статистика симуляции воды
// Адаптировано из 7 Days To Die WaterStats
struct WaterStats {
	int NumChunksProcessed;      // Количество обработанных чанков
	int NumChunksActive;         // Количество активных чанков
	int NumFlowEvents;           // Количество событий потока
	int NumVoxelsProcessed;      // Количество обработанных вокселей
	int NumVoxelsPutToSleep;     // Количество вокселей, переведенных в спящий режим
	int NumVoxelsWokeUp;         // Количество пробужденных вокселей
	
	WaterStats() 
		: NumChunksProcessed(0), NumChunksActive(0), NumFlowEvents(0),
		  NumVoxelsProcessed(0), NumVoxelsPutToSleep(0), NumVoxelsWokeUp(0) {}
	
	// Оператор сложения для агрегации статистики
	WaterStats operator+(const WaterStats& other) const {
		WaterStats result;
		result.NumChunksProcessed = NumChunksProcessed + other.NumChunksProcessed;
		result.NumChunksActive = NumChunksActive + other.NumChunksActive;
		result.NumFlowEvents = NumFlowEvents + other.NumFlowEvents;
		result.NumVoxelsProcessed = NumVoxelsProcessed + other.NumVoxelsProcessed;
		result.NumVoxelsPutToSleep = NumVoxelsPutToSleep + other.NumVoxelsPutToSleep;
		result.NumVoxelsWokeUp = NumVoxelsWokeUp + other.NumVoxelsWokeUp;
		return result;
	}
	
	// Сброс статистики
	void Reset() {
		NumChunksProcessed = 0;
		NumChunksActive = 0;
		NumFlowEvents = 0;
		NumVoxelsProcessed = 0;
		NumVoxelsPutToSleep = 0;
		NumVoxelsWokeUp = 0;
	}
	
	// Получить строковое представление для отладки
	std::string ToString() const {
		char buffer[256];
		snprintf(buffer, sizeof(buffer),
			"Chunks: %d/%d active, Voxels: %d processed, %d slept, %d woke, Flows: %d",
			NumChunksActive, NumChunksProcessed,
			NumVoxelsProcessed, NumVoxelsPutToSleep, NumVoxelsWokeUp,
			NumFlowEvents);
		return std::string(buffer);
	}
};

#endif /* VOXELS_WATERSTATS_H_ */

