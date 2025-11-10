#include "VoxelRenderer.h"
#include "Mesh.h"
#include "../voxels/MCChunk.h"
#include "../voxels/voxel.h"
#include "../lighting/LightingSystem.h"
#include "../lighting/Lighting.h"
#include <glm/glm.hpp>

#define VERTEX_SIZE (3 + 2 + 1 + 1) // position(3) + texCoord(2) + light(1) + blockId(1)

// Простая проверка: если координата внутри чанка, проверяем блок, иначе считаем, что блок не заблокирован
static bool isBlocked(MCChunk* chunk, MCChunk** nearbyChunks, int lx, int ly, int lz) {
	// Если координата внутри текущего чанка
	if (lx >= 0 && lx < MCChunk::CHUNK_SIZE_X && 
	    ly >= 0 && ly < MCChunk::CHUNK_SIZE_Y && 
	    lz >= 0 && lz < MCChunk::CHUNK_SIZE_Z) {
		voxel* vox = chunk->getVoxel(lx, ly, lz);
		return (vox != nullptr && vox->id != 0);
	}
	
	// Если координата вне текущего чанка, проверяем соседний чанк
	int cx = (lx < 0) ? -1 : (lx >= MCChunk::CHUNK_SIZE_X ? 1 : 0);
	int cy = (ly < 0) ? -1 : (ly >= MCChunk::CHUNK_SIZE_Y ? 1 : 0);
	int cz = (lz < 0) ? -1 : (lz >= MCChunk::CHUNK_SIZE_Z ? 1 : 0);
	
	int index = (cy + 1) * 9 + (cz + 1) * 3 + (cx + 1);
	if (nearbyChunks[index] == nullptr) {
		return false; // Нет соседнего чанка, считаем, что блок не заблокирован
	}
	
	MCChunk* neighborChunk = nearbyChunks[index];
	int nlx = (lx < 0) ? (MCChunk::CHUNK_SIZE_X + lx) : (lx - MCChunk::CHUNK_SIZE_X);
	int nly = (ly < 0) ? (MCChunk::CHUNK_SIZE_Y + ly) : (ly - MCChunk::CHUNK_SIZE_Y);
	int nlz = (lz < 0) ? (MCChunk::CHUNK_SIZE_Z + lz) : (lz - MCChunk::CHUNK_SIZE_Z);
	
	voxel* vox = neighborChunk->getVoxel(nlx, nly, nlz);
	return (vox != nullptr && vox->id != 0);
}

#define VERTEX(INDEX, X,Y,Z, U,V, L, ID) buffer[INDEX+0] = (X);\
								  buffer[INDEX+1] = (Y);\
								  buffer[INDEX+2] = (Z);\
								  buffer[INDEX+3] = (U);\
								  buffer[INDEX+4] = (V);\
								  buffer[INDEX+5] = (L);\
								  buffer[INDEX+6] = (ID);\
								  INDEX += VERTEX_SIZE;

int chunk_attrs[] = {3,2,1,1, 0}; // position(3), texCoord(2), light(1), blockId(1)

VoxelRenderer::VoxelRenderer(size_t capacity) : capacity(capacity) {
	buffer = new float[capacity * VERTEX_SIZE * 6];
}

VoxelRenderer::~VoxelRenderer(){
	delete[] buffer;
}

