#include "WaterRenderer.h"
#include "Mesh.h"
#include "../voxels/MCChunk.h"
#include "../voxels/WaterData.h"
#include "../voxels/WaterUtils.h"
#include <glm/glm.hpp>
#include <algorithm>

#define WATER_VERTEX_SIZE (3 + 3 + 2 + 1) // position(3) + normal(3) + texCoord(2) + alpha(1)

// Атрибуты для меша воды
int water_attrs[] = {3, 3, 2, 1, 0}; // position(3), normal(3), texCoord(2), alpha(1)

#define WATER_VERTEX(INDEX, X,Y,Z, NX,NY,NZ, U,V, A) buffer[INDEX+0] = (X);\
								  buffer[INDEX+1] = (Y);\
								  buffer[INDEX+2] = (Z);\
								  buffer[INDEX+3] = (NX);\
								  buffer[INDEX+4] = (NY);\
								  buffer[INDEX+5] = (NZ);\
								  buffer[INDEX+6] = (U);\
								  buffer[INDEX+7] = (V);\
								  buffer[INDEX+8] = (A);\
								  INDEX += WATER_VERTEX_SIZE;

WaterRenderer::WaterRenderer(size_t capacity) : capacity(capacity) {
	buffer = new float[capacity * WATER_VERTEX_SIZE * 6]; // Максимум 6 граней на воксель
}

WaterRenderer::~WaterRenderer() {
	delete[] buffer;
}

Mesh* WaterRenderer::render(MCChunk* chunk) {
	if (!chunk || !chunk->waterData || !chunk->waterData->hasActiveWater()) {
		return nullptr; // Нет воды в чанке
	}
	
	size_t index = 0;
	WaterData* waterData = chunk->waterData;
	
	// Проходим по всем вокселям чанка
	for (int y = 0; y < MCChunk::CHUNK_SIZE_Y; y++) {
		for (int z = 0; z < MCChunk::CHUNK_SIZE_Z; z++) {
			for (int x = 0; x < MCChunk::CHUNK_SIZE_X; x++) {
				int voxelIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(x, y, z);
				
				// Проверяем, есть ли активная вода в этом вокселе
				if (!waterData->isVoxelActive(voxelIndex)) {
					continue;
				}
				
				int mass = waterData->getVoxelMass(voxelIndex);
				if (mass <= WaterUtils::WATER_MASS_ACTIVE) {
					continue;
				}
				
				// Мировые координаты вокселя
				float wx = chunk->worldPos.x - MCChunk::CHUNK_SIZE_X / 2.0f + (float)x;
				float wy = chunk->worldPos.y - MCChunk::CHUNK_SIZE_Y / 2.0f + (float)y;
				float wz = chunk->worldPos.z - MCChunk::CHUNK_SIZE_Z / 2.0f + (float)z;
				
				// Вычисляем высоту воды на основе массы (0.0 - 1.0)
				float waterHeight = static_cast<float>(mass) / static_cast<float>(WaterUtils::WATER_MASS_MAX);
				float waterY = wy + 0.5f + (waterHeight - 1.0f) * 0.5f; // Высота поверхности воды
				
				// Альфа-канал на основе массы воды
				float alpha = std::min(1.0f, waterHeight * 0.8f + 0.2f);
				
				// Проверяем соседние воксели для определения видимых граней
				// Верхняя грань (всегда видима, если есть вода)
				if (shouldRenderFace(waterData, x, y, z, 0, 1, 0)) {
					glm::vec3 normal(0.0f, 1.0f, 0.0f);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 0.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					
					WATER_VERTEX(index, wx - 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 1.0f, 0.0f, alpha);
				}
				
				// Боковые грани (если соседний воксель не содержит воду)
				// Передняя грань (+Z)
				if (shouldRenderFace(waterData, x, y, z, 0, 0, 1)) {
					glm::vec3 normal(0.0f, 0.0f, 1.0f);
					WATER_VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 0.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					
					WATER_VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, wy - 0.5f, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 0.0f, alpha);
				}
				
				// Задняя грань (-Z)
				if (shouldRenderFace(waterData, x, y, z, 0, 0, -1)) {
					glm::vec3 normal(0.0f, 0.0f, -1.0f);
					WATER_VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					
					WATER_VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, normal.x, normal.y, normal.z, 1.0f, 0.0f, alpha);
				}
				
				// Правая грань (+X)
				if (shouldRenderFace(waterData, x, y, z, 1, 0, 0)) {
					glm::vec3 normal(1.0f, 0.0f, 0.0f);
					WATER_VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					
					WATER_VERTEX(index, wx + 0.5f, wy - 0.5f, wz - 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx + 0.5f, wy - 0.5f, wz + 0.5f, normal.x, normal.y, normal.z, 1.0f, 0.0f, alpha);
				}
				
				// Левая грань (-X)
				if (shouldRenderFace(waterData, x, y, z, -1, 0, 0)) {
					glm::vec3 normal(-1.0f, 0.0f, 0.0f);
					WATER_VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz + 0.5f, normal.x, normal.y, normal.z, 0.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					
					WATER_VERTEX(index, wx - 0.5f, wy - 0.5f, wz + 0.5f, normal.x, normal.y, normal.z, 0.0f, 0.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, waterY, wz - 0.5f, normal.x, normal.y, normal.z, 1.0f, 1.0f, alpha);
					WATER_VERTEX(index, wx - 0.5f, wy - 0.5f, wz - 0.5f, normal.x, normal.y, normal.z, 1.0f, 0.0f, alpha);
				}
			}
		}
	}
	
	if (index == 0) {
		return nullptr; // Нет вершин для рендеринга
	}
	
	size_t vertexCount = index / WATER_VERTEX_SIZE;
	return new Mesh(buffer, vertexCount, water_attrs);
}

bool WaterRenderer::shouldRenderFace(const WaterData* waterData, int x, int y, int z, int dx, int dy, int dz) const {
	// Проверяем границы чанка
	int nx = x + dx;
	int ny = y + dy;
	int nz = z + dz;
	
	// Если соседний воксель вне чанка, рендерим грань
	if (nx < 0 || nx >= MCChunk::CHUNK_SIZE_X ||
	    ny < 0 || ny >= MCChunk::CHUNK_SIZE_Y ||
	    nz < 0 || nz >= MCChunk::CHUNK_SIZE_Z) {
		return true; // Рендерим грань на границе чанка
	}
	
	// Проверяем, есть ли вода в соседнем вокселе
	int neighborIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(nx, ny, nz);
	
	// Если соседний воксель не содержит активную воду, рендерим грань
	if (!waterData->isVoxelActive(neighborIndex)) {
		return true;
	}
	
	// Для верхней грани всегда рендерим
	if (dy > 0) {
		return true;
	}
	
	return false;
}

