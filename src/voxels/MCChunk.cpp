#include "MCChunk.h"
#include "graphics/MarchingCubes.h"
#include "WaterData.h"
#include "WaterUtils.h"
#include "PrefabManager.h"
#include "Path.h"
#include "noise/OpenSimplex.h"
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include <cfloat>  // для std::isfinite

// ============ Утилиты для генерации террейна ============

// [-1..1] -> [0..1]
static inline float remap01(float x) {
	return 0.5f * x + 0.5f;
}

// ridge: превращает fbm в острые «гребни» [0..1]
static inline float ridge(float n) {
	n = 1.0f - std::fabs(n);        // инвертируем «долины»
	return n * n;                   // усиливаем вершины
}

// Мягкие террасы с плавной притяжкой (вместо жесткого квантования)
static inline float smooth_terrace(float h, float steps, float sharp) {
	// h∈[0..1], soft террасы: квантование с плавной притяжкой
	float t = std::floor(h * steps) / steps;
	float f = (h * steps - std::floor(h * steps));
	float s = glm::smoothstep(0.0f, 1.0f, f);
	return glm::mix(h, glm::mix(t, t + 1.0f / steps, s), sharp); // sharp∈[0..1]
}

// Структура для domain warp
struct Warp {
	float dx, dz;
};

// Безопасный domain warp (низкочастотный и ограниченный)
static inline Warp domain_warp(OpenSimplex3D& noise, float x, float z, float baseFreq) {
	float wf = baseFreq * 0.25f;     // warp частота ниже базовой (в 4 раза)
	float wx = noise.fbm_norm(x * wf, 0.0f, z * wf, 3, 2.1f, 0.55f);
	float wz = noise.fbm_norm((x + 37.7f) * wf, 0.0f, (z - 19.3f) * wf, 3, 2.1f, 0.55f);
	
	// Фикс по миру: 8..24 вокселей (не через обратную частоту!)
	float maxWarp = 16.0f;     // было 12 → сделать заметнее для теста
	return { wx * maxWarp, wz * maxWarp };
}

// Мягкий порог для плотности Marching Cubes
static inline float density_from_height(float y, float h, float isoLevel, float softness) {
	// isoLevel ~ 0 (поверхность). Плотность >0 — твёрдое
	float d = (h - y) - isoLevel;
	// мягкая «S»-образная компрессия вокруг 0
	float k = glm::clamp(std::fabs(d) / softness, 0.0f, 1.0f);
	float s = d * (0.5f + 0.5f * k); // меньше резких скачков
	return s;
}

// Мягкий min для пещер (soft union/subtract)
static inline float smin(float a, float b, float k) {
	// мягкий min (k — ширина сглаживания)
	float h = glm::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
	return glm::mix(b, a, h) - k * h * (1.0f - h);
}

MCChunk::MCChunk(int cx, int cy, int cz) 
	: chunkPos(cx, cy, cz), mesh(nullptr), voxelMesh(nullptr), waterMesh(nullptr), 
	  generated(false), voxelMeshModified(true), waterMeshModified(true), dirty(false), waterData(nullptr) {
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
	
	// Инициализируем систему воды
	waterData = new WaterData();
}

MCChunk::~MCChunk() {
	if (mesh != nullptr) {
		delete mesh;
	}
	if (voxelMesh != nullptr) {
		delete voxelMesh;
	}
	if (waterMesh != nullptr) {
		delete waterMesh;
	}
	if (waterData != nullptr) {
		delete waterData;
	}
	delete[] voxels;
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
	
	// Помечаем чанк как измененный для сохранения
	dirty = true;
	voxels[index].id = id;
	voxelMeshModified = true;
}