Mesh* VoxelRenderer::render(MCChunk* chunk, MCChunk** nearbyChunks, lighting::LightingSystem* lightingSystem){
	size_t index = 0;
	
	for (int y = 0; y < MCChunk::CHUNK_SIZE_Y; y++){
		for (int z = 0; z < MCChunk::CHUNK_SIZE_Z; z++){
			for (int x = 0; x < MCChunk::CHUNK_SIZE_X; x++){
				voxel* vox = chunk->getVoxel(x, y, z);
				if (vox == nullptr || vox->id == 0){
					continue;
				}
				
				unsigned int id = vox->id;
				float blockId = (float)id;
				
				// Вычисляем UV координаты из атласа (16x16 блоков)
				float uvsize = 1.0f/16.0f;
				float u = (id % 16) * uvsize;
				float v = 1.0f - ((1 + id / 16) * uvsize);
				
				// Мировые координаты блока
				float wx = chunk->worldPos.x - MCChunk::CHUNK_SIZE_X / 2.0f + (float)x;
				float wy = chunk->worldPos.y - MCChunk::CHUNK_SIZE_Y / 2.0f + (float)y;
				float wz = chunk->worldPos.z - MCChunk::CHUNK_SIZE_Z / 2.0f + (float)z;
				
				// Верхняя грань
				if (!isBlocked(chunk, nearbyChunks, x, y+1, z)){
					float l = 1.0f; // По умолчанию максимальное освещение
					if (lightingSystem != nullptr) {
						int bx = (int)std::round(wx);
						int by = (int)std::round(wy);
						int bz = (int)std::round(wz);
						glm::vec3 faceNormal(0.0f, 1.0f, 0.0f); // Нормаль верхней грани
						l = lightingSystem->getFaceLight(bx, by, bz, faceNormal);
					}
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz - 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz + 0.5f, u+uvsize, v+uvsize, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz + 0.5f, u, v+uvsize, l, blockId);
					
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz - 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz + 0.5f, u, v+uvsize, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz - 0.5f, u, v, l, blockId);
				}
				
				// Нижняя грань
				if (!isBlocked(chunk, nearbyChunks, x, y-1, z)){
					float l = 1.0f;
					if (lightingSystem != nullptr) {
						int bx = (int)std::round(wx);
						int by = (int)std::round(wy);
						int bz = (int)std::round(wz);
						glm::vec3 faceNormal(0.0f, -1.0f, 0.0f); // Нормаль нижней грани
						l = lightingSystem->getFaceLight(bx, by, bz, faceNormal);
					}
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, u, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz + 0.5f, u+uvsize, v+uvsize, l, blockId);
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, u, v+uvsize, l, blockId);
					
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, u, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz + 0.5f, u+uvsize, v+uvsize, l, blockId);
				}
				
				// Правая грань (+X)
				if (!isBlocked(chunk, nearbyChunks, x+1, y, z)){
					float l = 1.0f;
					if (lightingSystem != nullptr) {
						int bx = (int)std::round(wx);
						int by = (int)std::round(wy);
						int bz = (int)std::round(wz);
						glm::vec3 faceNormal(1.0f, 0.0f, 0.0f); // Нормаль правой грани
						l = lightingSystem->getFaceLight(bx, by, bz, faceNormal);
					}
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz - 0.5f, u+uvsize, v+uvsize, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz + 0.5f, u, v+uvsize, l, blockId);
					
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz + 0.5f, u, v+uvsize, l, blockId);
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz + 0.5f, u, v, l, blockId);
				}
				
				// Левая грань (-X)
				if (!isBlocked(chunk, nearbyChunks, x-1, y, z)){
					float l = 1.0f;
					if (lightingSystem != nullptr) {
						int bx = (int)std::round(wx);
						int by = (int)std::round(wy);
						int bz = (int)std::round(wz);
						glm::vec3 faceNormal(-1.0f, 0.0f, 0.0f); // Нормаль левой грани
						l = lightingSystem->getFaceLight(bx, by, bz, faceNormal);
					}
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, u, v, l, blockId);
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz + 0.5f, u+uvsize, v+uvsize, l, blockId);
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz - 0.5f, u, v+uvsize, l, blockId);
					
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, u, v, l, blockId);
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz + 0.5f, u+uvsize, v+uvsize, l, blockId);
				}
				
				// Передняя грань (+Z)
				if (!isBlocked(chunk, nearbyChunks, x, y, z+1)){
					float l = 1.0f;
					if (lightingSystem != nullptr) {
						int bx = (int)std::round(wx);
						int by = (int)std::round(wy);
						int bz = (int)std::round(wz);
						glm::vec3 faceNormal(0.0f, 0.0f, 1.0f); // Нормаль передней грани
						l = lightingSystem->getFaceLight(bx, by, bz, faceNormal);
					}
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, u, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz + 0.5f, u+uvsize, v+uvsize, l, blockId);
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz + 0.5f, u, v+uvsize, l, blockId);
					
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, u, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz + 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz + 0.5f, u+uvsize, v+uvsize, l, blockId);
				}
				
				// Задняя грань (-Z)
				if (!isBlocked(chunk, nearbyChunks, x, y, z-1)){
					float l = 1.0f;
					if (lightingSystem != nullptr) {
						int bx = (int)std::round(wx);
						int by = (int)std::round(wy);
						int bz = (int)std::round(wz);
						glm::vec3 faceNormal(0.0f, 0.0f, -1.0f); // Нормаль задней грани
						l = lightingSystem->getFaceLight(bx, by, bz, faceNormal);
					}
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx - 0.5f, wy + 0.5f, wz - 0.5f, u+uvsize, v+uvsize, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz - 0.5f, u, v+uvsize, l, blockId);
					
					VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, u+uvsize, v, l, blockId);
					VERTEX(index, wx + 0.5f, wy + 0.5f, wz - 0.5f, u, v+uvsize, l, blockId);
					VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, u, v, l, blockId);
				}
			}
		}
	}
	
	if (index == 0) {
		return nullptr; // Нет блоков для отрисовки
	}
	
	return new Mesh(buffer, index / VERTEX_SIZE, chunk_attrs);
}
