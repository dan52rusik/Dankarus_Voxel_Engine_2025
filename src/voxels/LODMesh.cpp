#include "LODMesh.h"
#include "../graphics/MeshBounds.h"
#include <algorithm>
#include <cmath>

namespace LODMesh {

void generateLODMesh(const ChunkSquareMesh& source, ChunkSquareMesh& target, int targetLOD) {
	if (targetLOD >= MaxLODLevels) {
		return;
	}
	
	int targetResolution = getLODResolution(targetLOD);
	int sourceResolution = source.resolution;
	
	if (targetResolution >= sourceResolution) {
		// Если целевое разрешение больше или равно исходному, просто копируем
		target = source;
		target.lodLevel = targetLOD;
		return;
	}
	
	// Вычисляем шаг даунсэмплинга
	int step = sourceResolution / targetResolution;
	
	// Инициализируем целевой меш
	int targetVertexCount = calculateVertexCount(targetResolution);
	int targetTriangleCount = calculateTriangleCount(targetResolution);
	target.init(targetVertexCount, targetLOD, targetResolution, 
	            source.colliderResolution, targetTriangleCount);
	
	// Даунсэмплинг вершин
	int targetIdx = 0;
	for (int z = 0; z <= targetResolution; z++) {
		for (int x = 0; x <= targetResolution; x++) {
			int srcX = x * step;
			int srcZ = z * step;
			int srcIdx = srcZ * (sourceResolution + 1) + srcX;
			
			if (srcIdx < (int)source.vertices.size()) {
				target.vertices[targetIdx] = source.vertices[srcIdx];
				if (targetIdx < (int)source.normals.size()) {
					target.normals[targetIdx] = source.normals[srcIdx];
				}
				if (targetIdx < (int)source.tangents.size()) {
					target.tangents[targetIdx] = source.tangents[srcIdx];
				}
				if (targetIdx < (int)source.colors.size()) {
					target.colors[targetIdx] = source.colors[srcIdx];
				}
				if (targetIdx < (int)source.textureIds.size()) {
					target.textureIds[targetIdx] = source.textureIds[srcIdx];
				}
				if (targetIdx < (int)source.isWater.size()) {
					target.isWater[targetIdx] = source.isWater[srcIdx];
				}
			}
			targetIdx++;
		}
	}
	
	// Генерируем индексы для треугольников
	uint32_t indexIdx = 0;
	for (int z = 0; z < targetResolution; z++) {
		for (int x = 0; x < targetResolution; x++) {
			int i = z * (targetResolution + 1) + x;
			
			// Первый треугольник
			target.indices[indexIdx++] = i;
			target.indices[indexIdx++] = i + targetResolution + 1;
			target.indices[indexIdx++] = i + 1;
			
			// Второй треугольник
			target.indices[indexIdx++] = i + 1;
			target.indices[indexIdx++] = i + targetResolution + 1;
			target.indices[indexIdx++] = i + targetResolution + 2;
		}
	}
	
	// Копируем метаданные
	target.chunkPosition = source.chunkPosition;
	target.waterPlaneBlockId = source.waterPlaneBlockId;
	target.voxelMesh = source.voxelMesh; // Копируем указатель на воксельный меш
	
	// Вычисляем границы (используем универсальную функцию)
	MeshBounds::Bounds bounds = MeshBounds::getBoundsFromVerts(target.vertices);
	target.boundsMin = bounds.min;
	target.boundsMax = bounds.max;
}

void generateCollider(const ChunkSquareMesh& mesh, ChunkSquareMesh& target, int colliderRes) {
	// Упрощенная версия: берем каждую N-ю вершину из визуального меша
	int step = mesh.resolution / colliderRes;
	if (step < 1) step = 1;
	
	// Инициализируем коллайдер в целевом меше
	int colliderSize = colliderRes * colliderRes;
	target.colliderVertices.resize(colliderSize);
	target.colliderVerticesHeight.resize(colliderSize);
	target.colliderResolution = colliderRes;
	
	int colliderIdx = 0;
	for (int z = 0; z < colliderRes; z++) {
		for (int x = 0; x < colliderRes; x++) {
			int srcX = x * step;
			int srcZ = z * step;
			int srcIdx = srcZ * (mesh.resolution + 1) + srcX;
			
			if (srcIdx < (int)mesh.vertices.size()) {
				// Используем только X и Z координаты, Y берем из высоты
				const glm::vec3& v = mesh.vertices[srcIdx];
				target.colliderVertices[colliderIdx] = glm::vec3(v.x, 0.0f, v.z);
				target.colliderVerticesHeight[colliderIdx] = v.y;
			}
			colliderIdx++;
		}
	}
}

} // namespace LODMesh

