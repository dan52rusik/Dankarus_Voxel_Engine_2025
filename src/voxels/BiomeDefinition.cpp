#include "BiomeDefinition.h"
#include "../noise/OpenSimplex.h"
#include <algorithm>
#include <cmath>

namespace BiomeDefinition {

	// Получить биом в точке (wx, wz) на основе шума
	BiomeType GetBiomeAt(float wx, float wz, OpenSimplex3D& noise) {
		// Используем несколько слоев шума для определения биома
		// Базовый шум для крупных регионов
		float baseNoise = noise.fbm(wx * 0.001f, 0.0f, wz * 0.001f, 3, 2.0f, 0.5f);
		baseNoise = (baseNoise + 1.0f) * 0.5f; // Нормализуем в [0..1]
		
		// Детальный шум для границ биомов
		float detailNoise = noise.fbm(wx * 0.005f + 1000.0f, 0.0f, wz * 0.005f + 2000.0f, 2, 2.0f, 0.5f);
		detailNoise = (detailNoise + 1.0f) * 0.5f;
		
		// Комбинируем шумы
		float combined = baseNoise * 0.7f + detailNoise * 0.3f;
		
		// Определяем биом на основе комбинированного значения
		// Распределение биомов:
		// 0.0-0.15: Snow (снег)
		// 0.15-0.30: Forest (лес)
		// 0.30-0.45: PineForest (хвойный лес)
		// 0.45-0.60: Plains (равнины)
		// 0.60-0.75: Desert (пустыня)
		// 0.75-0.85: Wasteland (пустошь)
		// 0.85-0.95: BurntForest (выжженный лес)
		// 0.95-1.0: Radiated (радиация)
		
		if (combined < 0.15f) {
			return BiomeType::Snow;
		} else if (combined < 0.30f) {
			return BiomeType::Forest;
		} else if (combined < 0.45f) {
			return BiomeType::PineForest;
		} else if (combined < 0.60f) {
			return BiomeType::Plains;
		} else if (combined < 0.75f) {
			return BiomeType::Desert;
		} else if (combined < 0.85f) {
			return BiomeType::Wasteland;
		} else if (combined < 0.95f) {
			return BiomeType::BurntForest;
		} else {
			return BiomeType::Radiated;
		}
	}
	
	// Получить цвет биома в точке
	uint32_t GetBiomeColorAt(float wx, float wz, OpenSimplex3D& noise) {
		BiomeType biome = GetBiomeAt(wx, wz, noise);
		return BiomeColors[static_cast<int>(biome)];
	}
	
	// Получить имя биома
	const char* GetBiomeName(BiomeType type) {
		return BiomeNames[static_cast<int>(type)];
	}
}

