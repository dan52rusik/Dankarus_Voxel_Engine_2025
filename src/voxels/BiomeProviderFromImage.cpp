#include "BiomeProviderFromImage.h"
#include "../graphics/ImageData.h"
#include "../coders/png.h"
#include "../files/files.h"
#include <iostream>
#include <algorithm>
#include <cstring>

// Простая хеш-функция для строки (аналог GetStableHashCode из C#)
static int GetStableHashCode(const std::string& str) {
	int hash = 0;
	for (char c : str) {
		hash = ((hash << 5) - hash) + static_cast<int>(c);
		hash = hash & hash; // Преобразуем в 32-битное число
	}
	return hash;
}

BiomeProviderFromImage::BiomeProviderFromImage(const std::string& levelName, int worldSize)
	: worldName(levelName), worldSize(worldSize), worldSizeHalf(worldSize / 2),
	  biomeMapWidth(0), biomeMapHeight(0), biomesMapWidthHalf(0), biomesMapHeightHalf(0),
	  biomesScaleDiv(0), radiationMapSize(0), radiationMapScale(0),
	  splatW(0), splatScaleDiv(0), cntSplatChannels(0) {
}

BiomeProviderFromImage::~BiomeProviderFromImage() {
}

uint8_t BiomeProviderFromImage::processColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	// Определяем тип биома по цвету (упрощенная версия)
	// В оригинале используется более сложная логика с BiomeImageLoader
	// Здесь используем простую логику: каждый канал = определенный биом
	
	// Если все каналы 0 или 255 - это "пустой" биом
	if ((r == 0 && g == 0 && b == 0) || (r == 255 && g == 255 && b == 255)) {
		return 255; // Пустой биом
	}
	
	// Определяем доминирующий канал
	if (r >= g && r >= b) {
		return static_cast<uint8_t>(BiomeDefinition::BiomeType::Desert); // Красный = пустыня
	} else if (g >= r && g >= b) {
		return static_cast<uint8_t>(BiomeDefinition::BiomeType::Forest); // Зеленый = лес
	} else if (b >= r && b >= g) {
		return static_cast<uint8_t>(BiomeDefinition::BiomeType::Water); // Синий = вода
	}
	
	return static_cast<uint8_t>(BiomeDefinition::BiomeType::Plains); // По умолчанию
}

bool BiomeProviderFromImage::initData(const std::string& worldPath) {
	// Загружаем биомную карту из изображения
	std::string biomesPath = worldPath + "/biomes.png";
	if (!files::file_exists(biomesPath)) {
		biomesPath = worldPath + "/biomes.tga";
		if (!files::file_exists(biomesPath)) {
			std::cout << "[BIOME] Biome map not found: " << worldPath << "/biomes.png or .tga" << std::endl;
			return false;
		}
	}
	
	// Загружаем PNG (TGA пока не поддерживается, можно добавить позже)
	ImageData* biomesImage = png::load_image(biomesPath);
	if (biomesImage == nullptr) {
		std::cerr << "[BIOME] Failed to load biome map: " << biomesPath << std::endl;
		return false;
	}
	
	biomeMapWidth = biomesImage->getWidth();
	biomeMapHeight = biomesImage->getHeight();
	biomesMapWidthHalf = biomeMapWidth / 2;
	biomesMapHeightHalf = biomeMapHeight / 2;
	biomesScaleDiv = worldSize / biomeMapWidth;
	
	std::cout << "[BIOME] Biomes image size w=" << biomeMapWidth << ", h=" << biomeMapHeight << std::endl;
	
	// Конвертируем изображение в карту биомов
	biomeMap.resize(biomeMapWidth * biomeMapHeight);
	
	const unsigned char* pixels = static_cast<const unsigned char*>(biomesImage->getData());
	int channels = (biomesImage->getFormat() == ImageFormat::rgba8888) ? 4 : 3;
	
	for (int y = 0; y < biomeMapHeight; y++) {
		for (int x = 0; x < biomeMapWidth; x++) {
			int srcIndex = ((biomeMapHeight - 1 - y) * biomeMapWidth + x) * channels; // Переворачиваем Y
			uint8_t r = pixels[srcIndex];
			uint8_t g = (channels > 1) ? pixels[srcIndex + 1] : r;
			uint8_t b = (channels > 2) ? pixels[srcIndex + 2] : r;
			uint8_t a = (channels > 3) ? pixels[srcIndex + 3] : 255;
			
			biomeMap[y * biomeMapWidth + x] = processColor(r, g, b, a);
		}
	}
	
	delete biomesImage;
	
	// Инициализируем шум для подбиомов
	int seed = GetStableHashCode(worldName);
	noiseGen = std::make_unique<PerlinNoise>(seed);
	
	// Загружаем радиационную карту (опционально)
	std::string radiationPath = worldPath + "/radiation.png";
	if (!files::file_exists(radiationPath)) {
		radiationPath = worldPath + "/radiation.tga";
	}
	
	if (files::file_exists(radiationPath)) {
		ImageData* radiationImage = png::load_image(radiationPath);
		if (radiationImage != nullptr) {
			radiationMapSize = radiationImage->getWidth();
			radiationMapScale = worldSize / radiationMapSize;
			
			// Если карта маленькая (<=512x512), загружаем в память
			if (radiationMapSize <= 512 && radiationImage->getHeight() <= 512) {
				radiationMapSmall.resize(radiationMapSize * radiationImage->getHeight());
				
				const unsigned char* radPixels = static_cast<const unsigned char*>(radiationImage->getData());
				int radChannels = (radiationImage->getFormat() == ImageFormat::rgba8888) ? 4 : 3;
				
				for (int y = 0; y < radiationImage->getHeight(); y++) {
					for (int x = 0; x < radiationMapSize; x++) {
						int srcIndex = ((radiationImage->getHeight() - 1 - y) * radiationMapSize + x) * radChannels;
						uint8_t r = radPixels[srcIndex];
						uint8_t g = (radChannels > 1) ? radPixels[srcIndex + 1] : r;
						uint8_t b = (radChannels > 2) ? radPixels[srcIndex + 2] : r;
						
						// Обрабатываем цвет радиации (0=нет, 1=низкая, 2=средняя, 3=высокая)
						uint8_t radLevel = 0;
						if (g > 0) radLevel = 1;
						if (b > 0) radLevel = 2;
						if (r > 0) radLevel = 3;
						
						radiationMapSmall[y * radiationMapSize + x] = radLevel;
					}
				}
			}
			
			delete radiationImage;
		}
	}
	
	return true;
}

