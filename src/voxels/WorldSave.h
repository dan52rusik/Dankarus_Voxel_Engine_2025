#ifndef VOXELS_WORLDSAVE_H_
#define VOXELS_WORLDSAVE_H_

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class ChunkManager;
class Camera;
class MCChunk;

class WorldSave {
public:
	WorldSave();
	~WorldSave();
	
	// Сохранение/загрузка мира (новая структура: папка с JSON и регионами)
	bool save(const std::string& worldPath, ChunkManager& chunkManager, const std::string& worldName, int64_t seed, 
	          float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation,
	          Camera* camera = nullptr);
	bool load(const std::string& worldPath, ChunkManager& chunkManager, std::string& worldName, int64_t& seed,
	          float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation,
	          Camera* camera = nullptr);
	
	// Сохранение/загрузка позиции игрока
	bool savePlayer(const std::string& worldPath, Camera* camera);
	bool loadPlayer(const std::string& worldPath, Camera* camera); // Только чтение/запись JSON, без логики спавна
	
private:
	// Вспомогательные функции для работы с файлами
	bool writeInt(std::ofstream& file, int value);
	bool writeInt64(std::ofstream& file, int64_t value);
	bool writeFloat(std::ofstream& file, float value);
	bool writeString(std::ofstream& file, const std::string& str);
	bool readInt(std::ifstream& file, int& value);
	bool readInt64(std::ifstream& file, int64_t& value);
	bool readFloat(std::ifstream& file, float& value);
	bool readString(std::ifstream& file, std::string& str);
	
	// Работа с регионами (чанками)
	bool saveRegion(const std::string& regionPath, MCChunk* chunk);
	bool loadRegion(const std::string& regionPath, ChunkManager& chunkManager, int rx, int ry, int rz);
	
	// Создание структуры папок мира
	bool createWorldDirectory(const std::string& worldPath);
};

#endif /* VOXELS_WORLDSAVE_H_ */

