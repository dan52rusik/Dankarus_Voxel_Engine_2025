#include "BiomeDefinition.h"
#include "../noise/OpenSimplex.h"
#include <algorithm>
#include <cmath>

namespace BiomeDefinition {

	// Получить биом в точке (wx, wz) на основе шума, высоты и влажности
	BiomeType GetBiomeAt(float wx, float wz, OpenSimplex3D& noise) {
		// Используем несколько слоев шума для определения биома
		// Базовый шум для крупных регионов (температура)
		float temperatureNoise = noise.fbm(wx * 0.0008f, 0.0f, wz * 0.0008f, 4, 2.0f, 0.5f);
		temperatureNoise = (temperatureNoise + 1.0f) * 0.5f; // Нормализуем в [0..1]
		
		// Влажность (отдельный шум)
		float humidityNoise = noise.fbm(wx * 0.0012f + 5000.0f, 0.0f, wz * 0.0012f + 5000.0f, 3, 2.0f, 0.5f);
		humidityNoise = (humidityNoise + 1.0f) * 0.5f; // [0..1]
		
		// Детальный шум для границ биомов (более плавные переходы)
		float detailNoise = noise.fbm(wx * 0.008f + 10000.0f, 0.0f, wz * 0.008f + 10000.0f, 2, 2.0f, 0.5f);
		detailNoise = (detailNoise + 1.0f) * 0.5f;
		
		// Комбинируем шумы с учетом деталей
		float temperature = temperatureNoise * 0.8f + detailNoise * 0.2f;
		float humidity = humidityNoise * 0.8f + detailNoise * 0.2f;
		
		// Определяем биом на основе температуры и влажности
		// Используем классификацию по климату:
		// 
		// Холодные биомы (низкая температура):
		if (temperature < 0.25f) {
			if (humidity > 0.5f) {
				return BiomeType::Snow; // Снег в холодных влажных регионах
			} else {
				return BiomeType::PineForest; // Хвойный лес в холодных сухих регионах
			}
		}
		// Умеренные биомы (средняя температура):
		else if (temperature < 0.6f) {
			if (humidity > 0.7f) {
				return BiomeType::Forest; // Лес в умеренных влажных регионах
			} else if (humidity > 0.4f) {
				return BiomeType::Plains; // Равнины в умеренных регионах
			} else {
				return BiomeType::PineForest; // Хвойный лес в умеренных сухих регионах
			}
		}
		// Жаркие биомы (высокая температура):
		else {
			if (humidity < 0.3f) {
				return BiomeType::Desert; // Пустыня в жарких сухих регионах
			} else if (humidity < 0.6f) {
				return BiomeType::Wasteland; // Пустошь в жарких умеренно сухих регионах
			} else {
				// В жарких влажных регионах - редкие биомы
				if (temperature > 0.85f && humidity > 0.75f) {
					return BiomeType::Radiated; // Радиация в экстремальных условиях
				} else if (temperature > 0.75f) {
					return BiomeType::BurntForest; // Выжженный лес
				} else {
					return BiomeType::Forest; // Лес в жарких влажных регионах
				}
			}
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

