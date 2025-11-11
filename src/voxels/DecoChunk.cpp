#include "DecoChunk.h"
#include <algorithm>

DecoChunk::DecoChunk(int _decoChunkX, int _decoChunkZ)
	: decoChunkX(_decoChunkX), decoChunkZ(_decoChunkZ),
	  isDecorated(false), isGameObjectUpdated(false), isModelsUpdated(false),
	  isVisible(false) {
}

DecoChunk::~DecoChunk() {
	destroy();
}

int DecoChunk::toDecoChunkPos(int worldPos) {
	// Преобразуем мировые координаты в координаты чанка декораций
	// Чанк декораций = 128 блоков
	if (worldPos < 0) {
		return (worldPos - (CHUNK_SIZE - 1)) / CHUNK_SIZE;
	}
	return worldPos / CHUNK_SIZE;
}

int DecoChunk::makeKey16(int x, int z) {
	// Создаём 16-bit ключ из координат чанка
	// Используем первые 8 бит для X, последние 8 бит для Z
	return ((x & 0xFF) << 8) | (z & 0xFF);
}

int64_t DecoChunk::makeSmallChunkKey(int worldX, int worldZ) {
	// Маленький чанк = 16x16 блоков (как в WorldChunkCache)
	int chunkX = worldX / 16;
	int chunkZ = worldZ / 16;
	// Используем 32 бита для каждой координаты
	return (static_cast<int64_t>(chunkX) << 32) | (static_cast<uint64_t>(chunkZ) & 0xFFFFFFFF);
}

void DecoChunk::addDecoObject(const DecoObject& deco, bool updateImmediately) {
	int64_t smallChunkKey = makeSmallChunkKey(deco.pos.x, deco.pos.z);
	
	auto it = decosPerSmallChunks.find(smallChunkKey);
	if (it == decosPerSmallChunks.end()) {
		decosPerSmallChunks[smallChunkKey] = std::list<DecoObject>();
		it = decosPerSmallChunks.find(smallChunkKey);
	}
	
	it->second.push_back(deco);
	
	if (updateImmediately) {
		isGameObjectUpdated = false;
		isModelsUpdated = false;
	}
}

bool DecoChunk::removeDecoObject(const glm::ivec3& pos) {
	int64_t smallChunkKey = makeSmallChunkKey(pos.x, pos.z);
	
	auto it = decosPerSmallChunks.find(smallChunkKey);
	if (it == decosPerSmallChunks.end()) {
		return false;
	}
	
	auto& decoList = it->second;
	for (auto decoIt = decoList.begin(); decoIt != decoList.end(); ++decoIt) {
		if (decoIt->pos == pos) {
			decoList.erase(decoIt);
			if (decoList.empty()) {
				decosPerSmallChunks.erase(it);
			}
			return true;
		}
	}
	
	return false;
}

bool DecoChunk::removeDecoObject(const DecoObject& deco) {
	return removeDecoObject(deco.pos);
}

DecoObject* DecoChunk::getDecoObjectAt(const glm::ivec3& pos) {
	int64_t smallChunkKey = makeSmallChunkKey(pos.x, pos.z);
	
	auto it = decosPerSmallChunks.find(smallChunkKey);
	if (it == decosPerSmallChunks.end()) {
		return nullptr;
	}
	
	for (auto& deco : it->second) {
		if (deco.pos == pos) {
			return &deco;
		}
	}
	
	return nullptr;
}

void DecoChunk::setVisible(bool visible) {
	isVisible = visible;
}

void DecoChunk::destroy() {
	decosPerSmallChunks.clear();
	isDecorated = false;
	isGameObjectUpdated = false;
	isModelsUpdated = false;
	isVisible = false;
}

void DecoChunk::restoreGeneratedDecos(int64_t worldChunkKey) {
	// Восстанавливаем сгенерированные декорации для чанка
	// (для reset функциональности)
	for (auto& pair : decosPerSmallChunks) {
		for (auto& deco : pair.second) {
			if (deco.state == DecoState::GeneratedInactive) {
				deco.state = DecoState::GeneratedActive;
			}
		}
	}
	isGameObjectUpdated = false;
	isModelsUpdated = false;
}

