#include "WorldDataLoader.h"
#include "../files/files.h"
#include "../coders/png.h"
#include "../graphics/ImageData.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#endif

WorldDataLoader::WorldDataLoader() {
}

WorldDataLoader::~WorldDataLoader() {
}

bool WorldDataLoader::LoadMapInfo(const std::string& filePath, MapInfo& mapInfo) {
	std::string content = files::read_string(filePath);
	if (content.empty()) {
		std::cerr << "[WorldDataLoader] Failed to read map_info.xml: " << filePath << std::endl;
		return false;
	}
	
	std::istringstream iss(content);
	std::string line;
	
	while (std::getline(iss, line)) {
		line = Trim(line);
		
		if (line.find("<property name=\"Name\"") != std::string::npos) {
			mapInfo.name = ExtractAttribute(line, "value");
			// Убираем :: с начала и конца
			if (mapInfo.name.size() >= 4 && mapInfo.name.substr(0, 2) == "::" && mapInfo.name.substr(mapInfo.name.size() - 2) == "::") {
				mapInfo.name = mapInfo.name.substr(2, mapInfo.name.size() - 4);
			}
		} else if (line.find("<property name=\"Modes\"") != std::string::npos) {
			std::string modesStr = ExtractAttribute(line, "value");
			std::istringstream modesStream(modesStr);
			std::string mode;
			while (std::getline(modesStream, mode, ',')) {
				mapInfo.modes.push_back(Trim(mode));
			}
		} else if (line.find("<property name=\"Description\"") != std::string::npos) {
			mapInfo.description = ExtractAttribute(line, "value");
		} else if (line.find("<property name=\"HeightMapSize\"") != std::string::npos) {
			std::string sizeStr = ExtractAttribute(line, "value");
			ParseVector2(sizeStr, mapInfo.heightMapSize);
		}
	}
	
	return true;
}

bool WorldDataLoader::LoadSpawnPoints(const std::string& filePath, std::vector<SpawnPoint>& spawnPoints) {
	std::string content = files::read_string(filePath);
	if (content.empty()) {
		std::cerr << "[WorldDataLoader] Failed to read spawnpoints.xml: " << filePath << std::endl;
		return false;
	}
	
	std::istringstream iss(content);
	std::string line;
	
	while (std::getline(iss, line)) {
		line = Trim(line);
		
		if (line.find("<spawnpoint") != std::string::npos) {
			std::string posStr = ExtractAttribute(line, "position");
			std::string rotStr = ExtractAttribute(line, "rotation");
			
			SpawnPoint spawn;
			if (ParseVector3(posStr, spawn.position) && ParseVector3(rotStr, spawn.rotation)) {
				spawnPoints.push_back(spawn);
			}
		}
	}
	
	return true;
}

bool WorldDataLoader::LoadWaterSources(const std::string& filePath, std::vector<WaterSource>& waterSources) {
	std::string content = files::read_string(filePath);
	if (content.empty()) {
		std::cerr << "[WorldDataLoader] Failed to read water_info.xml: " << filePath << std::endl;
		return false;
	}
	
	std::istringstream iss(content);
	std::string line;
	
	while (std::getline(iss, line)) {
		line = Trim(line);
		
		if (line.find("<Water pos=") != std::string::npos) {
			std::string posStr = ExtractAttribute(line, "pos");
			WaterSource water;
			
			if (ParseVector3(posStr, water.position)) {
				// Проверяем опциональные атрибуты minz и maxz
				std::string minzStr = ExtractAttribute(line, "minz");
				std::string maxzStr = ExtractAttribute(line, "maxz");
				
				if (!minzStr.empty() && ParseFloat(minzStr, water.minZ)) {
					water.hasMinZ = true;
				}
				if (!maxzStr.empty() && ParseFloat(maxzStr, water.maxZ)) {
					water.hasMaxZ = true;
				}
				
				waterSources.push_back(water);
			}
		}
	}
	
	return true;
}

