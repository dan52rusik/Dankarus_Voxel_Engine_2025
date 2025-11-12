#include "WorldSave.h"
#include "ChunkManager.h"
#include "MCChunk.h"
#include "voxel.h"
#include "GeneratorParams.h"
#include "utils/json_simple.h"
#include "files/files.h"
#include "window/Camera.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

WorldSave::WorldSave() {
}

WorldSave::~WorldSave() {
}

bool WorldSave::writeInt(std::ofstream& file, int value) {
	file.write(reinterpret_cast<const char*>(&value), sizeof(int));
	return file.good();
}

bool WorldSave::writeInt64(std::ofstream& file, int64_t value) {
	file.write(reinterpret_cast<const char*>(&value), sizeof(int64_t));
	return file.good();
}

bool WorldSave::writeFloat(std::ofstream& file, float value) {
	file.write(reinterpret_cast<const char*>(&value), sizeof(float));
	return file.good();
}

bool WorldSave::writeString(std::ofstream& file, const std::string& str) {
	int len = (int)str.length();
	if (!writeInt(file, len)) return false;
	file.write(str.c_str(), len);
	return file.good();
}

bool WorldSave::readInt(std::ifstream& file, int& value) {
	file.read(reinterpret_cast<char*>(&value), sizeof(int));
	return file.good();
}

bool WorldSave::readInt64(std::ifstream& file, int64_t& value) {
	file.read(reinterpret_cast<char*>(&value), sizeof(int64_t));
	return file.good();
}

bool WorldSave::readFloat(std::ifstream& file, float& value) {
	file.read(reinterpret_cast<char*>(&value), sizeof(float));
	return file.good();
}

bool WorldSave::readString(std::ifstream& file, std::string& str) {
	int len;
	if (!readInt(file, len)) return false;
	str.resize(len);
	file.read(&str[0], len);
	return file.good();
}

bool WorldSave::createWorldDirectory(const std::string& worldPath) {
	// Создаем основные папки
#ifdef _WIN32
	const char sep = '\\';
#else
	const char sep = '/';
#endif
	
	std::vector<std::string> dirs = {
		worldPath,
		worldPath + sep + "regions",
		worldPath + sep + "lights",
		worldPath + sep + "entities",
		worldPath + sep + "blocksdata",
		worldPath + sep + "inventories",
		worldPath + sep + "client",
		worldPath + sep + "content",
		worldPath + sep + "data"
	};
	
	for (const std::string& dir : dirs) {
		if (!files::directory_exists(dir)) {
			if (!files::create_directory(dir)) {
				std::cerr << "[SAVE] Failed to create directory: " << dir << std::endl;
				return false;
			}
		}
	}
	
	return true;
}

bool WorldSave::saveRegion(const std::string& regionPath, MCChunk* chunk) {
	std::ofstream file(regionPath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[SAVE] Failed to open region file for writing: " << regionPath << std::endl;
		return false;
	}
	
	// Магические байты
	const char magic[] = "RGON";
	file.write(magic, 4);
	
	// Версия формата региона
	int version = 1;
	writeInt(file, version);
	
	// Координаты чанка
	writeInt(file, chunk->chunkPos.x);
	writeInt(file, chunk->chunkPos.y);
	writeInt(file, chunk->chunkPos.z);
	
	// Собираем все блоки с id != 0
	std::vector<std::pair<glm::ivec3, uint8_t>> blocks;
	
	for (int y = 0; y < MCChunk::CHUNK_SIZE_Y; y++) {
		for (int z = 0; z < MCChunk::CHUNK_SIZE_Z; z++) {
			for (int x = 0; x < MCChunk::CHUNK_SIZE_X; x++) {
				voxel* vox = chunk->getVoxel(x, y, z);
				if (vox != nullptr && vox->id != 0) {
					blocks.push_back({glm::ivec3(x, y, z), vox->id});
				}
			}
		}
	}
	
	// Сохраняем количество блоков
	int numBlocks = (int)blocks.size();
	writeInt(file, numBlocks);
	
	// Сохраняем все блоки (локальные координаты в чанке)
	for (const auto& block : blocks) {
		writeInt(file, block.first.x);
		writeInt(file, block.first.y);
		writeInt(file, block.first.z);
		file.write(reinterpret_cast<const char*>(&block.second), sizeof(uint8_t));
	}
	
	file.close();
	return true;
}

