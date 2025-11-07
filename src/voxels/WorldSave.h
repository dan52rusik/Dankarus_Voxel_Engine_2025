#ifndef VOXELS_WORLDSAVE_H_
#define VOXELS_WORLDSAVE_H_

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class ChunkManager;

class WorldSave {
public:
	WorldSave();
	~WorldSave();
	
	// Сохранение/загрузка мира
	bool save(const std::string& filename, ChunkManager& chunkManager, int seed, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation);
	bool load(const std::string& filename, ChunkManager& chunkManager, int& seed, float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation);
	
private:
	// Вспомогательные функции для работы с файлами
	bool writeInt(std::ofstream& file, int value);
	bool writeFloat(std::ofstream& file, float value);
	bool writeString(std::ofstream& file, const std::string& str);
	bool readInt(std::ifstream& file, int& value);
	bool readFloat(std::ifstream& file, float& value);
	bool readString(std::ifstream& file, std::string& str);
};

#endif /* VOXELS_WORLDSAVE_H_ */

