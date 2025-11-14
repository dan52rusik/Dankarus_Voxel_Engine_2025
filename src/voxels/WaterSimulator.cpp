#include "WaterSimulator.h"
#include "ChunkManager.h"
#include "MCChunk.h"
#include "WaterData.h"
#include "WaterUtils.h"
#include "WaterVoxelState.h"
#include "WaterConstants.h"
#include "WaterStats.h"
#include <algorithm>
#include <cmath>
#include <iostream>

WaterSimulator::WaterSimulator(ChunkManager* chunkManager)
	: chunkManager(chunkManager),
	  updateFrequency(2.0f),  // 2 раза в секунду
	  updateAccumulator(0.0f),
	  enabled(true),
	  statsLogging(false) {
}

WaterSimulator::~WaterSimulator() {
}

void WaterSimulator::Update(float deltaTime) {
	if (!enabled) {
		return;
	}
	
	// Накапливаем время
	updateAccumulator += deltaTime;
	
	// Проверяем, нужно ли обновлять
	float updateInterval = 1.0f / updateFrequency;
	if (updateAccumulator < updateInterval) {
		return;
	}
	
	// Обновляем симуляцию
	updateAccumulator -= updateInterval;
	
	// Получаем только чанки с водой (оптимизация вместо getAllChunks())
	std::vector<MCChunk*> chunksWithWater = chunkManager->getChunksWithWater();
	
	// Для кэша соседей нужны все чанки (для межчанковых потоков)
	std::vector<MCChunk*> allChunks = chunkManager->getAllChunks();
	
	// Фильтруем только валидные чанки для кэша (защита от удалённых чанков)
	std::vector<MCChunk*> validChunks;
	validChunks.reserve(allChunks.size());
	for (MCChunk* chunk : allChunks) {
		if (chunk && chunk->generated) {
			validChunks.push_back(chunk);
		}
	}
	
	// Инициализируем кэш соседей только валидными чанками
	neighborCache.InitializeCache(validChunks);
	
	// Сбрасываем статистику
	currentStats.Reset();
	
	// Выполняем этапы симуляции (как в 7DTD)
	// Используем оптимизированный список чанков с водой вместо всех чанков
	PreProcess(chunksWithWater);
	CalcFlows(allChunks);
	ApplyFlows(allChunks);
	PostProcess(allChunks);
	
	// Выводим статистику, если включено логирование
	if (statsLogging && currentStats.NumChunksProcessed > 0) {
		std::cout << "[WATER_STATS] " << currentStats.ToString() << std::endl;
	}
}