bool WorldSave::loadRegion(const std::string& regionPath, ChunkManager& chunkManager, int rx, int ry, int rz) {
	// Эта функция пока не используется, т.к. загрузка регионов происходит напрямую в load()
	// Оставлена для будущего использования
	(void)regionPath;
	(void)chunkManager;
	(void)rx;
	(void)ry;
	(void)rz;
	return false;
}

bool WorldSave::save(const std::string& worldPath, ChunkManager& chunkManager, const std::string& worldName, int64_t seed,
                     float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation,
                     Camera* camera) {
	// Проверяем, является ли путь файлом (старый формат) или папкой (новый формат)
	// Если путь заканчивается на .vxl - это старый формат
	if (worldPath.length() > 4 && worldPath.substr(worldPath.length() - 4) == ".vxl") {
		// Старый формат - сохраняем как раньше (для обратной совместимости)
		// Но преобразуем int64_t seed в int
		int seedInt = (int)seed;
		std::ofstream file(worldPath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[SAVE] Failed to open file for writing: " << worldPath << std::endl;
			return false;
		}
		
		const char magic[] = "VXEL";
		file.write(magic, 4);
		
		int version = 1;
		writeInt(file, version);
		writeInt(file, seedInt);
		writeFloat(file, baseFreq);
		writeInt(file, octaves);
		writeFloat(file, lacunarity);
		writeFloat(file, gain);
		writeFloat(file, baseHeight);
		writeFloat(file, heightVariation);
		
		std::vector<MCChunk*> chunks = chunkManager.getAllChunks();
		std::vector<std::pair<glm::ivec3, uint8_t>> blocks;
		
		for (MCChunk* chunk : chunks) {
			for (int y = 0; y < MCChunk::CHUNK_SIZE_Y; y++) {
				for (int z = 0; z < MCChunk::CHUNK_SIZE_Z; z++) {
					for (int x = 0; x < MCChunk::CHUNK_SIZE_X; x++) {
						voxel* vox = chunk->getVoxel(x, y, z);
						if (vox != nullptr && vox->id != 0) {
							int wx = chunk->chunkPos.x * MCChunk::CHUNK_SIZE_X + x;
							int wy = chunk->chunkPos.y * MCChunk::CHUNK_SIZE_Y + y;
							int wz = chunk->chunkPos.z * MCChunk::CHUNK_SIZE_Z + z;
							blocks.push_back({glm::ivec3(wx, wy, wz), vox->id});
						}
					}
				}
			}
		}
		
		int numBlocks = (int)blocks.size();
		writeInt(file, numBlocks);
		
		for (const auto& block : blocks) {
			writeInt(file, block.first.x);
			writeInt(file, block.first.y);
			writeInt(file, block.first.z);
			file.write(reinterpret_cast<const char*>(&block.second), sizeof(uint8_t));
		}
		
		file.close();
		std::cout << "[SAVE] Saved " << numBlocks << " blocks to " << worldPath << " (old format)" << std::endl;
		return true;
	}
	
	// Новый формат - папка с JSON и регионами
	// Создаем структуру папок
	if (!createWorldDirectory(worldPath)) {
		std::cerr << "[SAVE] Failed to create world directory structure" << std::endl;
		return false;
	}
	
	// Сохраняем world.json
	json_simple::Value worldJson;
	worldJson["name"] = json_simple::Value(worldName);
	worldJson["seed"] = json_simple::Value(seed);
	worldJson["version"]["major"] = json_simple::Value(0);
	worldJson["version"]["minor"] = json_simple::Value(1);
	worldJson["generator"] = json_simple::Value("voxelnoxel:terrain");
	worldJson["time"]["total-time"] = json_simple::Value(0.0);
	worldJson["time"]["day-time"] = json_simple::Value(0.5);
	worldJson["time"]["day-time-speed"] = json_simple::Value(1);
	worldJson["weather"]["fog"] = json_simple::Value(0);
	
	// Параметры генерации (сохраняем в отдельном объекте)
	worldJson["generator-params"]["baseFreq"] = json_simple::Value(baseFreq);
	worldJson["generator-params"]["octaves"] = json_simple::Value(octaves);
	worldJson["generator-params"]["lacunarity"] = json_simple::Value(lacunarity);
	worldJson["generator-params"]["gain"] = json_simple::Value(gain);
	worldJson["generator-params"]["baseHeight"] = json_simple::Value(baseHeight);
	worldJson["generator-params"]["heightVariation"] = json_simple::Value(heightVariation);
	// Сохраняем waterLevel для согласованности
	float waterLevel = chunkManager.getWaterLevel();
	worldJson["generator-params"]["waterLevel"] = json_simple::Value(waterLevel);
	
#ifdef _WIN32
	const char sep = '\\';
#else
	const char sep = '/';
#endif
	
	if (!json_simple::writeFile(worldPath + sep + "world.json", worldJson)) {
		std::cerr << "[SAVE] Failed to write world.json" << std::endl;
		return false;
	}
	
	// Сохраняем player.json (если есть камера)
	if (camera != nullptr) {
		savePlayer(worldPath, camera);
	}
	
	// Сохраняем resources.json (базовый список блоков)
	json_simple::Value resourcesJson;
	json_simple::Value blocksArray;
	blocksArray.type = json_simple::Value::ARRAY;
	blocksArray.arrayValue.push_back(json_simple::Value("core:air"));
	blocksArray.arrayValue.push_back(json_simple::Value("core:stone"));
	blocksArray.arrayValue.push_back(json_simple::Value("core:lamp"));
	blocksArray.arrayValue.push_back(json_simple::Value("core:wood"));
	blocksArray.arrayValue.push_back(json_simple::Value("core:grass"));
	resourcesJson["blocks"] = blocksArray;
	resourcesJson["region-version"] = json_simple::Value(1);
	
	if (!json_simple::writeFile(worldPath + sep + "resources.json", resourcesJson)) {
		std::cerr << "[SAVE] Failed to write resources.json" << std::endl;
		return false;
	}
	
	// Сохраняем регионы (чанки)
	std::vector<MCChunk*> chunks = chunkManager.getAllChunks();
	
	// Сохраняем каждый чанк отдельно (упрощенная версия)
	int savedChunks = 0;
	for (MCChunk* chunk : chunks) {
		// Используем имя файла с координатами чанка
		std::ostringstream oss;
		oss << chunk->chunkPos.x << "_" << chunk->chunkPos.y << "_" << chunk->chunkPos.z << ".bin";
		std::string chunkFileName = worldPath + sep + "regions" + sep + oss.str();
		if (saveRegion(chunkFileName, chunk)) {
			savedChunks++;
		}
	}
	
	std::cout << "[SAVE] Saved world to " << worldPath << " (chunks: " << savedChunks << ")" << std::endl;
	return true;
}

