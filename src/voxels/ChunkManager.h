#ifndef VOXELS_CHUNKMANAGER_H_
#define VOXELS_CHUNKMANAGER_H_

#include "MCChunk.h"
#include "noise/OpenSimplex.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>

class ChunkManager {
public:
	ChunkManager();
	~ChunkManager();
	
	// Обновляет видимые чанки вокруг камеры
	void update(const glm::vec3& cameraPos, int renderDistance);
	
	// Получить все видимые чанки для отрисовки
	std::vector<MCChunk*> getVisibleChunks() const;
	
	// Параметры генерации
	void setNoiseParams(float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation);
	
private:
	std::unordered_map<std::string, MCChunk*> chunks;
	OpenSimplex3D noise;
	
	// Параметры генерации
	float baseFreq;
	int octaves;
	float lacunarity;
	float gain;
	float baseHeight;
	float heightVariation;
	
	// Вспомогательные функции
	std::string chunkKey(int cx, int cy, int cz) const;
	glm::ivec3 worldToChunk(const glm::vec3& worldPos) const;
	void generateChunk(int cx, int cy, int cz);
	void unloadDistantChunks(const glm::vec3& cameraPos, int renderDistance);
};

#endif /* VOXELS_CHUNKMANAGER_H_ */

