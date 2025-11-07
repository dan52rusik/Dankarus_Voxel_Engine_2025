#ifndef VOXELS_MCCHUNK_H_
#define VOXELS_MCCHUNK_H_

#include "graphics/Mesh.h"
#include "noise/OpenSimplex.h"
#include <glm/glm.hpp>
#include <vector>

class MCChunk {
public:
	glm::ivec3 chunkPos; // Позиция чанка в координатах чанков (не в мире)
	glm::vec3 worldPos;  // Позиция чанка в мире (центр чанка)
	Mesh* mesh;
	bool generated;
	
	// Параметры генерации
	static const int CHUNK_SIZE_X = 32;
	static const int CHUNK_SIZE_Y = 32;
	static const int CHUNK_SIZE_Z = 32;
	
	MCChunk(int cx, int cy, int cz);
	~MCChunk();
	
	void generate(OpenSimplex3D& noise, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation);
	
private:
	std::vector<float> densityField;
};

#endif /* VOXELS_MCCHUNK_H_ */

