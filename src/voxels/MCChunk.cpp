#include "MCChunk.h"
#include "graphics/MarchingCubes.h"
#include <vector>
#include <iostream>

MCChunk::MCChunk(int cx, int cy, int cz) 
	: chunkPos(cx, cy, cz), mesh(nullptr), voxelMesh(nullptr), generated(false), voxelMeshModified(true) {
	// Вычисляем позицию в мире (центр чанка)
	worldPos = glm::vec3(
		cx * CHUNK_SIZE_X + CHUNK_SIZE_X / 2.0f,
		cy * CHUNK_SIZE_Y + CHUNK_SIZE_Y / 2.0f,
		cz * CHUNK_SIZE_Z + CHUNK_SIZE_Z / 2.0f
	);
	
	// Инициализируем массив вокселей
	voxels = new voxel[CHUNK_VOL];
	for (int i = 0; i < CHUNK_VOL; i++) {
		voxels[i].id = 0; // Все блоки пустые по умолчанию
		voxels[i].density = 0.0f;
	}
}

MCChunk::~MCChunk() {
	if (mesh != nullptr) {
		delete mesh;
	}
	if (voxelMesh != nullptr) {
		delete voxelMesh;
	}
	if (voxels != nullptr) {
		delete[] voxels;
	}
}

voxel* MCChunk::getVoxel(int lx, int ly, int lz) {
	if (lx < 0 || lx >= CHUNK_SIZE_X || ly < 0 || ly >= CHUNK_SIZE_Y || lz < 0 || lz >= CHUNK_SIZE_Z) {
		return nullptr;
	}
	return &voxels[(ly * CHUNK_SIZE_Z + lz) * CHUNK_SIZE_X + lx];
}

void MCChunk::setVoxel(int lx, int ly, int lz, uint8_t id) {
	if (lx < 0 || lx >= CHUNK_SIZE_X || ly < 0 || ly >= CHUNK_SIZE_Y || lz < 0 || lz >= CHUNK_SIZE_Z) {
		std::cout << "[DEBUG] MCChunk::setVoxel: local coords out of bounds: (" << lx << ", " << ly << ", " << lz << ")" << std::endl;
		return;
	}
	int index = (ly * CHUNK_SIZE_Z + lz) * CHUNK_SIZE_X + lx;
	if (index < 0 || index >= CHUNK_VOL) {
		std::cout << "[DEBUG] MCChunk::setVoxel: index out of bounds: " << index << " for (" << lx << ", " << ly << ", " << lz << ")" << std::endl;
		return;
	}
	voxels[index].id = id;
	voxelMeshModified = true;
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
	// ВАЖНО: используем одинаковые мировые координаты для всех чанков,
	// чтобы значения на границах совпадали
	for (int y = 0; y < SY; y++) {
		for (int z = 0; z < SZ; z++) {
			for (int x = 0; x < SX; x++) {
				// Мировые координаты точки
				// Используем точные мировые координаты, чтобы значения на границах совпадали
				float wx = (float)(chunkPos.x * CHUNK_SIZE_X + x);
				float wy = (float)(chunkPos.y * CHUNK_SIZE_Y + y);
				float wz = (float)(chunkPos.z * CHUNK_SIZE_Z + z);
				
				// Вычисляем высоту поверхности в точке (x, z) используя шум только по X и Z
				// ВАЖНО: используем те же мировые координаты, что и в соседних чанках
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

float MCChunk::getDensity(const glm::vec3& worldPos) const {
	if (!generated) {
		return 0.0f;
	}
	
	// Вычисляем локальные координаты от начала чанка
	// Используем те же координаты, что и при генерации
	float localX = worldPos.x - (float)(chunkPos.x * CHUNK_SIZE_X);
	float localY = worldPos.y - (float)(chunkPos.y * CHUNK_SIZE_Y);
	float localZ = worldPos.z - (float)(chunkPos.z * CHUNK_SIZE_Z);
	
	// Проверяем границы
	if (localX < 0 || localX >= CHUNK_SIZE_X + 1 ||
	    localY < 0 || localY >= CHUNK_SIZE_Y + 1 ||
	    localZ < 0 || localZ >= CHUNK_SIZE_Z + 1) {
		return 0.0f; // Вне чанка
	}
	
	// Трилинейная интерполяция
	int x0 = (int)std::floor(localX);
	int y0 = (int)std::floor(localY);
	int z0 = (int)std::floor(localZ);
	int x1 = x0 + 1;
	int y1 = y0 + 1;
	int z1 = z0 + 1;
	
	// Ограничиваем индексы
	x0 = std::max(0, std::min(x0, CHUNK_SIZE_X));
	y0 = std::max(0, std::min(y0, CHUNK_SIZE_Y));
	z0 = std::max(0, std::min(z0, CHUNK_SIZE_Z));
	x1 = std::max(0, std::min(x1, CHUNK_SIZE_X));
	y1 = std::max(0, std::min(y1, CHUNK_SIZE_Y));
	z1 = std::max(0, std::min(z1, CHUNK_SIZE_Z));
	
	const int SX = CHUNK_SIZE_X + 1;
	const int SY = CHUNK_SIZE_Y + 1;
	const int SZ = CHUNK_SIZE_Z + 1;
	
	float fx = localX - x0;
	float fy = localY - y0;
	float fz = localZ - z0;
	
	// Получаем значения в углах куба
	float d000 = densityField[(y0 * SZ + z0) * SX + x0];
	float d100 = densityField[(y0 * SZ + z0) * SX + x1];
	float d010 = densityField[(y1 * SZ + z0) * SX + x0];
	float d110 = densityField[(y1 * SZ + z0) * SX + x1];
	float d001 = densityField[(y0 * SZ + z1) * SX + x0];
	float d101 = densityField[(y0 * SZ + z1) * SX + x1];
	float d011 = densityField[(y1 * SZ + z1) * SX + x0];
	float d111 = densityField[(y1 * SZ + z1) * SX + x1];
	
	// Трилинейная интерполяция
	float d00 = d000 * (1.0f - fx) + d100 * fx;
	float d01 = d001 * (1.0f - fx) + d101 * fx;
	float d10 = d010 * (1.0f - fx) + d110 * fx;
	float d11 = d011 * (1.0f - fx) + d111 * fx;
	
	float d0 = d00 * (1.0f - fy) + d10 * fy;
	float d1 = d01 * (1.0f - fy) + d11 * fy;
	
	return d0 * (1.0f - fz) + d1 * fz;
}