bool WorldDataLoader::LoadPrefabs(const std::string& filePath, std::vector<PrefabInstance>& prefabs) {
	std::string content = files::read_string(filePath);
	if (content.empty()) {
		std::cerr << "[WorldDataLoader] Failed to read prefabs.xml: " << filePath << std::endl;
		return false;
	}
	
	std::istringstream iss(content);
	std::string line;
	
	while (std::getline(iss, line)) {
		line = Trim(line);
		
		if (line.find("<decoration") != std::string::npos) {
			PrefabInstance prefab;
			
			prefab.type = ExtractAttribute(line, "type");
			prefab.name = ExtractAttribute(line, "name");
			
			std::string posStr = ExtractAttribute(line, "position");
			ParseVector3(posStr, prefab.position);
			
			std::string rotStr = ExtractAttribute(line, "rotation");
			ParseInt(rotStr, prefab.rotation);
			
			std::string yGroundStr = ExtractAttribute(line, "y_is_groundlevel");
			prefab.yIsGroundLevel = (yGroundStr == "true" || yGroundStr.empty());
			
			if (!prefab.name.empty()) {
				prefabs.push_back(prefab);
			}
		}
	}
	
	return true;
}

bool WorldDataLoader::LoadHeightMap(const std::string& filePath, int width, int height, std::vector<uint16_t>& heights) {
	heights.resize(width * height);
	
#ifdef _WIN32
	// Конвертируем путь в UTF-16 для поддержки Unicode
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0);
	if (size_needed <= 0) {
		std::cerr << "[WorldDataLoader] Failed to convert path to UTF-16: " << filePath << std::endl;
		return false;
	}
	std::vector<wchar_t> wpath(size_needed);
	MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, wpath.data(), size_needed);
	
	std::ifstream file;
	file.open(wpath.data(), std::ios::binary);
#else
	std::ifstream file(filePath, std::ios::binary);
#endif
	
	if (!file.is_open()) {
		std::cerr << "[WorldDataLoader] Failed to open height map: " << filePath << std::endl;
		return false;
	}
	
	file.read(reinterpret_cast<char*>(heights.data()), width * height * sizeof(uint16_t));
	file.close();
	
	return true;
}

bool WorldDataLoader::LoadBiomeMap(const std::string& filePath, int& width, int& height, std::vector<uint8_t>& biomes) {
	// Загружаем PNG и извлекаем индекс биома из цвета
	ImageData* img = png::load_image(filePath);
	if (img == nullptr) {
		std::cerr << "[WorldDataLoader] Failed to load biome map: " << filePath << std::endl;
		return false;
	}
	
	width = img->getWidth();
	height = img->getHeight();
	biomes.resize(width * height);
	
	// Преобразуем RGBA в индекс биома (используем R канал как индекс биома)
	uint8_t* data = (uint8_t*)img->getData();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 4;
			// Используем R канал как индекс биома (или можно использовать более сложную логику)
			biomes[y * width + x] = data[idx];
		}
	}
	
	delete img;
	return true;
}

bool WorldDataLoader::LoadSplatMap(const std::string& filePath, int& width, int& height, std::vector<uint8_t>& rgbaData) {
	ImageData* img = png::load_image(filePath);
	if (img == nullptr) {
		std::cerr << "[WorldDataLoader] Failed to load splat map: " << filePath << std::endl;
		return false;
	}
	
	width = img->getWidth();
	height = img->getHeight();
	
	// Копируем данные
	uint8_t* data = (uint8_t*)img->getData();
	int dataSize = width * height * 4; // RGBA
	rgbaData.resize(dataSize);
	std::copy(data, data + dataSize, rgbaData.begin());
	
	delete img;
	return true;
}

// Вспомогательные функции парсинга
bool WorldDataLoader::ParseVector3(const std::string& str, glm::vec3& vec) {
	std::istringstream iss(str);
	char comma1, comma2;
	if (iss >> vec.x >> comma1 >> vec.y >> comma2 >> vec.z) {
		return (comma1 == ',' && comma2 == ',');
	}
	return false;
}

bool WorldDataLoader::ParseVector2(const std::string& str, glm::ivec2& vec) {
	std::istringstream iss(str);
	char comma;
	if (iss >> vec.x >> comma >> vec.y) {
		return (comma == ',');
	}
	return false;
}

bool WorldDataLoader::ParseInt(const std::string& str, int& value) {
	std::istringstream iss(str);
	return (iss >> value).good();
}

bool WorldDataLoader::ParseFloat(const std::string& str, float& value) {
	std::istringstream iss(str);
	return (iss >> value).good();
}

std::string WorldDataLoader::ExtractAttribute(const std::string& line, const std::string& attrName) {
	std::string searchStr = attrName + "=\"";
	size_t start = line.find(searchStr);
	if (start == std::string::npos) {
		return "";
	}
	
	start += searchStr.length();
	size_t end = line.find("\"", start);
	if (end == std::string::npos) {
		return "";
	}
	
	return line.substr(start, end - start);
}

std::string WorldDataLoader::Trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return "";
	}
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

