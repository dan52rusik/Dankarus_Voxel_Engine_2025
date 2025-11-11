#include "WaterEvaporationManager.h"
#include "ChunkManager.h"
#include "MCChunk.h"
#include "WaterData.h"
#include "WaterUtils.h"
#include "WaterConstants.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

WaterEvaporationManager::WaterEvaporationManager(ChunkManager* chunkManager)
	: chunkManager(chunkManager),
	  evapWalkIndex(0), uniqueIndex(0),
	  evaporationTime(10.0f),  // 10 секунд до полного испарения
	  timeAccumulator(0.0f),
	  enabled(true) {
}

WaterEvaporationManager::~WaterEvaporationManager() {
	Clear();
}

uint64_t WaterEvaporationManager::GetCurrentTime() const {
	// Используем время в миллисекундах
	auto now = std::chrono::steady_clock::now();
	auto duration = now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void WaterEvaporationManager::Update(float deltaTime) {
	if (!enabled || evaporationMap.empty()) {
		return;
	}
	
	// Накапливаем время
	timeAccumulator += deltaTime;
	
	// Обновляем каждые 0.5 секунды
	if (timeAccumulator < 0.5f) {
		return;
	}
	
	timeAccumulator -= 0.5f;
	
	// Обновляем список для обхода
	evaporationWalkList.clear();
	for (const auto& pair : evaporationMap) {
		evaporationWalkList.push_back(pair.second);
	}
	
	// Обрабатываем только часть вокселей за раз (распределение нагрузки)
	const int MAX_EVAPORATIONS_PER_UPDATE = 15;
	int processed = 0;
	uint64_t currentTime = GetCurrentTime();
	
	// Список ключей для удаления
	std::vector<EvaporationKey> keysToRemove;
	
	for (int i = evapWalkIndex; i < static_cast<int>(evaporationWalkList.size()) && processed < MAX_EVAPORATIONS_PER_UPDATE; i++) {
		const EvaporationData& data = evaporationWalkList[i];
		
		// Проверяем, прошло ли достаточно времени
		if (currentTime > data.time + static_cast<uint64_t>(evaporationTime * 1000.0f)) {
			EvaporationKey key;
			key.chunkX = data.chunkPos.x;
			key.chunkY = data.chunkPos.y;
			key.chunkZ = data.chunkPos.z;
			key.voxelX = data.voxelPos.x;
			key.voxelY = data.voxelPos.y;
			key.voxelZ = data.voxelPos.z;
			
			keysToRemove.push_back(key);
			processed++;
		}
	}
	
	// Удаляем обработанные воксели и испаряем их
	for (const auto& key : keysToRemove) {
		// Находим чанк и испаряем воду
		std::string chunkKey = std::to_string(key.chunkX) + "," + std::to_string(key.chunkY) + "," + std::to_string(key.chunkZ);
		MCChunk* chunk = chunkManager->getChunk(chunkKey);
		if (chunk && chunk->waterData) {
			int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(
				key.voxelX, key.voxelY, key.voxelZ);
			int currentMass = chunk->waterData->getVoxelMass(index);
			if (currentMass > 0) {
				// Уменьшаем массу на небольшую величину
				int evaporationAmount = std::max(1, currentMass / 10); // 10% за раз
				int newMass = std::max(0, currentMass - evaporationAmount);
				
				chunk->waterData->setVoxelMass(index, newMass);
				chunk->dirty = true;
				chunk->waterMeshModified = true;
				
				// Если вода полностью испарилась, удаляем из списка
				if (newMass == 0) {
					evaporationMap.erase(key);
				}
			} else {
				evaporationMap.erase(key);
			}
		} else {
			evaporationMap.erase(key);
		}
		processed++;
	}
	
	// Обновляем индекс обхода
	evapWalkIndex += MAX_EVAPORATIONS_PER_UPDATE;
	if (evapWalkIndex >= static_cast<int>(evaporationWalkList.size())) {
		evapWalkIndex = 0;
	}
}

void WaterEvaporationManager::AddToEvaporationList(int chunkX, int chunkY, int chunkZ, 
                                                   int voxelX, int voxelY, int voxelZ) {
	if (!enabled) {
		return;
	}
	
	EvaporationKey key;
	key.chunkX = chunkX;
	key.chunkY = chunkY;
	key.chunkZ = chunkZ;
	key.voxelX = voxelX;
	key.voxelY = voxelY;
	key.voxelZ = voxelZ;
	
	EvaporationData data(GetCurrentTime(), uniqueIndex++, 
	                    glm::ivec3(chunkX, chunkY, chunkZ),
	                    glm::ivec3(voxelX, voxelY, voxelZ));
	
	evaporationMap[key] = data;
}

void WaterEvaporationManager::RemoveFromEvaporationList(int chunkX, int chunkY, int chunkZ,
                                                        int voxelX, int voxelY, int voxelZ) {
	EvaporationKey key;
	key.chunkX = chunkX;
	key.chunkY = chunkY;
	key.chunkZ = chunkZ;
	key.voxelX = voxelX;
	key.voxelY = voxelY;
	key.voxelZ = voxelZ;
	
	evaporationMap.erase(key);
}

void WaterEvaporationManager::Clear() {
	evaporationMap.clear();
	evaporationWalkList.clear();
	evapWalkIndex = 0;
	uniqueIndex = 0;
}

void WaterEvaporationManager::ProcessEvaporation(MCChunk* chunk, const EvaporationData& data) {
	if (!chunk || !chunk->waterData) {
		return;
	}
	
	// Уменьшаем массу воды (испарение)
	int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(
		data.voxelPos.x, data.voxelPos.y, data.voxelPos.z);
	
	int currentMass = chunk->waterData->getVoxelMass(index);
	if (currentMass > 0) {
		// Уменьшаем массу на небольшую величину
		int evaporationAmount = std::max(1, currentMass / 10); // 10% за раз
		int newMass = std::max(0, currentMass - evaporationAmount);
		
		chunk->waterData->setVoxelMass(index, newMass);
		chunk->dirty = true;
		chunk->waterMeshModified = true;
		
		// Если вода полностью испарилась, удаляем из списка
		if (newMass == 0) {
			RemoveFromEvaporationList(data.chunkPos.x, data.chunkPos.y, data.chunkPos.z,
			                          data.voxelPos.x, data.voxelPos.y, data.voxelPos.z);
		}
	}
}

