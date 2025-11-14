#include "WaterRenderer.h"
#include "Mesh.h"
#include "../voxels/MCChunk.h"
#include "../voxels/WaterData.h"
#include "../voxels/WaterUtils.h"
#include "../voxels/WaterConstants.h"
#include "MarchingCubes.h"
#include "../voxels/MarchingTables.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

#define WATER_VERTEX_SIZE (3 + 3 + 2 + 1) // position(3) + normal(3) + texCoord(2) + alpha(1)

// Атрибуты для меша воды
int water_attrs[] = {3, 3, 2, 1, 0}; // position(3), normal(3), texCoord(2), alpha(1)

using namespace glm;

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
	// Буфер больше не используется напрямую - используем std::vector в render()
	buffer = nullptr; // Оставляем для совместимости, но не выделяем память
}

WaterRenderer::~WaterRenderer() {
	// Буфер больше не выделяется в конструкторе
	if (buffer != nullptr) {
		delete[] buffer;
	}
}

namespace {
	// Helper: get index in 3D array (x-fastest order)
	inline int idx3(int x, int y, int z, int sx, int sy) {
		return (y * sy + z) * sx + x;
	}

	// Sample water density with bounds checking
	inline float sampleWaterDensity(const float* density, int x, int y, int z, int sx, int sy, int sz) {
		x = std::max(0, std::min(x, sx - 1));
		y = std::max(0, std::min(y, sy - 1));
		z = std::max(0, std::min(z, sz - 1));
		return density[idx3(x, y, z, sx, sy)];
	}

	// Calculate normal via gradient (central differences)
	// ИСПРАВЛЕНО: добавлена защита от NaN при нулевом градиенте
	vec3 calculateWaterNormal(const float* density, int x, int y, int z, int sx, int sy, int sz) {
		float dx = sampleWaterDensity(density, x + 1, y, z, sx, sy, sz) - sampleWaterDensity(density, x - 1, y, z, sx, sy, sz);
		float dy = sampleWaterDensity(density, x, y + 1, z, sx, sy, sz) - sampleWaterDensity(density, x, y - 1, z, sx, sy, sz);
		float dz = sampleWaterDensity(density, x, y, z + 1, sx, sy, sz) - sampleWaterDensity(density, x, y, z - 1, sx, sy, sz);
		vec3 grad(dx, dy, dz);
		float len2 = dot(grad, grad);
		// Защита от нулевого градиента (плоская поверхность)
		if (len2 < 1e-8f) {
			return vec3(0.0f, 1.0f, 0.0f); // Нормаль "вверх" по умолчанию для плоской воды
		}
		return grad / std::sqrt(len2);
	}

	// Create vertex by interpolating between two corner points (with alpha)
	struct WaterVertex {
		vec3 position;
		vec3 normal;
		float alpha;
	};

	WaterVertex createWaterVertex(const float* density, const float* alphaField,
	                              int x0, int y0, int z0, int x1, int y1, int z1,
	                              int sx, int sy, int sz, float isoLevel) {
		vec3 posA((float)x0, (float)y0, (float)z0);
		vec3 posB((float)x1, (float)y1, (float)z1);
		
		float densityA = sampleWaterDensity(density, x0, y0, z0, sx, sy, sz);
		float densityB = sampleWaterDensity(density, x1, y1, z1, sx, sy, sz);

		// Interpolate position
		float t = (isoLevel - densityA) / ((densityB - densityA) != 0.0f ? (densityB - densityA) : 1e-6f);
		t = std::max(0.0f, std::min(1.0f, t)); // Clamp
		vec3 position = posA + t * (posB - posA);

		// Interpolate normal
		vec3 normalA = calculateWaterNormal(density, x0, y0, z0, sx, sy, sz);
		vec3 normalB = calculateWaterNormal(density, x1, y1, z1, sx, sy, sz);
		vec3 normal = normalize(normalA + t * (normalB - normalA));

		// Interpolate alpha
		float alphaA = alphaField[idx3(x0, y0, z0, sx, sy)];
		float alphaB = alphaField[idx3(x1, y1, z1, sx, sy)];
		float alpha = alphaA + t * (alphaB - alphaA);

		WaterVertex v;
		v.position = position;
		v.normal = normal;
		v.alpha = alpha;
		return v;
	}
}