void MCChunk::generate(OpenSimplex3D& noise, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation) {
	// ВАЖНО: не проверяем generated здесь, чтобы можно было перегенерировать чанк
	// (например, при изменении параметров генерации)
	
	// Очищаем воду при регенерации (опционально)
	if (waterData != nullptr && generated) {
		waterData->clear();
	}
	
	const int NX = CHUNK_SIZE_X;
	const int NY = CHUNK_SIZE_Y;
	const int NZ = CHUNK_SIZE_Z;
	const int SX = NX + 1;
	const int SY = NY + 1;
	const int SZ = NZ + 1;
	
	densityField.resize(SX * SY * SZ);
	
	// Параметры для мягкого порога плотности
	const float softness = 0.6f; // старт: 0.6, не больше 0.75
	const float isoLevel = 0.05f; // чуть положительный, чтобы поверхность не тонула
	
	// Генерируем поле плотности для этого чанка
	// ВАЖНО: используем одинаковые мировые координаты для всех чанков,
	// чтобы значения на границах совпадали
	for (int y = 0; y < SY; y++) {
		for (int z = 0; z < SZ; z++) {
			for (int x = 0; x < SX; x++) {
				// Мировые координаты точки - УБЕДИСЬ ЧТО FLOAT!
				float wx = static_cast<float>(chunkPos.x * CHUNK_SIZE_X + x);
				float wy = static_cast<float>(chunkPos.y * CHUNK_SIZE_Y + y);
				float wz = static_cast<float>(chunkPos.z * CHUNK_SIZE_Z + z);
				
				// Страховка от кривых рук: проверка валидности baseFreq
				if (baseFreq <= 0.0f || !std::isfinite(baseFreq)) {
					// fallback, чтобы не получить «плоский мир»
					float density = density_from_height(wy, baseHeight, isoLevel, softness);
					densityField[(y * SZ + z) * SX + x] = density;
					continue;
				}
				
				// Вычисляем высоту поверхности используя правильную композицию
				float seaLevel = baseHeight;
				float heightScale = heightVariation;
				
				// 1) Безопасный domain warp
				Warp w = domain_warp(noise, wx, wz, baseFreq);
				float xw = wx + w.dx;
				float zw = wz + w.dz;
				
				// 2) Континенты (низкая частота) - шире окно
				float cont = noise.fbm_norm(xw * baseFreq * 0.25f, 0.0f, zw * baseFreq * 0.25f, 4, 2.0f, 0.5f);
				float cont01 = glm::smoothstep(-0.35f, 0.35f, cont); // было -0.25..0.45 → перекос
				
				// 3) Холмы (средняя частота)
				float hills = noise.fbm_norm(xw * baseFreq * 0.6f, 0.0f, zw * baseFreq * 0.6f, 5, 2.0f, 0.5f);
				hills = remap01(hills);
				
				// 4) Риджи (гребни)
				float rsrc = noise.fbm_norm(xw * baseFreq * 0.35f, 0.0f, zw * baseFreq * 0.35f, 4, 2.1f, 0.5f);
				float ridg = glm::clamp((ridge(rsrc) - 0.2f) * 1.4f, 0.0f, 1.0f);
				
				// 5) Детали (высокие частоты) - не душим полностью вне гребней
				float det = noise.fbm_norm(xw * baseFreq * 1.8f, 0.0f, zw * baseFreq * 1.8f, 3, 2.2f, 0.5f);
				det = det * det; // подавить мелкий шум внизу
				float detWeight = glm::mix(0.05f, 1.0f, glm::smoothstep(0.45f, 0.8f, ridg)); // оставь хвост
				
				// 6) Смешивание слоёв по маскам - усилить риджи для выразительности
				float hillsWeight = glm::mix(0.40f, 0.55f, cont01);  // немного ослабить холмы
				float ridgWeight = glm::mix(0.35f, 0.65f, cont01);  // усилить риджи (было 0.25..0.55)
				float h01 = glm::clamp(
					hillsWeight * hills +
					ridgWeight * ridg +
					detWeight * (det * 0.25f),
					0.0f, 1.0f);
				
				// 7) Мягкие террасы - мягче и выше порог
				float terraceSharp = glm::smoothstep(0.6f, 0.92f, h01) * 0.35f; // мягче и выше порог (было 0.45f)
				h01 = smooth_terrace(h01, 9.0f, terraceSharp); // steps 8-10, НЕ ниже 6
				
				// 8) Высота поверхности
				float surfaceHeight = seaLevel + h01 * heightScale;
				
				// 9) Плотность с мягким порогом (вместо резкого)
				float density = density_from_height(wy, surfaceHeight, isoLevel, softness);
				
				densityField[(y * SZ + z) * SX + x] = density;
				
				// ДИАГНОСТИКА: выводим min/max h01 для проверки диапазона
				static float hmin = 1e9f, hmax = -1e9f;
				static int debugCount = 0;
				static int callCount = 0;
				callCount++;
				hmin = std::min(hmin, h01);
				hmax = std::max(hmax, h01);
				if (debugCount < 1 && callCount >= 1000) {  // выводим после 1000 вызовов для статистики
					std::cout << "[HEIGHT_DEBUG] h01 min=" << hmin << " max=" << hmax 
					          << " range=" << (hmax - hmin) << " baseFreq=" << baseFreq << std::endl;
					debugCount++;
				}
			}
		}
	}
	
	// Удаляем старый меш перед созданием нового
	if (mesh != nullptr) {
		delete mesh;
		mesh = nullptr;
	}
	
	// Генерируем меш из поля плотности
	mesh = buildIsoSurface(densityField.data(), NX, NY, NZ, 0.0f);
	generated = true;
	
	// ДИАГНОСТИКА
	static int debugGenerateCount = 0;
	if (debugGenerateCount < 3) {
		std::cout << "[TERRAIN_GEN] Generated chunk (" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z 
		          << ") with FIXED terrain system (normalized fbm, safe warp, masked blending, soft terraces, soft density)" << std::endl;
		debugGenerateCount++;
	}
}

