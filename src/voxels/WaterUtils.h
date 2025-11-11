#ifndef VOXELS_WATERUTILS_H_
#define VOXELS_WATERUTILS_H_

#include <glm/glm.hpp>
#include <cstdint>

// Утилиты для работы с водой
// Адаптировано из 7 Days To Die WaterUtils
namespace WaterUtils {
	
	// Константы для воды
	static constexpr int WATER_MASS_MAX = 255;      // Максимальная масса воды
	static constexpr int WATER_MASS_ACTIVE = 195;   // Порог для активной воды (выше этого значения вода активна)
	static constexpr int WATER_MASS_MIN = 0;        // Минимальная масса (нет воды)
	
	// Получить уровень воды из массы (0 или 1)
	inline int GetWaterLevel(int mass) {
		return (mass > WATER_MASS_ACTIVE) ? 1 : 0;
	}
	
	// Проверить, находится ли воксель вне чанка (для соседей)
	// Адаптировано для размеров чанка 32x32x32
	template<int CHUNK_SIZE_X, int CHUNK_SIZE_Z>
	inline bool IsVoxelOutsideChunk(int neighborX, int neighborZ) {
		return neighborX < 0 || neighborX >= CHUNK_SIZE_X || neighborZ < 0 || neighborZ >= CHUNK_SIZE_Z;
	}
	
	// Перегрузка для стандартных размеров (32x32x32)
	inline bool IsVoxelOutsideChunk(int neighborX, int neighborZ) {
		return IsVoxelOutsideChunk<32, 32>(neighborX, neighborZ);
	}
	
	// Вычислить ключ вокселя для 2D (для хеширования)
	inline int GetVoxelKey2D(int x, int z) {
		return x * 8976890 + z * 981131;
	}
	
	// Вычислить ключ вокселя для 3D
	inline int GetVoxelKey(int x, int y, int z = 0) {
		return x * 8976890 + y * 981131 + z;
	}
	
	// Получить индекс вокселя в чанке (x, y, z в локальных координатах чанка)
	// Адаптировано для размеров чанка 32x32x32 (вместо 16x256x16 в 7DTD)
	// Формула: x + y * CHUNK_SIZE_X + z * CHUNK_SIZE_X * CHUNK_SIZE_Y
	// Или: (y * CHUNK_SIZE_Z + z) * CHUNK_SIZE_X + x (стандартная индексация)
	template<int CHUNK_SIZE_X, int CHUNK_SIZE_Y, int CHUNK_SIZE_Z>
	inline int GetVoxelIndex(int x, int y, int z) {
		return (y * CHUNK_SIZE_Z + z) * CHUNK_SIZE_X + x;
	}
	
	// Получить координаты из индекса вокселя
	template<int CHUNK_SIZE_X, int CHUNK_SIZE_Y, int CHUNK_SIZE_Z>
	inline glm::ivec3 GetVoxelCoords(int index) {
		glm::ivec3 coords;
		int volumeXY = CHUNK_SIZE_X * CHUNK_SIZE_Z;
		coords.y = index / volumeXY;
		int remainder = index % volumeXY;
		coords.z = remainder / CHUNK_SIZE_X;
		coords.x = remainder % CHUNK_SIZE_X;
		return coords;
	}
	
	// Получить Y координату из индекса
	template<int CHUNK_SIZE_X, int CHUNK_SIZE_Y, int CHUNK_SIZE_Z>
	inline int GetVoxelY(int index) {
		return index / (CHUNK_SIZE_X * CHUNK_SIZE_Z);
	}
	
} // namespace WaterUtils

#endif /* VOXELS_WATERUTILS_H_ */

