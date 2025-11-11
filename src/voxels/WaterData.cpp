#include "WaterData.h"
#include "WaterUtils.h"
#include <algorithm>
#include <cstring>

WaterData::WaterData() {
	allocate();
}

WaterData::~WaterData() {
	clear();
}

void WaterData::allocate() {
	voxelData.resize(CHUNK_VOLUME, 0);
	voxelState.resize(CHUNK_VOLUME);
	activeVoxels.resize(CHUNK_VOLUME, false);
	groundWaterHeights.resize(CHUNK_SIZE_X * CHUNK_SIZE_Z);
	flowVoxels.clear();
	flowsFromOtherChunks.clear();
	activationsFromOtherChunks.clear();
	voxelsToWakeup.clear();
}

void WaterData::clear() {
	voxelData.clear();
	voxelState.clear();
	activeVoxels.clear();
	groundWaterHeights.clear();
	flowVoxels.clear();
	flowsFromOtherChunks.clear();
	activationsFromOtherChunks.clear();
	voxelsToWakeup.clear();
}

bool WaterData::hasActiveWater() const {
	for (bool active : activeVoxels) {
		if (active) return true;
	}
	return false;
}

void WaterData::setVoxelActive(int x, int y, int z) {
	int index = WaterUtils::GetVoxelIndex<CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z>(x, y, z);
	setVoxelActive(index);
}

void WaterData::setVoxelActive(int index) {
	if (index >= 0 && index < CHUNK_VOLUME) {
		activeVoxels[index] = true;
	}
}

void WaterData::setVoxelInactive(int index) {
	if (index >= 0 && index < CHUNK_VOLUME) {
		activeVoxels[index] = false;
	}
}

bool WaterData::isVoxelActive(int index) const {
	if (index >= 0 && index < CHUNK_VOLUME) {
		return activeVoxels[index];
	}
	return false;
}

void WaterData::setVoxelMass(int x, int y, int z, int mass) {
	int index = WaterUtils::GetVoxelIndex<CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z>(x, y, z);
	setVoxelMass(index, mass);
}

void WaterData::setVoxelMass(int index, int mass) {
	if (index < 0 || index >= CHUNK_VOLUME) {
		return;
	}
	
	// Ограничиваем массу
	mass = std::max(0, std::min(mass, WaterUtils::WATER_MASS_MAX));
	
	voxelData[index] = mass;
	
	// Активируем воксель, если масса выше порога
	if (mass > WaterUtils::WATER_MASS_ACTIVE) {
		activeVoxels[index] = true;
	} else {
		activeVoxels[index] = false;
	}
}

int WaterData::getVoxelMass(int index) const {
	if (index >= 0 && index < CHUNK_VOLUME) {
		return voxelData[index];
	}
	return 0;
}

void WaterData::setVoxelSolid(int x, int y, int z, uint8_t flags) {
	int index = WaterUtils::GetVoxelIndex<CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z>(x, y, z);
	if (index < 0 || index >= CHUNK_VOLUME) {
		return;
	}
	
	WaterVoxelState oldState = voxelState[index];
	WaterVoxelState newState;
	newState.setSolid(flags);
	voxelState[index] = newState;
	
	// Обновляем границы грунтовых вод, если нужно
	GroundWaterBounds bounds = getGroundWaterBounds(x, z);
	if (!bounds.isGroundWater) {
		return;
	}
	
	// Проверяем изменения в твердости граней
	if (oldState.isSolidYNeg() && !newState.isSolidYNeg() && y == bounds.bottom) {
		bounds.bottom = static_cast<uint8_t>(findGroundWaterBottom(index));
		setGroundWaterBounds(x, z, bounds);
	} else if (oldState.isSolidYPos() && !newState.isSolidYPos() && y + 1 == bounds.bottom) {
		bounds.bottom = static_cast<uint8_t>(findGroundWaterBottom(index));
		setGroundWaterBounds(x, z, bounds);
	} else if (!oldState.isSolidYNeg() && newState.isSolidYNeg() && 
	           y > bounds.bottom && y <= bounds.waterHeight) {
		bounds.bottom = static_cast<uint8_t>(y);
		setGroundWaterBounds(x, z, bounds);
	} else if (oldState.isSolidYPos() && !newState.isSolidYPos()) {
		int newBottom = y + 1;
		if (newBottom > bounds.bottom && newBottom <= bounds.waterHeight) {
			bounds.bottom = static_cast<uint8_t>(newBottom);
			setGroundWaterBounds(x, z, bounds);
		}
	}
}