void WaterSimulator::PreProcess(const std::vector<MCChunk*>& chunksWithWater) {
	// Очищаем списки
	activeChunks.clear();
	modifiedChunks.clear();
	
	// Собираем активные и измененные чанки из оптимизированного списка
	for (MCChunk* chunk : chunksWithWater) {
		if (!chunk || !chunk->generated || !chunk->waterData) {
			continue;
		}
		
		// Проверяем наличие активной воды (теперь учитывает любую массу > 0)
		if (chunk->waterData->hasActiveWater()) {
			activeChunks.push_back(chunk);
		} else {
			// Если воды больше нет, удаляем чанк из списка (оптимизация)
			chunkManager->removeChunkWithWater(chunk);
		}
		
		// Проверяем наличие вокселей для пробуждения
		if (!chunk->waterData->getVoxelsToWakeup().empty()) {
			modifiedChunks.push_back(chunk);
		}
	}
	
	// Обрабатываем пробуждение вокселей в измененных чанках
	for (MCChunk* chunk : modifiedChunks) {
		neighborCache.SetChunk(chunk);
		
		// Пробуждаем воксели из очереди
		std::unordered_set<int>& voxelsToWakeup = chunk->waterData->getVoxelsToWakeup();
		for (int index : voxelsToWakeup) {
			glm::ivec3 coords = WaterUtils::GetVoxelCoords<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(index);
			chunk->waterData->setVoxelActive(index);
			
			// Пробуждаем соседей
			neighborCache.SetVoxel(coords.x, coords.y, coords.z);
			if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, -1)) currentStats.NumVoxelsWokeUp++;
			if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, 1)) currentStats.NumVoxelsWokeUp++;
			if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::X_NEG)) currentStats.NumVoxelsWokeUp++;
			if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::X_POS)) currentStats.NumVoxelsWokeUp++;
			if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::Z_NEG)) currentStats.NumVoxelsWokeUp++;
			if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::Z_POS)) currentStats.NumVoxelsWokeUp++;
		}
		voxelsToWakeup.clear();
		
		// Добавляем в активные, если еще не добавлен
		if (std::find(activeChunks.begin(), activeChunks.end(), chunk) == activeChunks.end()) {
			activeChunks.push_back(chunk);
		}
	}
	
	// Добавляем соседние чанки активных чанков (для межчанковых потоков)
	// Используем getAllChunks() для поиска соседей
	std::vector<MCChunk*> allChunks = chunkManager->getAllChunks();
	std::vector<MCChunk*> neighborsToAdd;
	for (MCChunk* chunk : activeChunks) {
		// Проверяем соседей по XZ
		for (int dx = -1; dx <= 1; dx++) {
			for (int dz = -1; dz <= 1; dz++) {
				if (dx == 0 && dz == 0) continue;
				
				int neighborCX = chunk->chunkPos.x + dx;
				int neighborCZ = chunk->chunkPos.z + dz;
				
				// Ищем соседний чанк
				for (MCChunk* neighbor : allChunks) {
					if (neighbor && neighbor->chunkPos.x == neighborCX && 
					    neighbor->chunkPos.z == neighborCZ &&
					    neighbor->chunkPos.y == chunk->chunkPos.y) {
						if (std::find(activeChunks.begin(), activeChunks.end(), neighbor) == activeChunks.end() &&
						    std::find(neighborsToAdd.begin(), neighborsToAdd.end(), neighbor) == neighborsToAdd.end()) {
							neighborsToAdd.push_back(neighbor);
						}
						break;
					}
				}
			}
		}
	}
	
	// Добавляем соседние чанки
	for (MCChunk* neighbor : neighborsToAdd) {
		activeChunks.push_back(neighbor);
	}
}

void WaterSimulator::CalcFlows(const std::vector<MCChunk*>& allChunks) {
	// Ограничиваем количество обрабатываемых чанков
	const int MAX_CHUNKS_PER_UPDATE = 5;
	int processed = 0;
	
	for (MCChunk* chunk : activeChunks) {
		if (processed >= MAX_CHUNKS_PER_UPDATE) {
			break;
		}
		
		// Проверяем валидность чанка перед использованием
		if (!chunk || !chunk->generated || !chunk->waterData || !chunk->waterData->hasActiveWater()) {
			continue;
		}
		
		currentStats.NumChunksProcessed++;
		currentStats.NumChunksActive++;
		
		ProcessChunkFlows(chunk, allChunks);
		processed++;
	}
}

