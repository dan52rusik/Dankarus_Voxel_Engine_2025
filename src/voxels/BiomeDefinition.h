#ifndef VOXELS_BIOMEDEFINITION_H_
#define VOXELS_BIOMEDEFINITION_H_

#include <string>
#include <cstdint>
#include <glm/glm.hpp>

// Forward declaration (OpenSimplex3D находится в глобальном namespace)
class OpenSimplex3D;

// Система биомов
// Адаптировано из 7 Days To Die BiomeDefinition
namespace BiomeDefinition {

	// Типы биомов
	enum class BiomeType : uint8_t {
		Any = 0,
		Snow = 1,
		Forest = 2,
		PineForest = 3,
		Plains = 4,
		Desert = 5,
		Water = 6,
		Radiated = 7,
		Wasteland = 8,
		BurntForest = 9,
		City = 10,
		CityWasteland = 11,
		WastelandHub = 12,
		CaveFloor = 13,
		CaveCeiling = 14
	};

	// Цвета биомов (RGB, 24-bit)
	static constexpr uint32_t BiomeColors[15] = {
		0x000000,        // Any - черный
		0xFFFFFF,        // Snow - белый
		0x000000,        // Forest - черный
		0x004000,        // PineForest - темно-зеленый
		0x000000,        // Plains - черный
		0xFFD757,        // Desert - желтый
		0x0063FF,        // Water - синий
		0x000000,        // Radiated - черный
		0xFF7800,        // Wasteland - оранжевый
		0xBA00FF,        // BurntForest - фиолетовый
		0x808080,        // City - серый
		0xC0C0C0,        // CityWasteland - светло-серый
		0xA0A0A0,        // WastelandHub - серый
		0x000000,        // CaveFloor - черный
		0x000000         // CaveCeiling - черный
	};

	// Имена биомов
	static constexpr const char* BiomeNames[15] = {
		"any",
		"snow",
		"forest",
		"pine_forest",
		"plains",
		"desert",
		"water",
		"radiated",
		"wasteland",
		"burnt_forest",
		"city",
		"city_wasteland",
		"wasteland_hub",
		"caveFloor",
		"caveCeiling"
	};

	// Определение биома
	class Biome {
	public:
		BiomeType type;
		std::string name;
		uint32_t color;
		int radiationLevel;
		int difficulty;
		
		// Параметры шума для определения биома
		float noiseFreq;
		float noiseMin;
		float noiseMax;
		glm::vec2 noiseOffset;
		
		Biome(BiomeType _type, const std::string& _name, uint32_t _color, 
		      int _radiationLevel = 0, int _difficulty = 1,
		      float _noiseFreq = 0.03f, float _noiseMin = 0.2f, float _noiseMax = 1.0f,
		      const glm::vec2& _noiseOffset = glm::vec2(0.0f))
			: type(_type), name(_name), color(_color), radiationLevel(_radiationLevel),
			  difficulty(_difficulty), noiseFreq(_noiseFreq), noiseMin(_noiseMin),
			  noiseMax(_noiseMax), noiseOffset(_noiseOffset) {}
		
		// Получить цвет биома
		static uint32_t GetBiomeColor(BiomeType type) {
			return BiomeColors[static_cast<int>(type)];
		}
	};
	
	// Получить биом по типу
	BiomeType GetBiomeAt(float wx, float wz, OpenSimplex3D& noise);
	
	// Получить цвет биома в точке
	uint32_t GetBiomeColorAt(float wx, float wz, OpenSimplex3D& noise);
	
	// Получить имя биома
	const char* GetBiomeName(BiomeType type);
}

#endif /* VOXELS_BIOMEDEFINITION_H_ */