// Оптимизированная генерация с использованием callback
void MCChunk::generate(std::function<float(float, float)> evalSurfaceHeight) {
	// ВАЖНО: не проверяем generated здесь, чтобы можно было перегенерировать чанк
	
	// Очищаем воду при регенерации (опционально)
	if (waterData != nullptr && generated) {
		waterData->clear();
	}
	
	const int NX = CHUNK_SIZE_X;
	const int NY = CHUNK_SIZE_Y;
	const int NZ = CHUNK_SIZE_Z;
	const int SX = NX + 1;
	const int SY = NY + 1;
	const int SZ = NZ + 1;
	
	densityField.resize(SX * SY * SZ);
	
	// Параметры для мягкого порога плотности
	const float softness = 0.6f; // старт: 0.6, не больше 0.75
	const float isoLevel = 0.05f; // чуть положительный, чтобы поверхность не тонула
	
	// Вычисляем мировую позицию чанка
	int chunkWorldX = chunkPos.x * CHUNK_SIZE_X;
	int chunkWorldY = chunkPos.y * CHUNK_SIZE_Y;
	int chunkWorldZ = chunkPos.z * CHUNK_SIZE_Z;
	
	// ОПТИМИЗАЦИЯ: вычисляем высоту поверхности один раз на (x,z), а не в цикле по y
	// Это в ~32 раза быстрее, так как высота не зависит от y
	for (int z = 0; z < SZ; z++) {
		for (int x = 0; x < SX; x++) {
			// Мировые координаты точки (x, z)
			float wx = static_cast<float>(chunkWorldX + x);
			float wz = static_cast<float>(chunkWorldZ + z);
			
			// Вычисляем высоту поверхности один раз для этой точки (x, z)
			float surfaceHeight = evalSurfaceHeight(wx, wz);
			
			// Проставляем плотность для всех y по этой высоте с мягким порогом
			for (int y = 0; y < SY; y++) {
				float wy = static_cast<float>(chunkWorldY + y);
				float density = density_from_height(wy, surfaceHeight, isoLevel, softness);
				densityField[(y * SZ + z) * SX + x] = density;
			}
		}
	}
	
	// Удаляем старый меш перед созданием нового
	if (mesh != nullptr) {
		delete mesh;
		mesh = nullptr;
	}
	
	// Генерируем меш из поля плотности
	mesh = buildIsoSurface(densityField.data(), NX, NY, NZ, 0.0f);
	generated = true;
	
	// ДИАГНОСТИКА
	static int debugOptimizedCount = 0;
	if (debugOptimizedCount < 3) {
		std::cout << "[TERRAIN_GEN] Generated chunk (" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z 
		          << ") with OPTIMIZED terrain system (callback-based, soft density threshold)" << std::endl;
		debugOptimizedCount++;
	}
}

void MCChunk::generateWater(float waterLevel) {
	// Используем функцию с постоянным уровнем воды
	generateWater([waterLevel](int, int) { return waterLevel; });
}