void WaterSimulator::ProcessChunkFlows(MCChunk* chunk, const std::vector<MCChunk*>& allChunks) {
	if (!chunk || !chunk->waterData) {
		return;
	}
	
	WaterData* waterData = chunk->waterData;
	neighborCache.SetChunk(chunk);
	
	// Получаем активные воксели
	std::vector<int> activeIndices = waterData->getActiveVoxelIndices();
	
	// Ограничиваем количество обрабатываемых вокселей
	const int MAX_VOXELS_PER_CHUNK = 50;
	int voxelCount = 0;
	
	for (int index : activeIndices) {
		if (voxelCount >= MAX_VOXELS_PER_CHUNK) {
			break;
		}
		
		currentStats.NumVoxelsProcessed++;
		
		glm::ivec3 coords = WaterUtils::GetVoxelCoords<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(index);
		int x = coords.x;
		int y = coords.y;
		int z = coords.z;
		
		int mass = waterData->getVoxelMass(index);
		if (mass < WaterConstants::MIN_MASS) {
			waterData->setVoxelInactive(index);
			currentStats.NumVoxelsPutToSleep++;
			continue;
		}
		
		neighborCache.SetVoxel(x, y, z);
		
		// Проверяем грунтовые воды (упрощенно - пропускаем для обычной воды)
		if (waterData->isInGroundWater(x, y, z)) {
			// Обработка грунтовых вод (упрощенно)
			continue;
		}
		
		// Обрабатываем потоки
		int remainingMass = mass;
		
		// 1. Поток вниз (приоритет)
		int flowDown = ProcessFlowBelow(chunk, index, x, y, z, remainingMass);
		remainingMass -= flowDown;
		
		// 2. Переполнение вверх (если масса превышает максимум)
		if (remainingMass > 0) {
			int flowUp = ProcessOverfull(chunk, index, x, y, z, remainingMass);
			remainingMass -= flowUp;
		}
		
		// 3. Потоки в стороны (если осталась масса)
		if (remainingMass > 0) {
			int flowSide = ProcessFlowSide(chunk, index, x, y, z, remainingMass, WaterNeighborCache::X_NEG);
			flowSide += ProcessFlowSide(chunk, index, x, y, z, remainingMass, WaterNeighborCache::X_POS);
			flowSide += ProcessFlowSide(chunk, index, x, y, z, remainingMass, WaterNeighborCache::Z_NEG);
			flowSide += ProcessFlowSide(chunk, index, x, y, z, remainingMass, WaterNeighborCache::Z_POS);
			remainingMass -= flowSide;
		}
		
		// Если масса стала слишком мала, деактивируем воксель
		if (remainingMass < WaterConstants::MIN_MASS && mass - remainingMass >= WaterConstants::MIN_MASS) {
			waterData->setVoxelInactive(index);
			currentStats.NumVoxelsPutToSleep++;
		}
		
		voxelCount++;
	}
}

int WaterSimulator::ProcessFlowBelow(MCChunk* chunk, int voxelIndex, int x, int y, int z, int mass) {
	if (y <= 0) {
		return 0; // Не можем течь вниз из нижнего слоя
	}
	
	WaterVoxelState state = chunk->waterData->getVoxelState(voxelIndex);
	if (state.isSolidYNeg()) {
		return 0; // Блокировано снизу
	}
	
	int yBelow = y - 1;
	WaterVoxelState stateBelow = neighborCache.GetNeighborStateY(-1);
	if (stateBelow.isSolidYPos()) {
		return 0; // Блокировано сверху в соседнем вокселе
	}
	
	// Проверяем грунтовые воды
	MCChunk* neighborChunk = nullptr;
	int nx, ny, nz;
	if (neighborCache.TryGetNeighborY(-1, neighborChunk, nx, ny, nz)) {
		if (neighborChunk && neighborChunk->waterData && 
		    neighborChunk->waterData->isInGroundWater(nx, ny, nz)) {
			// Грунтовые воды - вся масса уходит
			chunk->waterData->applyFlow(voxelIndex, -mass);
			return mass;
		}
	}
	
	// ИСПРАВЛЕНО: блокируем водопады в глубокие обрывы
	// Проверяем, нет ли сразу глубокой пропасти под нами
	int emptyBelowCount = 0;
	for (int dy = 1; dy <= 4; ++dy) {
		int yy = y - dy;
		if (yy < 0) break;
		
		int massBelowTest = 0;
		if (!TryGetMass(chunk, x, yy, z, massBelowTest)) break;
		
		if (massBelowTest == 0) {
			emptyBelowCount++;
		} else {
			break; // Нашли воду или блок - останавливаемся
		}
	}
	
	// Если под нами минимум 3 блока пустоты -> считаем это "обрывом", не льём туда воду
	if (emptyBelowCount >= 3) {
		return 0; // Блокируем водопад в пропасть
	}
	
	// Получаем массу в соседнем вокселе
	int massBelow = 0;
	if (!TryGetMass(chunk, x, yBelow, z, massBelow)) {
		return 0;
	}
	
	// Вычисляем стабильную массу ниже (улучшенная версия с переливом)
	int stableMass = WaterConstants::GetStableMassBelow(mass, massBelow);
	int flow = stableMass - massBelow;
	
	if (flow <= 0) {
		return 0;
	}
	
	// ИСПРАВЛЕНО: максимально замедляем вертикальный поток для почти статичной воды
	// Используем очень маленький множитель для вертикального потока
	const float VERTICAL_FLOW_SPEED = 0.02f; // Почти нулевой поток - вода почти статична
	flow = static_cast<int>(flow * VERTICAL_FLOW_SPEED);
	
	// Если после умножения меньше MIN_FLOW — не течём вообще
	if (flow < WaterConstants::MIN_FLOW) {
		return 0;
	}
	
	flow = std::min(flow, mass); // Не больше текущей массы
	
	// Если stableMass > MAX_MASS (перелив), ограничиваем поток разумным значением
	if (stableMass > WaterConstants::MAX_MASS) {
		int overflow = stableMass - WaterConstants::MAX_MASS;
		flow = std::min(flow, overflow + static_cast<int>((WaterConstants::MAX_MASS - massBelow) * WaterConstants::FLOW_SPEED));
	}
	
	// Применяем поток
	chunk->waterData->applyFlow(voxelIndex, -flow);
	
	// Применяем поток к соседнему вокселю
	if (neighborChunk == chunk) {
		// В том же чанке
		int neighborIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
		chunk->waterData->applyFlow(neighborIndex, flow);
	} else if (neighborChunk && neighborChunk->generated && neighborChunk->waterData) {
		// В другом чанке - добавляем в очередь (проверяем валидность)
		int neighborIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
		neighborChunk->waterData->enqueueFlow(neighborIndex, flow);
		neighborChunk->waterData->enqueueVoxelActive(neighborIndex);
		
		// Добавляем соседний чанк в список чанков с водой
		chunkManager->addChunkWithWater(neighborChunk);
	}
	
	currentStats.NumFlowEvents++;
	return flow;
}

