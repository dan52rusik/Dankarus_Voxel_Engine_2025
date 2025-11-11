#ifndef VOXELS_MCCHUNK_H_
#define VOXELS_MCCHUNK_H_

#include "graphics/Mesh.h"
#include "noise/OpenSimplex.h"
#include "voxel.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

// Forward declaration
class WaterData;

class MCChunk {
public:
	glm::ivec3 chunkPos; // Позиция чанка в координатах чанков (не в мире)
	glm::vec3 worldPos;  // Позиция чанка в мире (центр чанка)
	Mesh* mesh; // Меш для Marching Cubes (поверхность земли)
	Mesh* voxelMesh; // Меш для воксельных блоков
	bool generated;
	bool voxelMeshModified; // Флаг для пересборки меша вокселей
	bool dirty; // Флаг для отслеживания изменений чанка (для сохранения)
	
	// Параметры генерации
	static const int CHUNK_SIZE_X = 32;
	static const int CHUNK_SIZE_Y = 32;
	static const int CHUNK_SIZE_Z = 32;
	static const int CHUNK_VOL = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
	
	MCChunk(int cx, int cy, int cz);
	~MCChunk();
	
	void generate(OpenSimplex3D& noise, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation);
	
	// Оптимизированная генерация с использованием callback для вычисления высоты поверхности
	// Вычисляет высоту один раз на (x,z), а не в цикле по y - в ~32 раза быстрее
	void generate(std::function<float(float, float)> evalSurfaceHeight);
	
	// Система воксельных блоков
	voxel* voxels; // Массив вокселей для блоков
	voxel* getVoxel(int lx, int ly, int lz); // Получить воксель по локальным координатам
	void setVoxel(int lx, int ly, int lz, uint8_t id); // Установить воксель
	
	// Получить плотность в точке (для raycast по поверхности)
	float getDensity(const glm::vec3& worldPos) const;
	
	// Система воды
	WaterData* waterData; // Данные о воде в чанке
	WaterData* getWaterData() { return waterData; }
	const WaterData* getWaterData() const { return waterData; }
	
	// Генерация воды в чанке
	// waterLevel - уровень воды в мировых координатах (например, 10.0f)
	// или функция для получения уровня воды в точке (worldX, worldZ)
	void generateWater(float waterLevel);
	void generateWater(std::function<float(int, int)> getWaterLevelFunc);
	
	// Проверка, является ли воксель твёрдым (на основе densityField)
	// Используется для предотвращения заливки воды в грунт
	// densityField имеет размеры (CHUNK_SIZE_X+1) x (CHUNK_SIZE_Y+1) x (CHUNK_SIZE_Z+1)
	// Для вокселя (lx, ly, lz) проверяем плотность в точке (lx, ly, lz) densityField
	inline bool isSolidLocal(int lx, int ly, int lz) const {
		// Проверяем границы для densityField (0..CHUNK_SIZE_X включительно)
		if (lx < 0 || lx > CHUNK_SIZE_X ||
		    ly < 0 || ly > CHUNK_SIZE_Y ||
		    lz < 0 || lz > CHUNK_SIZE_Z) {
			return false;
		}
		if (!generated || densityField.empty()) {
			return false;
		}
		const int SX = CHUNK_SIZE_X + 1;
		const int SY = CHUNK_SIZE_Y + 1;
		const int SZ = CHUNK_SIZE_Z + 1;
		// Индексация densityField: (y * SZ + z) * SX + x
		int idx = (ly * SZ + lz) * SX + lx;
		if (idx < 0 || idx >= static_cast<int>(densityField.size())) {
			return false;
		}
		float d = densityField[idx];
		// Смягчаем проверку: если плотность чуть отрицательная (около изосерфейса),
		// всё равно считаем твёрдым, чтобы вода не просачивалась
		return d > -0.05f; // "земля" (почти положительная плотность, с небольшим запасом)
	}
	
private:
	std::vector<float> densityField;
};

#endif /* VOXELS_MCCHUNK_H_ */

