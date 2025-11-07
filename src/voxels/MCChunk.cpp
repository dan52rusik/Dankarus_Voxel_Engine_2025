#include "MCChunk.h"
#include "graphics/MarchingCubes.h"
#include <vector>

MCChunk::MCChunk(int cx, int cy, int cz) 
	: chunkPos(cx, cy, cz), mesh(nullptr), generated(false) {
	// Вычисляем позицию в мире (центр чанка)
	worldPos = glm::vec3(
		cx * CHUNK_SIZE_X + CHUNK_SIZE_X / 2.0f,
		cy * CHUNK_SIZE_Y + CHUNK_SIZE_Y / 2.0f,
		cz * CHUNK_SIZE_Z + CHUNK_SIZE_Z / 2.0f
	);
}

MCChunk::~MCChunk() {
	if (mesh != nullptr) {
		delete mesh;
	}
}

void MCChunk::generate(OpenSimplex3D& noise, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation) {
	if (generated) {
		return;
	}
	
	const int NX = CHUNK_SIZE_X;
	const int NY = CHUNK_SIZE_Y;
	const int NZ = CHUNK_SIZE_Z;
	const int SX = NX + 1;
	const int SY = NY + 1;
	const int SZ = NZ + 1;
	
	densityField.resize(SX * SY * SZ);
	
	// Генерируем поле плотности для этого чанка
	for (int y = 0; y < SY; y++) {
		for (int z = 0; z < SZ; z++) {
			for (int x = 0; x < SX; x++) {
				// Мировые координаты точки
				float wx = worldPos.x - CHUNK_SIZE_X / 2.0f + (float)x;
				float wy = worldPos.y - CHUNK_SIZE_Y / 2.0f + (float)y;
				float wz = worldPos.z - CHUNK_SIZE_Z / 2.0f + (float)z;
				
				// Вычисляем высоту поверхности в точке (x, z) используя шум только по X и Z
				float heightNoise = noise.fbm(wx * baseFreq, 0.0f, wz * baseFreq, octaves, lacunarity, gain);
				float surfaceHeight = baseHeight + heightNoise * heightVariation;
				
				// Плотность: если y < surfaceHeight, то плотность положительная (земля)
				float density = surfaceHeight - wy;
				
				densityField[(y * SZ + z) * SX + x] = density;
			}
		}
	}
	
	// Генерируем меш из поля плотности
	mesh = buildIsoSurface(densityField.data(), NX, NY, NZ, 0.0f);
	generated = true;
}

