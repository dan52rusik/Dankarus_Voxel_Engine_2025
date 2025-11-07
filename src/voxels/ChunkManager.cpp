#include "ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
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
	// Для отрицательных координат нужно правильно вычислять чанк
	// floor() уже правильно работает для отрицательных чисел
	// Например: floor(-1/32) = floor(-0.03125) = -1
	//           floor(-33/32) = floor(-1.03125) = -2
	int cx = (int)std::floor(worldPos.x / (float)MCChunk::CHUNK_SIZE_X);
	int cy = (int)std::floor(worldPos.y / (float)MCChunk::CHUNK_SIZE_Y);
	int cz = (int)std::floor(worldPos.z / (float)MCChunk::CHUNK_SIZE_Z);
	
	return glm::ivec3(cx, cy, cz);
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

std::vector<MCChunk*> ChunkManager::getAllChunks() const {
	std::vector<MCChunk*> all;
	for (const auto& pair : chunks) {
		if (pair.second->generated) {
			all.push_back(pair.second);
		}
	}
	return all;
}

void ChunkManager::setNoiseParams(float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation) {
	this->baseFreq = baseFreq;
	this->octaves = octaves;
	this->lacunarity = lacunarity;
	this->gain = gain;
	this->baseHeight = baseHeight;
	this->heightVariation = heightVariation;
}

void ChunkManager::getNoiseParams(float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation) const {
	baseFreq = this->baseFreq;
	octaves = this->octaves;
	lacunarity = this->lacunarity;
	gain = this->gain;
	baseHeight = this->baseHeight;
	heightVariation = this->heightVariation;
}

voxel* ChunkManager::getVoxel(int x, int y, int z) {
	glm::ivec3 chunkPos = worldToChunk(glm::vec3(x, y, z));
	std::string key = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z);
	
	auto it = chunks.find(key);
	if (it == chunks.end()) {
		return nullptr;
	}
	
	MCChunk* chunk = it->second;
	int lx = x - chunkPos.x * MCChunk::CHUNK_SIZE_X;
	int ly = y - chunkPos.y * MCChunk::CHUNK_SIZE_Y;
	int lz = z - chunkPos.z * MCChunk::CHUNK_SIZE_Z;
	
	// Корректировка для отрицательных координат
	if (lx < 0) {
		lx += MCChunk::CHUNK_SIZE_X;
	}
	if (ly < 0) {
		ly += MCChunk::CHUNK_SIZE_Y;
	}
	if (lz < 0) {
		lz += MCChunk::CHUNK_SIZE_Z;
	}
	
	return chunk->getVoxel(lx, ly, lz);
}