int WaterSimulator::ProcessOverfull(MCChunk* chunk, int voxelIndex, int x, int y, int z, int mass) {
	if (mass < WaterConstants::MAX_MASS) {
		return 0; // Не переполнен
	}
	
	if (y >= MCChunk::CHUNK_SIZE_Y - 1) {
		return 0; // Не можем течь вверх из верхнего слоя
	}
	
	WaterVoxelState state = chunk->waterData->getVoxelState(voxelIndex);
	if (state.isSolidYPos()) {
		return 0; // Блокировано сверху
	}
	
	int yAbove = y + 1;
	WaterVoxelState stateAbove = neighborCache.GetNeighborStateY(1);
	if (stateAbove.isSolidYNeg()) {
		return 0; // Блокировано снизу в соседнем вокселе
	}
	
	// Получаем массу в соседнем вокселе
	int massAbove = 0;
	MCChunk* neighborChunk = nullptr;
	int nx, ny, nz;
	if (!neighborCache.TryGetNeighborY(1, neighborChunk, nx, ny, nz) ||
	    !TryGetMass(neighborChunk, nx, ny, nz, massAbove)) {
		return 0;
	}
	
	// Вычисляем переполнение
	int overflow = mass - WaterConstants::MAX_MASS;
	int maxAcceptable = WaterConstants::OVERFULL_MAX - massAbove;
	
	if (overflow <= 0 || maxAcceptable <= WaterConstants::MIN_FLOW) {
		return 0;
	}
	
	int flow = std::min(overflow, maxAcceptable);
	flow = std::max(flow, 1); // Минимум 1
	
	// Применяем поток
	chunk->waterData->applyFlow(voxelIndex, -flow);
	
	if (neighborChunk == chunk) {
		int neighborIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
		chunk->waterData->applyFlow(neighborIndex, flow);
	} else if (neighborChunk && neighborChunk->generated && neighborChunk->waterData) {
		// В другом чанке - добавляем в очередь (проверяем валидность)
		int neighborIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
		neighborChunk->waterData->enqueueFlow(neighborIndex, flow);
		neighborChunk->waterData->enqueueVoxelActive(neighborIndex);
		
		// Добавляем соседний чанк в список чанков с водой
		chunkManager->addChunkWithWater(neighborChunk);
	}
	
	currentStats.NumFlowEvents++;
	return flow;
}