void MCChunk::generateWater(std::function<float(int, int)> getWaterLevelFunc) {
	if (waterData == nullptr) {
		return;
	}
	
	// Вычисляем локальную позицию чанка в мире
	float chunkWorldY = chunkPos.y * CHUNK_SIZE_Y;
	int chunkWorldX = chunkPos.x * CHUNK_SIZE_X;
	int chunkWorldZ = chunkPos.z * CHUNK_SIZE_Z;
	
	// Генерируем воду для каждого вокселя
	for (int z = 0; z < CHUNK_SIZE_Z; z++) {
		for (int x = 0; x < CHUNK_SIZE_X; x++) {
			// Вычисляем мировые координаты
			int worldX = chunkWorldX + x;
			int worldZ = chunkWorldZ + z;
			
			// Получаем уровень воды для этой точки
			float localWaterLevel = getWaterLevelFunc(worldX, worldZ);
			
			// Если уровень воды не определен (суша), пропускаем
			if (localWaterLevel < -100.0f) {
				continue;
			}
			
			// Вычисляем локальный уровень воды относительно чанка
			int localWaterY = static_cast<int>(localWaterLevel - chunkWorldY);
			
			// Отладочный вывод для первых нескольких точек
			static int debugCount = 0;
			if (debugCount < 10) {
				std::cout << "[WATER] Generating water at (" << worldX << ", " << worldZ << ") level=" << localWaterLevel 
				          << " chunkY=" << chunkWorldY << " localY=" << localWaterY << std::endl;
				debugCount++;
			}
			
			// Определяем диапазон для заполнения водой
			// Если уровень воды выше чанка, заполняем весь чанк
			// Если уровень воды внутри чанка, заполняем до уровня воды
			int minY = 0;
			int maxY;
			if (localWaterY < 0) {
				// Уровень воды ниже чанка - не заполняем
				continue;
			} else if (localWaterY >= CHUNK_SIZE_Y) {
				// Уровень воды выше чанка - заполняем весь чанк
				maxY = CHUNK_SIZE_Y;
			} else {
				// Уровень воды внутри чанка - заполняем до уровня воды
				maxY = localWaterY + 1; // +1 чтобы заполнить и сам уровень воды
			}
			
			// Заполняем водой все воксели от низа до уровня воды
			int waterCount = 0;
			for (int y = minY; y < maxY && y < CHUNK_SIZE_Y; y++) {
				// Пропускаем твёрдое (проверяем плотность перед заливкой)
				if (isSolidLocal(x, y, z)) {
					continue;
				}
				
				voxel* vox = getVoxel(x, y, z);
				
				// Если воксель пустой (воздух), добавляем воду
				if (vox == nullptr || vox->id == 0) {
					// Вычисляем мировую Y координату
					float wy = chunkWorldY + y;
					
					// Если эта точка ниже или на уровне воды, заполняем водой
					if (wy <= localWaterLevel) {
						waterData->setVoxelMass(x, y, z, WaterUtils::WATER_MASS_MAX);
						waterData->setVoxelActive(x, y, z);
						waterCount++;
					}
				}
			}
			
			// Отладочный вывод для первых нескольких точек с водой
			static int debugWaterCount = 0;
			if (debugWaterCount < 5 && waterCount > 0) {
				std::cout << "[WATER] Added " << waterCount << " water voxels at (" << worldX << ", " << worldZ 
				          << ") level=" << localWaterLevel << std::endl;
				debugWaterCount++;
			}
		}
	}
	
	// Помечаем меш воды для пересборки
	waterMeshModified = true;
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

// Применение модификаций из WorldBuilder
void MCChunk::applyWorldBuilderModifications(
	const std::vector<uint8_t>& roadMap,
	const std::vector<float>& waterMap,
	int worldSize,
	void* prefabManagerPtr) {
	
	if (roadMap.empty() && waterMap.empty() && prefabManagerPtr == nullptr) {
		return; // Нет данных для применения
	}
	
	// Приводим prefabManager к нужному типу
	PrefabSystem::PrefabManager* prefabManager = static_cast<PrefabSystem::PrefabManager*>(prefabManagerPtr);
	
	// Вычисляем мировые координаты начала чанка
	int chunkWorldX = chunkPos.x * CHUNK_SIZE_X;
	int chunkWorldY = chunkPos.y * CHUNK_SIZE_Y;
	int chunkWorldZ = chunkPos.z * CHUNK_SIZE_Z;
	
	// Применяем дороги
	if (!roadMap.empty() && roadMap.size() == static_cast<size_t>(worldSize * worldSize)) {
		using namespace Pathfinding;
		
		// Проходим по всем вокселям чанка
		for (int z = 0; z < CHUNK_SIZE_Z; z++) {
			for (int x = 0; x < CHUNK_SIZE_X; x++) {
				int worldX = chunkWorldX + x;
				int worldZ = chunkWorldZ + z;
				
				// Проверяем границы мира
				if (worldX < 0 || worldX >= worldSize || worldZ < 0 || worldZ >= worldSize) {
					continue;
				}
				
				int mapIndex = worldX + worldZ * worldSize;
				uint8_t roadId = roadMap[mapIndex];
				
				if (roadId != PATH_FREE) {
					// Есть дорога - понижаем высоту и устанавливаем блок дороги
					// Находим поверхность (высоту террейна)
					float surfaceHeight = 0.0f;
					for (int y = CHUNK_SIZE_Y - 1; y >= 0; y--) {
						if (isSolidLocal(x, y, z)) {
							surfaceHeight = chunkWorldY + y + 1.0f;
							break;
						}
					}
					
					// Понижаем высоту на 2 блока для дороги
					int roadY = static_cast<int>(surfaceHeight - chunkWorldY) - 2;
					if (roadY >= 0 && roadY < CHUNK_SIZE_Y) {
						// Устанавливаем блок дороги (ID 5 = камень/асфальт)
						setVoxel(x, roadY, z, 5);
						
						// Заполняем дорогу до поверхности
						for (int y = roadY + 1; y < CHUNK_SIZE_Y && y < static_cast<int>(surfaceHeight - chunkWorldY); y++) {
							if (!isSolidLocal(x, y, z)) {
								setVoxel(x, y, z, 5);
							}
						}
					}
				}
			}
		}
	}
	
	// Применяем озера (модификация высоты и воды)
	if (!waterMap.empty() && waterMap.size() == static_cast<size_t>(worldSize * worldSize)) {
		for (int z = 0; z < CHUNK_SIZE_Z; z++) {
			for (int x = 0; x < CHUNK_SIZE_X; x++) {
				int worldX = chunkWorldX + x;
				int worldZ = chunkWorldZ + z;
				
				// Проверяем границы мира
				if (worldX < 0 || worldX >= worldSize || worldZ < 0 || worldZ >= worldSize) {
					continue;
				}
				
				int mapIndex = worldX + worldZ * worldSize;
				float waterLevel = waterMap[mapIndex];
				
				if (waterLevel > 0.0f) {
					// Есть озеро - понижаем высоту и заполняем водой
					// Находим текущую поверхность
					float currentSurfaceHeight = 0.0f;
					for (int y = CHUNK_SIZE_Y - 1; y >= 0; y--) {
						if (isSolidLocal(x, y, z)) {
							currentSurfaceHeight = chunkWorldY + y + 1.0f;
							break;
						}
					}
					
					// Понижаем поверхность на величину waterLevel
					float newSurfaceHeight = currentSurfaceHeight - waterLevel;
					int newSurfaceY = static_cast<int>(newSurfaceHeight - chunkWorldY);
					
					// Удаляем блоки выше новой поверхности
					for (int y = newSurfaceY + 1; y < CHUNK_SIZE_Y && y < static_cast<int>(currentSurfaceHeight - chunkWorldY); y++) {
						voxel* vox = getVoxel(x, y, z);
						if (vox != nullptr && vox->id != 0) {
							setVoxel(x, y, z, 0); // Удаляем блок
						}
					}
					
					// Заполняем водой до уровня waterLevel
					int waterY = static_cast<int>(waterLevel - chunkWorldY);
					if (waterY >= 0 && waterY < CHUNK_SIZE_Y) {
						for (int y = 0; y <= waterY && y < CHUNK_SIZE_Y; y++) {
							if (!isSolidLocal(x, y, z)) {
								waterData->setVoxelMass(x, y, z, WaterUtils::WATER_MASS_MAX);
								waterData->setVoxelActive(x, y, z);
							}
						}
					}
				}
			}
		}
	}
	
	// Применяем префабы
	if (prefabManager != nullptr) {
		// Получаем все префабы, которые пересекаются с этим чанком
		// (это нужно реализовать в PrefabManager)
		// Пока что просто проходим по всем префабам и проверяем пересечение
		
		// Вычисляем границы чанка в мировых координатах
		glm::ivec3 chunkMin(chunkWorldX, chunkWorldY, chunkWorldZ);
		glm::ivec3 chunkMax(chunkWorldX + CHUNK_SIZE_X, chunkWorldY + CHUNK_SIZE_Y, chunkWorldZ + CHUNK_SIZE_Z);
		
		// TODO: Реализовать получение префабов для чанка в PrefabManager
		// Пока что оставляем заглушку
	}
	
	// Помечаем чанк как измененный
	dirty = true;
	voxelMeshModified = true;
	waterMeshModified = true; // Помечаем для пересборки меша воды
}

