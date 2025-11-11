#ifndef GRAPHICS_MARCHINGCUBES_H_
#define GRAPHICS_MARCHINGCUBES_H_

#include <cstddef>
#include <cstdint>

class Mesh;

// Константы плотности (для стандартизации значений)
// Используются sbyte значения из 7 Days To Die для совместимости
namespace MarchingCubes {
	// Плотность воздуха (положительные значения)
	static constexpr int8_t DensityAir = 127;      // Максимальная плотность воздуха
	static constexpr int8_t DensityAirHi = 100;    // Высокая плотность воздуха
	
	// Плотность террейна (отрицательные значения)
	static constexpr int8_t DensityTerrain = -128; // Минимальная плотность террейна
	static constexpr int8_t DensityTerrainHi = -100; // Высокая плотность террейна
	
	// Конвертация float плотности в sbyte (для совместимости с константами)
	// В нашем коде: положительная density = террейн (земля), отрицательная = воздух
	// В 7DTD sbyte: положительные = воздух, отрицательные = террейн
	// Поэтому инвертируем знак
	inline int8_t densityToSByte(float density) {
		// Инвертируем: наша положительная density (террейн) -> отрицательный sbyte
		// Наша отрицательная density (воздух) -> положительный sbyte
		float inverted = -density;
		
		// Ограничиваем и конвертируем в sbyte
		if (inverted > 127.0f) {
			return DensityAir;
		} else if (inverted > 100.0f) {
			return DensityAirHi;
		} else if (inverted < -128.0f) {
			return DensityTerrain;
		} else if (inverted < -100.0f) {
			return DensityTerrainHi;
		} else {
			return static_cast<int8_t>(inverted);
		}
	}
	
	// Получить offset Y для размещения декораций на поверхности
	// Используется для размещения травы, камней и других объектов на поверхности террейна
	// _densY - плотность в текущей точке Y
	// _densYm1 - плотность в точке Y-1 (выше)
	// Возвращает offset по Y для размещения декорации
	inline float GetDecorationOffsetY(int8_t densY, int8_t densYm1) {
		// Формула из 7 Days To Die: -0.0035 * (densY + densYm1)
		// Ограничиваем диапазон от -0.4 до 0.4
		float offset = -0.0035f * static_cast<float>(static_cast<int>(densY) + static_cast<int>(densYm1));
		
		// Clamp к диапазону [-0.4, 0.4]
		if (offset < -0.4f) return -0.4f;
		if (offset > 0.4f) return 0.4f;
		return offset;
	}
	
	// Перегрузка для float плотности (удобнее для использования)
	inline float GetDecorationOffsetY(float densY, float densYm1) {
		int8_t sbyteY = densityToSByte(densY);
		int8_t sbyteYm1 = densityToSByte(densYm1);
		return GetDecorationOffsetY(sbyteY, sbyteYm1);
	}
}

// density: scalar field sized (nx+1)*(ny+1)*(nz+1) in x-fastest order
// isoLevel: threshold for isosurface extraction
Mesh* buildIsoSurface(const float* density, int nx, int ny, int nz, float isoLevel);

#endif /* GRAPHICS_MARCHINGCUBES_H_ */

