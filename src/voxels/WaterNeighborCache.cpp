#include "WaterNeighborCache.h"
#include "MCChunk.h"
#include "WaterData.h"
#include "WaterUtils.h"
#include <sstream>

WaterNeighborCache::WaterNeighborCache()
	: voxelX(0), voxelY(0), voxelZ(0), centerChunk(nullptr) {
}

WaterNeighborCache::~WaterNeighborCache() {
}

void WaterNeighborCache::InitializeCache(const std::vector<MCChunk*>& allChunks) {
	chunkMap.clear();
	
	for (MCChunk* chunk : allChunks) {
		if (chunk && chunk->generated) {
			std::string key = GetChunkKey(chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z);
			chunkMap[key] = chunk;
		}
	}
}

void WaterNeighborCache::SetChunk(MCChunk* chunk) {
	centerChunk = chunk;
}

void WaterNeighborCache::SetVoxel(int x, int y, int z) {
	voxelX = x;
	voxelY = y;
	voxelZ = z;
}

bool WaterNeighborCache::TryGetNeighbor(glm::ivec2 xzOffset, MCChunk*& neighborChunk, 
                                        int& x, int& y, int& z) {
	if (!centerChunk) {
		return false;
	}
	
	x = voxelX + xzOffset.x;
	y = voxelY;
	z = voxelZ + xzOffset.y;
	
	// Проверяем, находится ли сосед в том же чанке
	if (x >= 0 && x < MCChunk::CHUNK_SIZE_X &&
	    z >= 0 && z < MCChunk::CHUNK_SIZE_Z) {
		neighborChunk = centerChunk;
		return true;
	}
	
	// Сосед в другом чанке - вычисляем координаты чанка
	int localX = x;
	int localZ = z;
	int chunkDX = 0;
	int chunkDZ = 0;
	
	if (x < 0) {
		chunkDX = -1;
		localX = MCChunk::CHUNK_SIZE_X - 1;
	} else if (x >= MCChunk::CHUNK_SIZE_X) {
		chunkDX = 1;
		localX = 0;
	}
	
	if (z < 0) {
		chunkDZ = -1;
		localZ = MCChunk::CHUNK_SIZE_Z - 1;
	} else if (z >= MCChunk::CHUNK_SIZE_Z) {
		chunkDZ = 1;
		localZ = 0;
	}
	
	// Ищем соседний чанк
	MCChunk* neighbor = GetNeighborChunk(chunkDX, 0, chunkDZ);
	if (neighbor) {
		neighborChunk = neighbor;
		x = localX;
		z = localZ;
		return true;
	}
	
	return false;
}

bool WaterNeighborCache::TryGetNeighborY(int yOffset, MCChunk*& neighborChunk,
                                        int& x, int& y, int& z) {
	if (!centerChunk) {
		return false;
	}
	
	x = voxelX;
	y = voxelY + yOffset;
	z = voxelZ;
	
	// Проверяем границы по Y
	if (y < 0 || y >= MCChunk::CHUNK_SIZE_Y) {
		return false;
	}
	
	neighborChunk = centerChunk;
	return true;
}

bool WaterNeighborCache::TryGetMass(glm::ivec2 xzOffset, int& mass) {
	MCChunk* neighborChunk = nullptr;
	int x, y, z;
	if (!TryGetNeighbor(xzOffset, neighborChunk, x, y, z)) {
		return false;
	}
	
	// Проверяем валидность чанка перед использованием
	if (!neighborChunk || !neighborChunk->generated || !neighborChunk->waterData) {
		return false;
	}
	
	int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(x, y, z);
	mass = neighborChunk->waterData->getVoxelMass(index);
	return true;
}

bool WaterNeighborCache::TryGetMassY(int yOffset, int& mass) {
	MCChunk* neighborChunk = nullptr;
	int x, y, z;
	if (!TryGetNeighborY(yOffset, neighborChunk, x, y, z)) {
		return false;
	}
	
	// Проверяем валидность чанка перед использованием
	if (!neighborChunk || !neighborChunk->generated || !neighborChunk->waterData) {
		return false;
	}
	
	int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(x, y, z);
	mass = neighborChunk->waterData->getVoxelMass(index);
	return true;
}

WaterVoxelState WaterNeighborCache::GetNeighborState(glm::ivec2 xzOffset) {
	MCChunk* neighborChunk = nullptr;
	int x, y, z;
	if (!TryGetNeighbor(xzOffset, neighborChunk, x, y, z)) {
		return WaterVoxelState();
	}
	
	// Проверяем валидность чанка перед использованием
	if (!neighborChunk || !neighborChunk->generated || !neighborChunk->waterData) {
		return WaterVoxelState();
	}
	
	int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(x, y, z);
	return neighborChunk->waterData->getVoxelState(index);
}

WaterVoxelState WaterNeighborCache::GetNeighborStateY(int yOffset) {
	MCChunk* neighborChunk = nullptr;
	int x, y, z;
	if (!TryGetNeighborY(yOffset, neighborChunk, x, y, z)) {
		return WaterVoxelState();
	}
	
	// Проверяем валидность чанка перед использованием
	if (!neighborChunk || !neighborChunk->generated || !neighborChunk->waterData) {
		return WaterVoxelState();
	}
	
	int index = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(x, y, z);
	return neighborChunk->waterData->getVoxelState(index);
}

std::string WaterNeighborCache::GetChunkKey(int cx, int cy, int cz) const {
	std::ostringstream oss;
	oss << cx << "," << cy << "," << cz;
	return oss.str();
}

MCChunk* WaterNeighborCache::GetNeighborChunk(int dx, int dy, int dz) {
	if (!centerChunk || !centerChunk->generated) {
		return nullptr;
	}
	
	int neighborCX = centerChunk->chunkPos.x + dx;
	int neighborCY = centerChunk->chunkPos.y + dy;
	int neighborCZ = centerChunk->chunkPos.z + dz;
	
	std::string key = GetChunkKey(neighborCX, neighborCY, neighborCZ);
	auto it = chunkMap.find(key);
	if (it != chunkMap.end()) {
		MCChunk* chunk = it->second;
		// Проверяем, что чанк всё ещё валиден (не был удалён)
		if (chunk && chunk->generated) {
			return chunk;
		}
		// Если чанк был удалён, удаляем его из кэша
		chunkMap.erase(it);
	}
	
	return nullptr;
}