bool WorldSave::load(const std::string& worldPath, ChunkManager& chunkManager, std::string& worldName, int64_t& seed,
                     float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation,
                     Camera* camera) {
	// Проверяем, является ли путь файлом (старый формат) или папкой (новый формат)
	if (worldPath.length() > 4 && worldPath.substr(worldPath.length() - 4) == ".vxl") {
		// Старый формат
		std::ifstream file(worldPath, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "[LOAD] Failed to open file for reading: " << worldPath << std::endl;
			return false;
		}
		
		char magic[4];
		file.read(magic, 4);
		if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'E' || magic[3] != 'L') {
			std::cerr << "[LOAD] Invalid file format" << std::endl;
			file.close();
			return false;
		}
		
		int version;
		if (!readInt(file, version)) {
			file.close();
			return false;
		}
		
		int seedInt;
		if (!readInt(file, seedInt)) return false;
		seed = seedInt;
		if (!readFloat(file, baseFreq)) return false;
		if (!readInt(file, octaves)) return false;
		if (!readFloat(file, lacunarity)) return false;
		if (!readFloat(file, gain)) return false;
		if (!readFloat(file, baseHeight)) return false;
		if (!readFloat(file, heightVariation)) return false;
		
		int numBlocks;
		if (!readInt(file, numBlocks)) {
			file.close();
			return false;
		}
		
		std::vector<std::pair<glm::ivec3, uint8_t>> blocks;
		for (int i = 0; i < numBlocks; i++) {
			int x, y, z;
			uint8_t id;
			
			if (!readInt(file, x) || !readInt(file, y) || !readInt(file, z)) break;
			file.read(reinterpret_cast<char*>(&id), sizeof(uint8_t));
			
			if (!file.good()) break;
			blocks.push_back({glm::ivec3(x, y, z), id});
		}
		
		file.close();
		
		for (const auto& block : blocks) {
			chunkManager.setVoxel(block.first.x, block.first.y, block.first.z, block.second);
		}
		
		// Извлекаем имя мира из пути
		size_t lastSlash = worldPath.find_last_of("/\\");
		size_t lastDot = worldPath.find_last_of(".");
		if (lastSlash != std::string::npos && lastDot != std::string::npos) {
			worldName = worldPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
		} else {
			worldName = "Unknown";
		}
		
		std::cout << "[LOAD] Loaded " << blocks.size() << " blocks from " << worldPath << " (old format)" << std::endl;
		return true;
	}
	
	// Новый формат - папка с JSON
