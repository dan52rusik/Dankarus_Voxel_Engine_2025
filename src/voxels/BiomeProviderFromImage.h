#ifndef VOXELS_BIOMEPROVIDERFROMIMAGE_H_
#define VOXELS_BIOMEPROVIDERFROMIMAGE_H_

#include "BiomeDefinition.h"
#include "noise/PerlinNoise.h"
#include <string>
#include <vector>
#include <memory>

// WorldBiomeProviderFromImage - провайдер биомов из изображения
// Адаптировано из 7 Days To Die WorldBiomeProviderFromImage
class BiomeProviderFromImage {
private:
	// Сжатая карта биомов (GridCompressedData<byte>)
	// Упрощенная версия: используем простой 2D массив
	std::vector<uint8_t> biomeMap;
	int biomeMapWidth;
	int biomeMapHeight;
	int biomesMapWidthHalf;
	int biomesMapHeightHalf;
	int biomesScaleDiv; // worldSize / biomeMapWidth
	
	// Шум для подбиомов
	std::unique_ptr<PerlinNoise> noiseGen;
	
	// Параметры мира
	std::string worldName;
	int worldSize;
	int worldSizeHalf;
	
	// Радиационная карта (упрощенная версия)
	std::vector<uint8_t> radiationMapSmall;
	int radiationMapSize;
	int radiationMapScale;
	
	// Splat maps (для текстур поверхности)
	std::vector<uint8_t> splatMapMaxValue;
	int splatW;
	int splatScaleDiv;
	int cntSplatChannels;
	
	// Вспомогательные функции
	uint8_t processColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	
public:
	BiomeProviderFromImage(const std::string& levelName, int worldSize = 4096);
	~BiomeProviderFromImage();
	
	// Инициализация (загрузка биомной карты из изображения)
	bool initData(const std::string& worldPath);
	
	// Получить биом в точке (x, z)
	BiomeDefinition::BiomeType GetBiomeAt(int x, int z);
	
	// Получить биом с интенсивностью (перегрузка)
	BiomeDefinition::BiomeType GetBiomeAt(int x, int z, float& intensity);
	
	// Получить подбиом (subbiome) в точке
	BiomeDefinition::BiomeType GetBiomeOrSubAt(int x, int z);
	
	// Получить индекс подбиома
	int GetSubBiomeIdxAt(BiomeDefinition::BiomeType biome, int x, int y, int z);
	
	// Получить радиацию в точке
	float GetRadiationAt(int x, int z);
	
	// Получить влажность в точке
	float GetHumidityAt(int x, int z) { return 0.0f; } // Пока не реализовано
	
	// Получить температуру в точке
	float GetTemperatureAt(int x, int z) { return 0.0f; } // Пока не реализовано
	
	// Получить размер карты биомов
	glm::ivec2 GetSize() const { return glm::ivec2(biomeMapWidth, biomeMapHeight); }
	
	// Получить имя мира
	std::string GetWorldName() const { return worldName; }
};

#endif /* VOXELS_BIOMEPROVIDERFROMIMAGE_H_ */

