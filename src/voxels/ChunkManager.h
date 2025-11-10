#ifndef VOXELS_CHUNKMANAGER_H_
#define VOXELS_CHUNKMANAGER_H_

#include "MCChunk.h"
#include "voxel.h"
#include "noise/OpenSimplex.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <limits>

class ChunkManager {
public:
	ChunkManager();
	~ChunkManager();
	
	// Обновляет видимые чанки вокруг камеры
	void update(const glm::vec3& cameraPos, int renderDistance);
	
	// Получить все видимые чанки для отрисовки
	std::vector<MCChunk*> getVisibleChunks() const;
	
	// Получить все загруженные чанки (для сохранения)
	std::vector<MCChunk*> getAllChunks() const;
	
	// Параметры генерации
	void setNoiseParams(float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation);
	void getNoiseParams(float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation) const;
	void setSeed(int64_t seed); // Установить seed для генерации мира
	
	// Очистка всех чанков (для создания нового мира)
	void clear();
	
	// Система воксельных блоков
	voxel* getVoxel(int x, int y, int z); // Получить воксель по мировым координатам
	void setVoxel(int x, int y, int z, uint8_t id); // Установить воксель по мировым координатам
	voxel* rayCast(const glm::vec3& start, const glm::vec3& dir, float maxDist, glm::vec3& end, glm::vec3& norm, glm::vec3& iend); // Raycast для определения блока
	bool rayCastSurface(const glm::vec3& start, const glm::vec3& dir, float maxDist, glm::vec3& hitPos, glm::vec3& hitNorm); // Raycast по поверхности Marching Cubes
	
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