#ifdef _WIN32
	const char sep = '\\';
#else
	const char sep = '/';
#endif
	
	// Загружаем world.json
	std::string worldJsonPath = worldPath + sep + "world.json";
	json_simple::Value worldJson = json_simple::Parser::parseFile(worldJsonPath);
	
	if (worldJson.type == json_simple::Value::NULL_TYPE) {
		std::cerr << "[LOAD] Failed to load world.json from " << worldPath << std::endl;
		return false;
	}
	
	worldName = worldJson["name"].getString("Unknown");
	seed = worldJson["seed"].getInt64(0);
	
	// Загружаем параметры генерации
	GeneratorParams gp;
	if (worldJson.has("generator-params")) {
		json_simple::Value params = worldJson["generator-params"];
		gp.baseFreq = params["baseFreq"].getNumber(1.0f / 256.0f);  // ПРАВИЛЬНО: float деление
		gp.octaves = params["octaves"].getInt(5);
		gp.lacunarity = params["lacunarity"].getNumber(2.0f);
		gp.gain = params["gain"].getNumber(0.5f);
		gp.baseHeight = params["baseHeight"].getNumber(40.0f);
			gp.heightVariation = params["heightVariation"].getNumber(240.0f);
		gp.waterLevel = params["waterLevel"].getNumber(gp.baseHeight - 2.0f);  // читаем из JSON или вычисляем
	} else {
		// Значения по умолчанию
		gp.baseFreq = 1.0f / 256.0f;  // ПРАВИЛЬНО: float деление
		gp.octaves = 5;
		gp.lacunarity = 2.0f;
		gp.gain = 0.5f;
		gp.baseHeight = 40.0f;
			gp.heightVariation = 240.0f;
		gp.waterLevel = gp.baseHeight - 2.0f;
	}
	gp.seed = seed;
	
	// Возвращаем параметры через выходные параметры (для совместимости)
	baseFreq = gp.baseFreq;
	octaves = gp.octaves;
	lacunarity = gp.lacunarity;
	gain = gp.gain;
	baseHeight = gp.baseHeight;
	heightVariation = gp.heightVariation;
	
	// Применяем параметры через единую функцию configure()
	chunkManager.configure(gp);
	
	// Загружаем регионы (чанки)
	// Примечание: загрузка player.json теперь выполняется в WorldManager::loadWorld()
	std::string regionsPath = worldPath + sep + "regions";
	std::vector<std::string> regionFiles = files::list_files(regionsPath, ".bin");
	
	int loadedChunks = 0;
	for (const auto& regionFile : regionFiles) {
		// Имя файла имеет формат "cx_cy_cz.bin"
		// Парсим координаты чанка из имени файла
		std::string nameWithoutExt = regionFile;
		if (nameWithoutExt.size() > 4 && nameWithoutExt.substr(nameWithoutExt.size() - 4) == ".bin") {
			nameWithoutExt = nameWithoutExt.substr(0, nameWithoutExt.size() - 4);
		}
		
		// Парсим координаты
		size_t pos1 = nameWithoutExt.find('_');
		size_t pos2 = nameWithoutExt.find('_', pos1 + 1);
		if (pos1 != std::string::npos && pos2 != std::string::npos) {
			try {
				int cx = std::stoi(nameWithoutExt.substr(0, pos1));
				int cy = std::stoi(nameWithoutExt.substr(pos1 + 1, pos2 - pos1 - 1));
				int cz = std::stoi(nameWithoutExt.substr(pos2 + 1));
				
				// Загружаем чанк
				std::string regionFilePath = regionsPath + sep + regionFile;
				
				// Читаем файл и загружаем блоки
				std::ifstream file(regionFilePath, std::ios::binary);
				if (file.is_open()) {
					char magic[4];
					file.read(magic, 4);
					if (magic[0] == 'R' && magic[1] == 'G' && magic[2] == 'O' && magic[3] == 'N') {
						int version;
						if (readInt(file, version)) {
							int fileCx, fileCy, fileCz;
							if (readInt(file, fileCx) && readInt(file, fileCy) && readInt(file, fileCz)) {
								int numBlocks;
								if (readInt(file, numBlocks)) {
									for (int i = 0; i < numBlocks; i++) {
										int lx, ly, lz;
										uint8_t id;
										if (readInt(file, lx) && readInt(file, ly) && readInt(file, lz)) {
											file.read(reinterpret_cast<char*>(&id), sizeof(uint8_t));
											if (file.good()) {
												// Вычисляем мировые координаты
												int wx = fileCx * MCChunk::CHUNK_SIZE_X + lx;
												int wy = fileCy * MCChunk::CHUNK_SIZE_Y + ly;
												int wz = fileCz * MCChunk::CHUNK_SIZE_Z + lz;
												chunkManager.setVoxel(wx, wy, wz, id);
											}
										}
									}
									loadedChunks++;
								}
							}
						}
					}
					file.close();
				}
			} catch (...) {
				// Игнорируем ошибки парсинга
			}
		}
	}
	
	std::cout << "[LOAD] Loaded world '" << worldName << "' (seed: " << seed << ") from " << worldPath << " (chunks: " << loadedChunks << ")" << std::endl;
	return true;
}

