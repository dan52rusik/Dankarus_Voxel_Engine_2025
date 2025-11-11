#ifndef VOXELS_WORLDDATALOADER_H_
#define VOXELS_WORLDDATALOADER_H_

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Структуры данных мира 7DTD
// Адаптировано из формата Navezgane

// Информация о карте
struct MapInfo {
	std::string name;
	std::vector<std::string> modes;
	std::string description;
	glm::ivec2 heightMapSize; // Размер карты высот (например, 6144x6144)
	
	MapInfo() : heightMapSize(0, 0) {}
};

// Точка спавна
struct SpawnPoint {
	glm::vec3 position;
	glm::vec3 rotation; // В градусах (yaw, pitch, roll)
	
	SpawnPoint() : position(0.0f), rotation(0.0f) {}
	SpawnPoint(const glm::vec3& pos, const glm::vec3& rot) : position(pos), rotation(rot) {}
};

// Источник воды
struct WaterSource {
	glm::vec3 position;
	float minZ; // Минимальная Z координата (опционально)
	float maxZ; // Максимальная Z координата (опционально)
	bool hasMinZ;
	bool hasMaxZ;
	
	WaterSource() : position(0.0f), minZ(0.0f), maxZ(0.0f), hasMinZ(false), hasMaxZ(false) {}
	WaterSource(const glm::vec3& pos) : position(pos), minZ(0.0f), maxZ(0.0f), hasMinZ(false), hasMaxZ(false) {}
};

// Префаб/декорация
struct PrefabInstance {
	std::string type;      // "model" или другой тип
	std::string name;      // Имя префаба
	glm::vec3 position;
	int rotation;           // Поворот (0-3, соответствует 0°, 90°, 180°, 270°)
	bool yIsGroundLevel;   // Выравнивание по уровню земли
	
	PrefabInstance() : position(0.0f), rotation(0), yIsGroundLevel(true) {}
};

// Загрузчик данных мира 7DTD
// Адаптировано из формата Navezgane
class WorldDataLoader {
public:
	WorldDataLoader();
	~WorldDataLoader();
	
	// Загрузка map_info.xml
	bool LoadMapInfo(const std::string& filePath, MapInfo& mapInfo);
	
	// Загрузка spawnpoints.xml
	bool LoadSpawnPoints(const std::string& filePath, std::vector<SpawnPoint>& spawnPoints);
	
	// Загрузка water_info.xml
	bool LoadWaterSources(const std::string& filePath, std::vector<WaterSource>& waterSources);
	
	// Загрузка prefabs.xml
	bool LoadPrefabs(const std::string& filePath, std::vector<PrefabInstance>& prefabs);
	
	// Загрузка карты высот (DTM - Digital Terrain Model)
	// Формат: 16-bit RAW, размер определяется из map_info.xml
	// Возвращает массив высот (размер: width * height)
	// Высоты в диапазоне 0-65535, обычно масштабируются
	bool LoadHeightMap(const std::string& filePath, int width, int height, std::vector<uint16_t>& heights);
	
	// Загрузка карты биомов (biomes.png)
	// Возвращает массив индексов биомов (размер: width * height)
	// Каждый пиксель соответствует индексу биома
	bool LoadBiomeMap(const std::string& filePath, int& width, int& height, std::vector<uint8_t>& biomes);
	
	// Загрузка splat карт (текстуры материалов)
	// splat1-4 - разные слои материалов
	// Возвращает RGBA данные
	bool LoadSplatMap(const std::string& filePath, int& width, int& height, std::vector<uint8_t>& rgbaData);
	
	// Вспомогательные функции для парсинга XML
	static bool ParseVector3(const std::string& str, glm::vec3& vec);
	static bool ParseVector2(const std::string& str, glm::ivec2& vec);
	static bool ParseInt(const std::string& str, int& value);
	static bool ParseFloat(const std::string& str, float& value);
	
private:
	// Вспомогательные функции для парсинга XML
	std::string ExtractAttribute(const std::string& line, const std::string& attrName);
	std::string Trim(const std::string& str);
};

#endif /* VOXELS_WORLDDATALOADER_H_ */