WaterVoxelState WaterData::getVoxelState(int index) const {
	if (index >= 0 && index < CHUNK_VOLUME) {
		return voxelState[index];
	}
	return WaterVoxelState();
}

void WaterData::applyFlow(int x, int y, int z, int flow) {
	int index = WaterUtils::GetVoxelIndex<CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z>(x, y, z);
	applyFlow(index, flow);
}

void WaterData::applyFlow(int index, int flow) {
	if (index < 0 || index >= CHUNK_VOLUME) {
		return;
	}
	
	// Добавляем поток к существующему
	auto it = flowVoxels.find(index);
	if (it != flowVoxels.end()) {
		flow += it->second;
	}
	
	flowVoxels[index] = flow;
}

void WaterData::enqueueFlow(int voxelIndex, int flow) {
	flowsFromOtherChunks.push_back(WaterFlow(voxelIndex, flow));
}

void WaterData::applyEnqueuedFlows() {
	for (const auto& flow : flowsFromOtherChunks) {
		applyFlow(flow.voxelIndex, flow.flow);
	}
	flowsFromOtherChunks.clear();
}

void WaterData::enqueueVoxelActive(int x, int y, int z) {
	int index = WaterUtils::GetVoxelIndex<CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z>(x, y, z);
	enqueueVoxelActive(index);
}

void WaterData::enqueueVoxelActive(int index) {
	activationsFromOtherChunks.push_back(index);
}

void WaterData::applyEnqueuedActivations() {
	for (int index : activationsFromOtherChunks) {
		setVoxelActive(index);
	}
	activationsFromOtherChunks.clear();
}

void WaterData::enqueueVoxelWakeup(int x, int y, int z) {
	int index = WaterUtils::GetVoxelIndex<CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z>(x, y, z);
	enqueueVoxelWakeup(index);
}

void WaterData::enqueueVoxelWakeup(int index) {
	voxelsToWakeup.insert(index);
}

bool WaterData::isInGroundWater(int x, int y, int z) const {
	GroundWaterBounds bounds = getGroundWaterBounds(x, z);
	return bounds.isGroundWater && 
	       y >= bounds.bottom && 
	       y <= bounds.waterHeight;
}

void WaterData::setGroundWaterBounds(int x, int z, const GroundWaterBounds& bounds) {
	int index = getGroundWaterIndex(x, z);
	if (index >= 0 && index < static_cast<int>(groundWaterHeights.size())) {
		groundWaterHeights[index] = bounds;
	}
}

GroundWaterBounds WaterData::getGroundWaterBounds(int x, int z) const {
	int index = getGroundWaterIndex(x, z);
	if (index >= 0 && index < static_cast<int>(groundWaterHeights.size())) {
		return groundWaterHeights[index];
	}
	return GroundWaterBounds();
}

int WaterData::findGroundWaterBottom(int fromIndex) const {
	// Ищем вверх от fromIndex до начала колонки
	int startY = WaterUtils::GetVoxelY<CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z>(fromIndex);
	
	for (int y = startY; y >= 0; --y) {
		int index = fromIndex - (startY - y) * (CHUNK_SIZE_X * CHUNK_SIZE_Z);
		if (index < 0) break;
		
		WaterVoxelState state = getVoxelState(index);
		if (state.isSolidYNeg()) {
			return y;
		}
		if (state.isSolidYPos()) {
			int nextY = std::min(y + 1, 255);
			if (nextY <= startY) {
				return nextY;
			}
		}
	}
	
	return 0;
}

std::vector<int> WaterData::getActiveVoxelIndices() const {
	std::vector<int> indices;
	for (size_t i = 0; i < activeVoxels.size(); ++i) {
		if (activeVoxels[i]) {
			indices.push_back(static_cast<int>(i));
		}
	}
	return indices;
}