bool WorldSave::savePlayer(const std::string& worldPath, Camera* camera) {
	if (camera == nullptr) return false;
	
	json_simple::Value playerJson;
	json_simple::Value playersArray;
	playersArray.type = json_simple::Value::ARRAY;
	
	json_simple::Value player;
	player["id"] = json_simple::Value(0);
	player["name"] = json_simple::Value("");
	player["position"] = json_simple::Value(json_simple::Value::ARRAY);
	player["position"].arrayValue.push_back(json_simple::Value(camera->position.x));
	player["position"].arrayValue.push_back(json_simple::Value(camera->position.y));
	player["position"].arrayValue.push_back(json_simple::Value(camera->position.z));
	
	// Сохраняем вращение камеры (упрощенно)
	player["rotation"] = json_simple::Value(json_simple::Value::ARRAY);
	player["rotation"].arrayValue.push_back(json_simple::Value(0.0));
	player["rotation"].arrayValue.push_back(json_simple::Value(0.0));
	player["rotation"].arrayValue.push_back(json_simple::Value(0.0));
	
	playersArray.arrayValue.push_back(player);
	playerJson["players"] = playersArray;
	
#ifdef _WIN32
	const char sep = '\\';
#else
	const char sep = '/';
#endif
	
	return json_simple::writeFile(worldPath + sep + "player.json", playerJson);
}

bool WorldSave::loadPlayer(const std::string& worldPath, Camera* camera) {
	if (camera == nullptr) return false;
	
#ifdef _WIN32
	const char sep = '\\';
#else
	const char sep = '/';
#endif
	
	std::string playerJsonPath = worldPath + sep + "player.json";
	json_simple::Value playerJson = json_simple::Parser::parseFile(playerJsonPath);
	
	if (playerJson.type == json_simple::Value::NULL_TYPE) {
		// Файл может не существовать - это нормально
		// Если файла нет - вернём false; WorldManager сам выберет безопасный спавн
		return false;
	}
	
	if (playerJson.has("players") && playerJson["players"].size() > 0) {
		json_simple::Value player = playerJson["players"][0];
		
		if (player.has("position") && player["position"].size() >= 3) {
			float x = player["position"][0].getNumber(0.0);
			float y = player["position"][1].getNumber(0.0);
			float z = player["position"][2].getNumber(0.0);
			
			// Только читаем позицию из JSON, без логики спавна
			camera->position = glm::vec3(x, y, z);
		}
		
		// Вращение пока не загружаем (требует дополнительной работы с камерой)
	}
	
	return true;
}