Mesh* WaterRenderer::render(MCChunk* chunk) {
	if (!chunk || !chunk->waterData || !chunk->waterData->hasActiveWater()) {
		return nullptr; // Нет воды в чанке
	}
	
	WaterData* waterData = chunk->waterData;
	const int NX = MCChunk::CHUNK_SIZE_X;
	const int NY = MCChunk::CHUNK_SIZE_Y;
	const int NZ = MCChunk::CHUNK_SIZE_Z;
	const int SX = NX + 1;
	const int SY = NY + 1;
	const int SZ = NZ + 1;
	
	// Создаём поле плотности для воды (на основе масс воды)
	// Плотность > 0 означает воду, плотность < 0 означает воздух
	std::vector<float> waterDensity;
	std::vector<float> waterAlpha;
	waterDensity.resize(SX * SY * SZ, -1.0f); // По умолчанию воздух (отрицательная плотность)
	waterAlpha.resize(SX * SY * SZ, 0.0f);
	
	// Порог массы для рендера - не рендерим мусорные маленькие массы
	// Это предотвращает появление тонких "жилок" воды от остаточных масс
	const int RENDER_MIN_MASS = WaterConstants::MIN_MASS_SIDE_SPREAD; // Используем минимальную массу для бокового распространения
	
	// Заполняем поле плотности на основе масс воды
	// ИСПРАВЛЕНО: используем максимальное значение при заполнении углов,
	// чтобы не перезаписывать большие значения меньшими (это устраняет пропуски)
	for (int y = 0; y < NY; y++) {
		for (int z = 0; z < NZ; z++) {
			for (int x = 0; x < NX; x++) {
				int voxelIndex = WaterUtils::GetVoxelIndex<NX, NY, NZ>(x, y, z);
				int mass = waterData->getVoxelMass(voxelIndex);
				
				// ИСПРАВЛЕНО: не рендерим мусорные маленькие массы
				if (mass < RENDER_MIN_MASS) {
					continue; // Пропускаем воксели с недостаточной массой
				}
				
				// Нормализуем массу в плотность (0.0 - 1.0)
				// Используем мягкую шкалу для более плавного распределения
				float normalizedMassRaw = static_cast<float>(mass) / static_cast<float>(WaterUtils::WATER_MASS_MAX);
				// Применяем степенную функцию для поднятия плотности "толстой" воды
				// и подавления почти пустых вокселей
				float normalizedMass = std::pow(normalizedMassRaw, 0.7f);
				float alpha = std::min(1.0f, normalizedMassRaw * 0.8f + 0.2f);
				
				// Заполняем все 8 углов вокселя, используя МАКСИМАЛЬНОЕ значение
				// Это предотвращает перезапись больших значений меньшими
				for (int dy = 0; dy <= 1; dy++) {
					for (int dz = 0; dz <= 1; dz++) {
						for (int dx = 0; dx <= 1; dx++) {
							int idx = idx3(x + dx, y + dy, z + dz, SX, SY);
							// Используем максимальное значение плотности (не перезаписываем меньшими)
							waterDensity[idx] = std::max(waterDensity[idx], normalizedMass);
							waterAlpha[idx] = std::max(waterAlpha[idx], alpha);
						}
					}
				}
			}
		}
	}
	
	// ИСПРАВЛЕНО: применяем лёгкое сглаживание поля плотности и альфы для устранения зубчатости
	// Делаем один проход box blur по XZ плоскости (3x3x1), чтобы убрать резкие грани
	std::vector<float> smoothedDensity = waterDensity;
	std::vector<float> smoothedAlpha = waterAlpha;
	for (int y = 0; y < SY; y++) {
		for (int z = 1; z < SZ - 1; z++) {
			for (int x = 1; x < SX - 1; x++) {
				int idx = idx3(x, y, z, SX, SY);
				// Box blur 3x3 по XZ (только горизонтальное сглаживание)
				float sumDensity = 0.0f;
				float sumAlpha = 0.0f;
				float count = 0.0f;
				for (int dz = -1; dz <= 1; dz++) {
					for (int dx = -1; dx <= 1; dx++) {
						int sampleIdx = idx3(x + dx, y, z + dz, SX, SY);
						// Учитываем только положительные значения (вода), игнорируем воздух
						if (waterDensity[sampleIdx] > 0.0f) {
							sumDensity += waterDensity[sampleIdx];
							sumAlpha += waterAlpha[sampleIdx];
							count += 1.0f;
						}
					}
				}
				// ИСПРАВЛЕНО: применяем сглаживание только там, где уже есть вода
				// Это предотвращает "распухание" воды в воздух
				if (count > 0.0f && waterDensity[idx] > 0.0f) {
					float blurredDensity = sumDensity / count;
					float blurredAlpha = sumAlpha / count;
					// Настоящий blur: смешиваем исходное значение с усреднённым
					smoothedDensity[idx] = waterDensity[idx] * 0.3f + blurredDensity * 0.7f;
					smoothedAlpha[idx] = waterAlpha[idx] * 0.3f + blurredAlpha * 0.7f;
				} else {
					// В воздухе оставляем исходное значение (не создаём воду из ничего)
					smoothedDensity[idx] = waterDensity[idx];
					smoothedAlpha[idx] = waterAlpha[idx];
				}
			}
		}
	}
	// Заменяем исходные поля на сглаженные
	waterDensity = smoothedDensity;
	waterAlpha = smoothedAlpha;
	
	// Используем Marching Cubes для генерации плавной поверхности воды
	// ИСПРАВЛЕНО: понижен isoLevel до 0.01, чтобы даже малые массы воды отображались
	// isoLevel = 0.01 означает, что поверхность воды будет там, где плотность > 0.01
	const float isoLevel = 0.01f;
	
	std::vector<float> buffer;
	buffer.reserve(NX * NY * NZ * WATER_VERTEX_SIZE * 6); // Оценка размера
	
	// Вычисляем смещение для мировых координат
	float worldOffsetX = chunk->worldPos.x - NX / 2.0f;
	float worldOffsetY = chunk->worldPos.y - NY / 2.0f;
	float worldOffsetZ = chunk->worldPos.z - NZ / 2.0f;
	
	// Process each cube (Marching Cubes алгоритм)
	for (int y = 0; y < NY; y++) {
		for (int z = 0; z < NZ; z++) {
			for (int x = 0; x < NX; x++) {
				// Calculate coordinates of each corner of the current cube
				int cornerCoords[8][3] = {
					{x,     y,     z    },
					{x + 1, y,     z    },
					{x + 1, y,     z + 1},
					{x,     y,     z + 1},
					{x,     y + 1, z    },
					{x + 1, y + 1, z    },
					{x + 1, y + 1, z + 1},
					{x,     y + 1, z + 1}
				};

				// Calculate cube configuration
				int cubeConfiguration = 0;
				bool hasAnyWater = false;
				for (int i = 0; i < 8; i++) {
					float d = sampleWaterDensity(waterDensity.data(), 
					                            cornerCoords[i][0], cornerCoords[i][1], cornerCoords[i][2], 
					                            SX, SY, SZ);
					// Для воды: плотность > isoLevel означает воду внутри
					// ИСПРАВЛЕНО: также проверяем, что плотность положительная (не воздух)
					if (d > isoLevel && d > 0.0f) {
						cubeConfiguration |= (1 << i);
						hasAnyWater = true;
					}
				}
				
				// Пропускаем кубы без воды (оптимизация)
				if (!hasAnyWater) {
					continue;
				}

				// Get triangulation for this configuration
				const int* edgeIndices = TRIANGULATION_TABLE[cubeConfiguration];

				// Create triangles
				for (int i = 0; i < 16; i += 3) {
					if (edgeIndices[i] == -1) break;

					// Get edge indices
					int edgeIndexA = edgeIndices[i];
					int edgeIndexB = edgeIndices[i + 1];
					int edgeIndexC = edgeIndices[i + 2];

					// Get corner indices for each edge
					int a0 = CORNER_INDEX_A_FROM_EDGE[edgeIndexA];
					int a1 = CORNER_INDEX_B_FROM_EDGE[edgeIndexA];
					int b0 = CORNER_INDEX_A_FROM_EDGE[edgeIndexB];
					int b1 = CORNER_INDEX_B_FROM_EDGE[edgeIndexB];
					int c0 = CORNER_INDEX_A_FROM_EDGE[edgeIndexC];
					int c1 = CORNER_INDEX_B_FROM_EDGE[edgeIndexC];

					// Create vertices (order: C, B, A - reverse order like Unity!)
					WaterVertex vertexC = createWaterVertex(waterDensity.data(), waterAlpha.data(),
						cornerCoords[c0][0], cornerCoords[c0][1], cornerCoords[c0][2],
						cornerCoords[c1][0], cornerCoords[c1][1], cornerCoords[c1][2],
						SX, SY, SZ, isoLevel);
					WaterVertex vertexB = createWaterVertex(waterDensity.data(), waterAlpha.data(),
						cornerCoords[b0][0], cornerCoords[b0][1], cornerCoords[b0][2],
						cornerCoords[b1][0], cornerCoords[b1][1], cornerCoords[b1][2],
						SX, SY, SZ, isoLevel);
					WaterVertex vertexA = createWaterVertex(waterDensity.data(), waterAlpha.data(),
						cornerCoords[a0][0], cornerCoords[a0][1], cornerCoords[a0][2],
						cornerCoords[a1][0], cornerCoords[a1][1], cornerCoords[a1][2],
						SX, SY, SZ, isoLevel);

					// Emit triangle vertices (C, B, A order - reverse winding!)
					// Vertex C
					buffer.push_back(vertexC.position.x + worldOffsetX);
					buffer.push_back(vertexC.position.y + worldOffsetY);
					buffer.push_back(vertexC.position.z + worldOffsetZ);
					buffer.push_back(vertexC.normal.x);
					buffer.push_back(vertexC.normal.y);
					buffer.push_back(vertexC.normal.z);
					buffer.push_back(vertexC.position.x / NX); // U координата текстуры
					buffer.push_back(vertexC.position.z / NZ); // V координата текстуры
					buffer.push_back(vertexC.alpha);

					// Vertex B
					buffer.push_back(vertexB.position.x + worldOffsetX);
					buffer.push_back(vertexB.position.y + worldOffsetY);
					buffer.push_back(vertexB.position.z + worldOffsetZ);
					buffer.push_back(vertexB.normal.x);
					buffer.push_back(vertexB.normal.y);
					buffer.push_back(vertexB.normal.z);
					buffer.push_back(vertexB.position.x / NX);
					buffer.push_back(vertexB.position.z / NZ);
					buffer.push_back(vertexB.alpha);

					// Vertex A
					buffer.push_back(vertexA.position.x + worldOffsetX);
					buffer.push_back(vertexA.position.y + worldOffsetY);
					buffer.push_back(vertexA.position.z + worldOffsetZ);
					buffer.push_back(vertexA.normal.x);
					buffer.push_back(vertexA.normal.y);
					buffer.push_back(vertexA.normal.z);
					buffer.push_back(vertexA.position.x / NX);
					buffer.push_back(vertexA.position.z / NZ);
					buffer.push_back(vertexA.alpha);
				}
			}
		}
	}
	
	if (buffer.empty()) {
		return nullptr; // Нет вершин для рендеринга
	}
	
	size_t vertexCount = buffer.size() / WATER_VERTEX_SIZE;
	return new Mesh(buffer.data(), vertexCount, water_attrs);
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

