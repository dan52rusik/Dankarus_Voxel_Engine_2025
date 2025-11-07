#include "ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <string>

ChunkManager::ChunkManager() 
	: noise(1337), baseFreq(0.03f), octaves(4), lacunarity(2.0f), gain(0.5f), 
	  baseHeight(12.0f), heightVariation(4.0f) {
}

ChunkManager::~ChunkManager() {
	for (auto& pair : chunks) {
		delete pair.second;
	}
	chunks.clear();
}

std::string ChunkManager::chunkKey(int cx, int cy, int cz) const {
	return std::to_string(cx) + "," + std::to_string(cy) + "," + std::to_string(cz);
}

glm::ivec3 ChunkManager::worldToChunk(const glm::vec3& worldPos) const {
	return glm::ivec3(
		(int)std::floor(worldPos.x / MCChunk::CHUNK_SIZE_X),
		(int)std::floor(worldPos.y / MCChunk::CHUNK_SIZE_Y),
		(int)std::floor(worldPos.z / MCChunk::CHUNK_SIZE_Z)
	);
}

void ChunkManager::generateChunk(int cx, int cy, int cz) {
	std::string key = chunkKey(cx, cy, cz);
	
	// Проверяем, не загружен ли уже чанк
	if (chunks.find(key) != chunks.end()) {
		return;
	}
	
	MCChunk* chunk = new MCChunk(cx, cy, cz);
	chunk->generate(noise, baseFreq, octaves, lacunarity, gain, baseHeight, heightVariation);
	chunks[key] = chunk;
}

void ChunkManager::unloadDistantChunks(const glm::vec3& cameraPos, int renderDistance) {
	glm::ivec3 cameraChunk = worldToChunk(cameraPos);
	
	// Удаляем чанки, которые слишком далеко от камеры
	auto it = chunks.begin();
	while (it != chunks.end()) {
		MCChunk* chunk = it->second;
		glm::ivec3 chunkPos = chunk->chunkPos;
		
		// Вычисляем расстояние в чанках
		int dx = chunkPos.x - cameraChunk.x;
		int dy = chunkPos.y - cameraChunk.y;
		int dz = chunkPos.z - cameraChunk.z;
		int dist = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
		
		if (dist > renderDistance) {
			delete chunk;
			it = chunks.erase(it);
		} else {
			++it;
		}
	}
}

void ChunkManager::update(const glm::vec3& cameraPos, int renderDistance) {
	glm::ivec3 cameraChunk = worldToChunk(cameraPos);
	
	// Генерируем чанки вокруг камеры
	for (int x = -renderDistance; x <= renderDistance; x++) {
		for (int y = -renderDistance; y <= renderDistance; y++) {
			for (int z = -renderDistance; z <= renderDistance; z++) {
				// Генерируем только чанки в пределах радиуса
				if (std::max({std::abs(x), std::abs(y), std::abs(z)}) <= renderDistance) {
					int cx = cameraChunk.x + x;
					int cy = cameraChunk.y + y;
					int cz = cameraChunk.z + z;
					generateChunk(cx, cy, cz);
				}
			}
		}
	}
	
	// Выгружаем далекие чанки
	unloadDistantChunks(cameraPos, renderDistance);
}

std::vector<MCChunk*> ChunkManager::getVisibleChunks() const {
	std::vector<MCChunk*> visible;
	for (const auto& pair : chunks) {
		if (pair.second->generated && pair.second->mesh != nullptr) {
			visible.push_back(pair.second);
		}
	}
	return visible;
}

void ChunkManager::setNoiseParams(float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation) {
	this->baseFreq = baseFreq;
	this->octaves = octaves;
	this->lacunarity = lacunarity;
	this->gain = gain;
	this->baseHeight = baseHeight;
	this->heightVariation = heightVariation;
}

