#ifndef VOXELS_MCCHUNK_H_
#define VOXELS_MCCHUNK_H_

#include "graphics/Mesh.h"
#include "noise/OpenSimplex.h"
#include "voxel.h"
#include <glm/glm.hpp>
#include <vector>

class MCChunk {
public:
	glm::ivec3 chunkPos; // Позиция чанка в координатах чанков (не в мире)
	glm::vec3 worldPos;  // Позиция чанка в мире (центр чанка)
	Mesh* mesh; // Меш для Marching Cubes (поверхность земли)
	Mesh* voxelMesh; // Меш для воксельных блоков
	bool generated;
	bool voxelMeshModified; // Флаг для пересборки меша вокселей
	
	// Параметры генерации
	static const int CHUNK_SIZE_X = 32;
	static const int CHUNK_SIZE_Y = 32;
	static const int CHUNK_SIZE_Z = 32;
	static const int CHUNK_VOL = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
	
	MCChunk(int cx, int cy, int cz);
	~MCChunk();
	
	void generate(OpenSimplex3D& noise, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation);
	
	// Система воксельных блоков
	voxel* voxels; // Массив вокселей для блоков
	voxel* getVoxel(int lx, int ly, int lz); // Получить воксель по локальным координатам
	void setVoxel(int lx, int ly, int lz, uint8_t id); // Установить воксель
	
	// Получить плотность в точке (для raycast по поверхности)
	float getDensity(const glm::vec3& worldPos) const;
	
private:
	std::vector<float> densityField;
};

#endif /* VOXELS_MCCHUNK_H_ */