int WaterSimulator::ProcessFlowSide(MCChunk* chunk, int voxelIndex, int x, int y, int z, int mass, glm::ivec2 xzOffset) {
	WaterVoxelState state = chunk->waterData->getVoxelState(voxelIndex);
	
	// Проверяем блокировку грани
	if (state.isSolidXZ(xzOffset)) {
		return 0;
	}
	
	// Получаем соседний воксель
	MCChunk* neighborChunk = nullptr;
	int nx, ny, nz;
	if (!neighborCache.TryGetNeighbor(xzOffset, neighborChunk, nx, ny, nz)) {
		return 0;
	}
	
	if (!neighborChunk || !neighborChunk->waterData) {
		return 0;
	}
	
	// Проверяем блокировку с другой стороны
	WaterVoxelState neighborState = neighborChunk->waterData->getVoxelState(
		WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz));
	
	if (neighborState.isSolidXZ(glm::ivec2(-xzOffset.x, -xzOffset.y))) {
		return 0;
	}
	
	// Проверяем поддержку снизу (для бокового потока)
	bool hasSupportBelow = true;
	bool neighborHasSupportBelow = true;
	
	if (y > 0) {
		WaterVoxelState stateBelow = neighborCache.GetNeighborStateY(-1);
		hasSupportBelow = stateBelow.isSolidYPos() || state.isSolidYNeg();
		
		neighborCache.SetVoxel(nx, ny, nz);
		WaterVoxelState neighborStateBelow = neighborCache.GetNeighborStateY(-1);
		neighborHasSupportBelow = neighborStateBelow.isSolidYPos() || neighborState.isSolidYNeg();
		neighborCache.SetVoxel(x, y, z); // Восстанавливаем
	}
	
	// Если поддержка разная, требуется минимальная масса для потока
	int minMassForFlow = WaterConstants::MIN_MASS;
	if (hasSupportBelow != neighborHasSupportBelow) {
		minMassForFlow = 0; // Может течь без минимальной массы
	} else if (mass < WaterConstants::MIN_MASS_SIDE_SPREAD) {
		return 0; // Недостаточно массы для бокового распространения
	}
	
	// Получаем массу соседа
	int neighborMass = 0;
	if (!TryGetMass(neighborChunk, nx, ny, nz, neighborMass)) {
		return 0;
	}
	
	// Вычисляем поток (50% от разницы, максимум 25% от массы)
	int massDiff = mass - neighborMass;
	if (massDiff <= 0) {
		return 0;
	}
	
	int flow = static_cast<int>(massDiff * WaterConstants::FLOW_SPEED);
	flow = std::min(flow, static_cast<int>(mass * 0.25f)); // Максимум 25% от массы
	flow = std::max(flow, minMassForFlow); // Минимум
	
	if (flow <= 0) {
		return 0;
	}
	
	// Применяем поток
	chunk->waterData->applyFlow(voxelIndex, -flow);
	
	if (neighborChunk == chunk) {
		int neighborIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
		chunk->waterData->applyFlow(neighborIndex, flow);
	} else if (neighborChunk && neighborChunk->generated && neighborChunk->waterData) {
		// В другом чанке - добавляем в очередь (проверяем валидность)
		int neighborIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
		neighborChunk->waterData->enqueueFlow(neighborIndex, flow);
		neighborChunk->waterData->enqueueVoxelActive(neighborIndex);
		
		// Добавляем соседний чанк в список чанков с водой
		chunkManager->addChunkWithWater(neighborChunk);
	}
	
	currentStats.NumFlowEvents++;
	return flow;
}