BiomeDefinition::BiomeType BiomeProviderFromImage::GetBiomeAt(int x, int z) {
	if (biomesScaleDiv == 0) {
		return BiomeDefinition::BiomeType::Any;
	}
	
	int _x = x / biomesScaleDiv + biomesMapWidthHalf;
	if (_x < 0 || _x >= biomeMapWidth) {
		return BiomeDefinition::BiomeType::Any;
	}
	
	int _y = z / biomesScaleDiv + biomesMapHeightHalf;
	if (_y < 0 || _y >= biomeMapHeight) {
		return BiomeDefinition::BiomeType::Any;
	}
	
	uint8_t id = biomeMap[_y * biomeMapWidth + _x];
	if (id == 255) {
		return BiomeDefinition::BiomeType::Any; // Пустой биом
	}
	
	return static_cast<BiomeDefinition::BiomeType>(id);
}

BiomeDefinition::BiomeType BiomeProviderFromImage::GetBiomeAt(int x, int z, float& intensity) {
	intensity = 1.0f;
	return GetBiomeAt(x, z);
}

BiomeDefinition::BiomeType BiomeProviderFromImage::GetBiomeOrSubAt(int x, int z) {
	// Упрощенная версия - просто возвращаем основной биом
	// Полная версия должна учитывать подбиомы через GetSubBiomeIdxAt
	return GetBiomeAt(x, z);
}

int BiomeProviderFromImage::GetSubBiomeIdxAt(BiomeDefinition::BiomeType biome, int x, int y, int z) {
	// Упрощенная версия - подбиомы пока не реализованы
	// В оригинале используется PerlinNoise::FBM для определения подбиома
	if (noiseGen == nullptr) {
		return -1;
	}
	
	// TODO: Реализовать логику подбиомов на основе шума
	// Пока возвращаем -1 (нет подбиома)
	return -1;
}

float BiomeProviderFromImage::GetRadiationAt(int x, int z) {
	if (radiationMapSmall.empty()) {
		return 0.0f;
	}
	
	int index = (x + worldSizeHalf) / radiationMapScale + (z + worldSizeHalf) / radiationMapScale * radiationMapSize;
	if (index >= 0 && index < static_cast<int>(radiationMapSmall.size())) {
		return static_cast<float>(radiationMapSmall[index]);
	}
	
	return 0.0f;
}

