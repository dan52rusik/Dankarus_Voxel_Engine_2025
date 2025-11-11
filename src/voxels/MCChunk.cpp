#include "MCChunk.h"
#include "graphics/MarchingCubes.h"
#include "WaterData.h"
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

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

// террасы (опционально, даёт «ступеньки» скал). intensity ~ [0..1]
static inline float terrace(float h, float steps = 6.0f, float intensity = 0.35f) {
	float t = std::floor(h * steps) / steps;
	return glm::mix(h, t, intensity);
}

MCChunk::MCChunk(int cx, int cy, int cz) 
	: chunkPos(cx, cy, cz), mesh(nullptr), voxelMesh(nullptr), generated(false), voxelMeshModified(true), dirty(false), waterData(nullptr) {
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
	if (voxels != nullptr) {
		delete[] voxels;
	}
	if (waterData != nullptr) {
		delete waterData;
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
	
	// Помечаем чанк как измененный для сохранения
	dirty = true;
	voxels[index].id = id;
	voxelMeshModified = true;
}

void MCChunk::generate(OpenSimplex3D& noise, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation) {
	// ВАЖНО: не проверяем generated здесь, чтобы можно было перегенерировать чанк
	// (например, при изменении параметров генерации)
	// if (generated) {
	// 	return;
	// }
	
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
				
				// Вычисляем высоту поверхности в точке (x, z) используя улучшенную систему генерации
				// ВАЖНО: используем те же мировые координаты, что и в соседних чанках
				
				// 1) Domain warping — «кривим» входные координаты, чтобы исчезла регулярность FBM
				float w1 = noise.fbm(wx * 0.008f, 0.0f, wz * 0.008f, 3, 2.0f, 0.5f);
				float w2 = noise.fbm(wx * 0.008f + 100.0f, 0.0f, wz * 0.008f + 100.0f, 3, 2.0f, 0.5f);
				float warpAmp = 35.0f; // насколько «гнём» координаты (в мировых юнитах)
				float wxw = wx + w1 * warpAmp;
				float wzw = wz + w2 * warpAmp;
				
				// 2) Континентальность (очень низкая частота) — задаёт большие области суши/впадин
				float continents = remap01(noise.fbm(wx * 0.0015f, 0.0f, wz * 0.0015f, 2, 2.0f, 0.5f));
				// «поднимаем» материки и немного притапливаем впадины
				float continentBias = glm::mix(-0.6f, 0.6f, continents);  // [-0.6..0.6]
				
				// 3) Базовый FBM (мягкие холмы) — уже по варпнутым координатам
				// Используем baseFreq для масштабирования частот
				float hills = noise.fbm(wxw * baseFreq, 0.0f, wzw * baseFreq, 5, 2.0f, 0.5f); // -1..1
				
				// 4) Средне-высокочастотная полоса (для дополнительного разнообразия)
				float midNoise = noise.fbm(wxw * baseFreq * 1.5f, 0.0f, wzw * baseFreq * 1.5f, 3, 2.0f, 0.5f) * 0.4f; // -1..1
				
				// 5) Ридж-мультифрактал (острые хребты/скалы) - более заметные горы
				float ridgesFBM = noise.fbm(wxw * baseFreq * 0.5f, 0.0f, wzw * baseFreq * 0.5f, 4, 2.0f, 0.5f); // -1..1
				float ridges = ridge(ridgesFBM);  // 0..1
				// Делаем горы более заметными
				ridges = std::max(0.0f, ridges - 0.25f) * 1.2f; // Усиливаем только высокие значения
				
				// 6) Детальки (мелкие формы) - усиленные для заметности внутри чанка
				float details = noise.fbm(wxw * baseFreq * 3.5f, 0.0f, wzw * baseFreq * 3.5f, 3, 2.0f, 0.5f) * 0.6f; // -1..1
				
				// 7) Склеиваем: материки -> холмы -> средние -> хребты -> детали.
				//    Вес хребтов растёт на «краях континентов» (там интереснее рельеф)
				float ridgeWeight = glm::smoothstep(0.25f, 0.85f, continents); // 0..1
				float combined =
					hills * 0.65f
					+ midNoise                                    // Средне-высокочастотная полоса
					+ (ridges * 2.0f - 1.0f) * (0.9f * ridgeWeight)   // Горы с большим весом
					+ details                                     // Усиленные детали
					+ continentBias;                              // большой макро-тренд
				
				// 8) Курация амплитуды и опциональные «террасы»
				combined = glm::clamp(combined, -1.2f, 1.2f);
				float shaped = terrace(remap01(combined), 7.0f, 0.25f);    // 0..1 с лёгкими ступенями
				shaped = shaped * 2.0f - 1.0f;                             // обратно в [-1..1]
				
				// 9) Высота поверхности
				float surfaceHeight = baseHeight + shaped * heightVariation;
				
				// 10) Плотность (как и раньше)
				float density = surfaceHeight - wy;
				
				densityField[(y * SZ + z) * SX + x] = density;
				
				// Отладочный вывод для первых нескольких точек (чтобы проверить параметры)
				static int debugTerrainCount = 0;
				if (debugTerrainCount < 5 && x == 0 && z == 0) {
					std::cout << "[TERRAIN] baseHeight=" << baseHeight 
					          << " heightVariation=" << heightVariation
					          << " continents=" << continents
					          << " combined=" << combined
					          << " shaped=" << shaped
					          << " surfaceHeight=" << surfaceHeight << std::endl;
					debugTerrainCount++;
				}
			}
		}
	}
	
	// Удаляем старый меш перед созданием нового (исправление утечки памяти)
	if (mesh != nullptr) {
		delete mesh;
		mesh = nullptr;
	}
	
	// Генерируем меш из поля плотности
	mesh = buildIsoSurface(densityField.data(), NX, NY, NZ, 0.0f);
	generated = true;
	
	// ДИАГНОСТИКА: выводим информацию о генерации
	static int debugGenerateCount = 0;
	if (debugGenerateCount < 3) {
		std::cout << "[TERRAIN_GEN] Generated chunk (" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z 
		          << ") with NEW terrain system (domain warping, continents, ridges, terraces)" << std::endl;
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
			
			// Проставляем плотность для всех y по этой высоте
			for (int y = 0; y < SY; y++) {
				float wy = static_cast<float>(chunkWorldY + y);
				float density = surfaceHeight - wy;
				densityField[(y * SZ + z) * SX + x] = density;
			}
		}
	}
	
	// Удаляем старый меш перед созданием нового (исправление утечки памяти)
	if (mesh != nullptr) {
		delete mesh;
		mesh = nullptr;
	}
	
	// Генерируем меш из поля плотности
	mesh = buildIsoSurface(densityField.data(), NX, NY, NZ, 0.0f);
	generated = true;
	
	// ДИАГНОСТИКА: выводим информацию о генерации
	static int debugOptimizedCount = 0;
	if (debugOptimizedCount < 3) {
		std::cout << "[TERRAIN_GEN] Generated chunk (" << chunkPos.x << "," << chunkPos.y << "," << chunkPos.z 
		          << ") with OPTIMIZED terrain system (callback-based, ~32x faster)" << std::endl;
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