void WaterSimulator::ApplyFlows(const std::vector<MCChunk*>& allChunks) {
	// Применяем потоки ко всем чанкам
	for (MCChunk* chunk : allChunks) {
		// Проверяем валидность чанка перед использованием
		if (!chunk || !chunk->generated || !chunk->waterData) {
			continue;
		}
		
		// Применяем межчанковые потоки
		chunk->waterData->applyEnqueuedFlows();
		
		// Применяем потоки из flowVoxels
		if (chunk->waterData->hasFlows()) {
			neighborCache.SetChunk(chunk);
			
			for (auto it = chunk->waterData->flowVoxelsBegin(); 
			     it != chunk->waterData->flowVoxelsEnd(); ++it) {
				int index = it->first;
				int flow = it->second;
				
				int currentMass = chunk->waterData->getVoxelMass(index);
				int newMass = currentMass + flow;
				
				// Ограничиваем массу
				newMass = std::max(0, std::min(newMass, WaterConstants::MAX_MASS));
				
				chunk->waterData->setVoxelMass(index, newMass);
				
				// Пробуждаем соседей
				glm::ivec3 coords = WaterUtils::GetVoxelCoords<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(index);
				neighborCache.SetVoxel(coords.x, coords.y, coords.z);
				
				if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, -1)) currentStats.NumVoxelsWokeUp++;
				if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, 1)) currentStats.NumVoxelsWokeUp++;
				if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::X_NEG)) currentStats.NumVoxelsWokeUp++;
				if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::X_POS)) currentStats.NumVoxelsWokeUp++;
				if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::Z_NEG)) currentStats.NumVoxelsWokeUp++;
				if (WakeNeighbor(chunk, coords.x, coords.y, coords.z, WaterNeighborCache::Z_POS)) currentStats.NumVoxelsWokeUp++;
				
				// Помечаем чанк как измененный
				chunk->dirty = true;
				chunk->waterMeshModified = true;
				
				// Добавляем чанк в список чанков с водой (если еще не добавлен)
				if (chunk->waterData && chunk->waterData->hasActiveWater()) {
					chunkManager->addChunkWithWater(chunk);
				}
			}
			
			// Очищаем потоки
			chunk->waterData->clearFlows();
		}
	}
}

void WaterSimulator::PostProcess(const std::vector<MCChunk*>& allChunks) {
	// Применяем активации из других чанков
	for (MCChunk* chunk : allChunks) {
		// Проверяем валидность чанка перед использованием
		if (!chunk || !chunk->generated || !chunk->waterData) {
			continue;
		}
		
		chunk->waterData->applyEnqueuedActivations();
		
		// Если есть активная вода, добавляем в список активных
		if (chunk->waterData->hasActiveWater()) {
			if (std::find(activeChunks.begin(), activeChunks.end(), chunk) == activeChunks.end()) {
				activeChunks.push_back(chunk);
			}
		}
	}
}

bool WaterSimulator::WakeNeighbor(MCChunk* chunk, int x, int y, int z, int yOffset) {
	int neighborY = y + yOffset;
	if (neighborY < 0 || neighborY >= MCChunk::CHUNK_SIZE_Y) {
		return false;
	}
	
	MCChunk* neighborChunk = nullptr;
	int nx, ny, nz;
	if (neighborCache.TryGetNeighborY(yOffset, neighborChunk, nx, ny, nz)) {
		if (neighborChunk && neighborChunk->waterData) {
			if (neighborChunk == chunk) {
				int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
				neighborChunk->waterData->setVoxelActive(index);
				return true;
			} else {
				neighborChunk->waterData->enqueueVoxelActive(nx, ny, nz);
				return true;
			}
		}
	}
	return false;
}

bool WaterSimulator::WakeNeighbor(MCChunk* chunk, int x, int y, int z, glm::ivec2 xzOffset) {
	MCChunk* neighborChunk = nullptr;
	int nx, ny, nz;
	if (neighborCache.TryGetNeighbor(xzOffset, neighborChunk, nx, ny, nz)) {
		if (neighborChunk && neighborChunk->waterData) {
			if (neighborChunk == chunk) {
				int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
				neighborChunk->waterData->setVoxelActive(index);
				return true;
			} else {
				neighborChunk->waterData->enqueueVoxelActive(nx, ny, nz);
				return true;
			}
		}
	}
	return false;
}

bool WaterSimulator::TryGetMass(MCChunk* chunk, int x, int y, int z, int& mass) {
	if (!chunk || !chunk->waterData) {
		return false;
	}
	
	// Проверяем грунтовые воды
	if (chunk->waterData->isInGroundWater(x, y, z)) {
		mass = WaterConstants::MAX_MASS;
		return false; // Грунтовые воды не обрабатываются как обычные потоки
	}
	
	int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(x, y, z);
	mass = chunk->waterData->getVoxelMass(index);
	
	if (mass > WaterConstants::MIN_MASS) {
		return true;
	}
	
	// Проверяем, не заблокирован ли воксель
	WaterVoxelState state = chunk->waterData->getVoxelState(index);
	return !state.isSolid();
}

int WaterSimulator::GetStableMassBelow(int mass, int massBelow) const {
	return WaterConstants::GetStableMassBelow(mass, massBelow);
}