void ChunkManager::setVoxel(int x, int y, int z, uint8_t id) {
	glm::ivec3 chunkPos = worldToChunk(glm::vec3(x, y, z));
	std::string key = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z);
	
	auto it = chunks.find(key);
	if (it == chunks.end()) {
		// Чанк не найден - создаем его (для загрузки сохранений)
		generateChunk(chunkPos.x, chunkPos.y, chunkPos.z);
		it = chunks.find(key);
		if (it == chunks.end()) {
			std::cout << "[DEBUG] Failed to create chunk for world coords (" << x << ", " << y << ", " << z 
			          << ") chunk coords (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
			return;
		}
	}
	
	MCChunk* chunk = it->second;
	
	// Вычисляем локальные координаты правильно
	int lx = x - chunkPos.x * MCChunk::CHUNK_SIZE_X;
	int ly = y - chunkPos.y * MCChunk::CHUNK_SIZE_Y;
	int lz = z - chunkPos.z * MCChunk::CHUNK_SIZE_Z;
	
	// Корректировка для отрицательных координат
	// Например, для x = -1 и chunkPos.x = -1, lx должно быть 31 (CHUNK_SIZE_X - 1)
	if (lx < 0) {
		lx += MCChunk::CHUNK_SIZE_X;
	}
	if (ly < 0) {
		ly += MCChunk::CHUNK_SIZE_Y;
	}
	if (lz < 0) {
		lz += MCChunk::CHUNK_SIZE_Z;
	}
	
	// Проверяем границы
	if (lx < 0 || lx >= MCChunk::CHUNK_SIZE_X || 
	    ly < 0 || ly >= MCChunk::CHUNK_SIZE_Y || 
	    lz < 0 || lz >= MCChunk::CHUNK_SIZE_Z) {
		std::cout << "[DEBUG] Local coords out of bounds: world(" << x << ", " << y << ", " << z 
		          << ") chunk(" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z 
		          << ") local(" << lx << ", " << ly << ", " << lz << ")" << std::endl;
		return;
	}
	
	// Устанавливаем блок
	chunk->setVoxel(lx, ly, lz, id);
	
	// Проверяем, что блок установился
	voxel* checkVox = chunk->getVoxel(lx, ly, lz);
	if (checkVox == nullptr || checkVox->id != id) {
		std::cout << "[DEBUG] Block not set correctly: world(" << x << ", " << y << ", " << z 
		          << ") chunk(" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z 
		          << ") local(" << lx << ", " << ly << ", " << lz << ") id=" << (int)id << std::endl;
	}
	
	// Помечаем соседние чанки как измененные, если блок на границе
	if (lx == 0) {
		std::string neighborKey = chunkKey(chunkPos.x - 1, chunkPos.y, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (lx == MCChunk::CHUNK_SIZE_X - 1) {
		std::string neighborKey = chunkKey(chunkPos.x + 1, chunkPos.y, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (ly == 0) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y - 1, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (ly == MCChunk::CHUNK_SIZE_Y - 1) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y + 1, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (lz == 0) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z - 1);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (lz == MCChunk::CHUNK_SIZE_Z - 1) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z + 1);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
}

voxel* ChunkManager::rayCast(const glm::vec3& a, const glm::vec3& dir, float maxDist, glm::vec3& end, glm::vec3& norm, glm::vec3& iend) {
	float px = a.x;
	float py = a.y;
	float pz = a.z;
	
	float dx = dir.x;
	float dy = dir.y;
	float dz = dir.z;
	
	float t = 0.0f;
	int ix = (int)std::floor(px);
	int iy = (int)std::floor(py);
	int iz = (int)std::floor(pz);
	
	float stepx = (dx > 0.0f) ? 1.0f : -1.0f;
	float stepy = (dy > 0.0f) ? 1.0f : -1.0f;
	float stepz = (dz > 0.0f) ? 1.0f : -1.0f;
	
	float infinity = std::numeric_limits<float>::infinity();
	
	float txDelta = (dx == 0.0f) ? infinity : std::abs(1.0f / dx);
	float tyDelta = (dy == 0.0f) ? infinity : std::abs(1.0f / dy);
	float tzDelta = (dz == 0.0f) ? infinity : std::abs(1.0f / dz);
	
	float xdist = (stepx > 0) ? (ix + 1 - px) : (px - ix);
	float ydist = (stepy > 0) ? (iy + 1 - py) : (py - iy);
	float zdist = (stepz > 0) ? (iz + 1 - pz) : (pz - iz);
	
	float txMax = (txDelta < infinity) ? txDelta * xdist : infinity;
	float tyMax = (tyDelta < infinity) ? tyDelta * ydist : infinity;
	float tzMax = (tzDelta < infinity) ? tzDelta * zdist : infinity;
	
	int steppedIndex = -1;
	
	while (t <= maxDist){
		voxel* vox = getVoxel(ix, iy, iz);
		if (vox != nullptr && vox->id != 0){
			end.x = px + t * dx;
			end.y = py + t * dy;
			end.z = pz + t * dz;
			
			iend.x = ix;
			iend.y = iy;
			iend.z = iz;
			
			norm.x = norm.y = norm.z = 0.0f;
			if (steppedIndex == 0) norm.x = -stepx;
			if (steppedIndex == 1) norm.y = -stepy;
			if (steppedIndex == 2) norm.z = -stepz;
			return vox;
		}
		if (txMax < tyMax) {
			if (txMax < tzMax) {
				ix += (int)stepx;
				t = txMax;
				txMax += txDelta;
				steppedIndex = 0;
			} else {
				iz += (int)stepz;
				t = tzMax;
				tzMax += tzDelta;
				steppedIndex = 2;
			}
		} else {
			if (tyMax < tzMax) {
				iy += (int)stepy;
				t = tyMax;
				tyMax += tyDelta;
				steppedIndex = 1;
			} else {
				iz += (int)stepz;
				t = tzMax;
				tzMax += tzDelta;
				steppedIndex = 2;
			}
		}
	}
	iend.x = ix;
	iend.y = iy;
	iend.z = iz;
	
	end.x = px + t * dx;
	end.y = py + t * dy;
	end.z = pz + t * dz;
	norm.x = norm.y = norm.z = 0.0f;
	return nullptr;
}

bool ChunkManager::rayCastSurface(const glm::vec3& start, const glm::vec3& dir, float maxDist, glm::vec3& hitPos, glm::vec3& hitNorm) {
	// Простой raycast по поверхности Marching Cubes
	// Ищем пересечение луча с изосерфейсом (где плотность = 0)
	
	float stepSize = 0.1f; // Шаг для проверки плотности
	float t = 0.0f;
	float lastDensity = 0.0f;
	
	while (t < maxDist) {
		glm::vec3 pos = start + dir * t;
		
		// Получаем плотность в этой точке
		float density = 0.0f;
		glm::ivec3 chunkPos = worldToChunk(pos);
		std::string key = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z);
		
		auto it = chunks.find(key);
		if (it != chunks.end()) {
			MCChunk* chunk = it->second;
			density = chunk->getDensity(pos);
		}
		
		// Если плотность изменила знак, значит пересекли изосерфейс
		if (lastDensity != 0.0f && (lastDensity > 0.0f) != (density > 0.0f)) {
			// Нашли пересечение - используем бисекцию для точности
			float t0 = t - stepSize;
			float t1 = t;
			float d0 = lastDensity;
			float d1 = density;
			
			// Биссекция для точного нахождения точки пересечения
			for (int i = 0; i < 5; i++) {
				float tm = (t0 + t1) * 0.5f;
				glm::vec3 pm = start + dir * tm;
				
				float dm = 0.0f;
				glm::ivec3 chunkPosM = worldToChunk(pm);
				std::string keyM = chunkKey(chunkPosM.x, chunkPosM.y, chunkPosM.z);
				auto itM = chunks.find(keyM);
				if (itM != chunks.end()) {
					dm = itM->second->getDensity(pm);
				}
				
				if ((d0 > 0.0f) == (dm > 0.0f)) {
					t0 = tm;
					d0 = dm;
				} else {
					t1 = tm;
					d1 = dm;
				}
			}
			
			hitPos = start + dir * ((t0 + t1) * 0.5f);
			
			// Вычисляем нормаль через градиент плотности
			float eps = 0.1f;
			float dx = 0.0f, dy = 0.0f, dz = 0.0f;
			
			glm::ivec3 chunkPosG = worldToChunk(hitPos);
			std::string keyG = chunkKey(chunkPosG.x, chunkPosG.y, chunkPosG.z);
			auto itG = chunks.find(keyG);
			if (itG != chunks.end()) {
				MCChunk* chunkG = itG->second;
				dx = chunkG->getDensity(hitPos + glm::vec3(eps, 0, 0)) - 
				     chunkG->getDensity(hitPos - glm::vec3(eps, 0, 0));
				dy = chunkG->getDensity(hitPos + glm::vec3(0, eps, 0)) - 
				     chunkG->getDensity(hitPos - glm::vec3(0, eps, 0));
				dz = chunkG->getDensity(hitPos + glm::vec3(0, 0, eps)) - 
				     chunkG->getDensity(hitPos - glm::vec3(0, 0, eps));
			}
			
			hitNorm = glm::normalize(glm::vec3(dx, dy, dz));
			if (hitNorm.y < 0.0f) {
				hitNorm = -hitNorm; // Нормаль должна быть направлена вверх
			}
			
			return true;
		}
		
		lastDensity = density;
		t += stepSize;
	}
	
	return false;
}

