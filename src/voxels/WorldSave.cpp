#include "WorldSave.h"
#include "ChunkManager.h"
#include "MCChunk.h"
#include "voxel.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

WorldSave::WorldSave() {
}

WorldSave::~WorldSave() {
}

bool WorldSave::writeInt(std::ofstream& file, int value) {
	file.write(reinterpret_cast<const char*>(&value), sizeof(int));
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

bool WorldSave::save(const std::string& filename, ChunkManager& chunkManager, int seed, float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation) {
	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[SAVE] Failed to open file for writing: " << filename << std::endl;
		return false;
	}
	
	// Магические байты для проверки формата
	const char magic[] = "VXEL";
	file.write(magic, 4);
	
	// Версия формата сохранения
	int version = 1;
	writeInt(file, version);
	
	// Параметры генерации
	writeInt(file, seed);
	writeFloat(file, baseFreq);
	writeInt(file, octaves);
	writeFloat(file, lacunarity);
	writeFloat(file, gain);
	writeFloat(file, baseHeight);
	writeFloat(file, heightVariation);
	
	// Сохраняем все воксельные блоки
	// Формат: [x, y, z, id]...
	std::vector<MCChunk*> chunks = chunkManager.getAllChunks();
	
	// Собираем все блоки с id != 0
	std::vector<std::pair<glm::ivec3, uint8_t>> blocks;
	
	for (MCChunk* chunk : chunks) {
		for (int y = 0; y < MCChunk::CHUNK_SIZE_Y; y++) {
			for (int z = 0; z < MCChunk::CHUNK_SIZE_Z; z++) {
				for (int x = 0; x < MCChunk::CHUNK_SIZE_X; x++) {
					voxel* vox = chunk->getVoxel(x, y, z);
					if (vox != nullptr && vox->id != 0) {
						// Вычисляем мировые координаты (используем ту же формулу, что и в ChunkManager)
						int wx = chunk->chunkPos.x * MCChunk::CHUNK_SIZE_X + x;
						int wy = chunk->chunkPos.y * MCChunk::CHUNK_SIZE_Y + y;
						int wz = chunk->chunkPos.z * MCChunk::CHUNK_SIZE_Z + z;
						blocks.push_back({glm::ivec3(wx, wy, wz), vox->id});
					}
				}
			}
		}
	}
	
	// Сохраняем количество блоков
	int numBlocks = (int)blocks.size();
	writeInt(file, numBlocks);
	
	// Сохраняем все блоки
	for (const auto& block : blocks) {
		writeInt(file, block.first.x);
		writeInt(file, block.first.y);
		writeInt(file, block.first.z);
		file.write(reinterpret_cast<const char*>(&block.second), sizeof(uint8_t));
	}
	
	file.close();
	
	std::cout << "[SAVE] Saved " << numBlocks << " blocks to " << filename << std::endl;
	return true;
}

bool WorldSave::load(const std::string& filename, ChunkManager& chunkManager, int& seed, float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[LOAD] Failed to open file for reading: " << filename << std::endl;
		return false;
	}
	
	// Проверяем магические байты
	char magic[4];
	file.read(magic, 4);
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'E' || magic[3] != 'L') {
		std::cerr << "[LOAD] Invalid file format (magic bytes mismatch)" << std::endl;
		file.close();
		return false;
	}
	
	// Читаем версию
	int version;
	if (!readInt(file, version)) {
		std::cerr << "[LOAD] Failed to read version" << std::endl;
		file.close();
		return false;
	}
	
	// Читаем параметры генерации
	if (!readInt(file, seed)) return false;
	if (!readFloat(file, baseFreq)) return false;
	if (!readInt(file, octaves)) return false;
	if (!readFloat(file, lacunarity)) return false;
	if (!readFloat(file, gain)) return false;
	if (!readFloat(file, baseHeight)) return false;
	if (!readFloat(file, heightVariation)) return false;
	
	// Читаем количество блоков
	int numBlocks;
	if (!readInt(file, numBlocks)) {
		std::cerr << "[LOAD] Failed to read block count" << std::endl;
		file.close();
		return false;
	}
	
	std::cout << "[LOAD] Loading " << numBlocks << " blocks from " << filename << std::endl;
	
	// Сначала читаем все блоки в память
	std::vector<std::pair<glm::ivec3, uint8_t>> blocks;
	for (int i = 0; i < numBlocks; i++) {
		int x, y, z;
		uint8_t id;
		
		if (!readInt(file, x)) break;
		if (!readInt(file, y)) break;
		if (!readInt(file, z)) break;
		file.read(reinterpret_cast<char*>(&id), sizeof(uint8_t));
		
		if (!file.good()) break;
		
		blocks.push_back({glm::ivec3(x, y, z), id});
	}
	
	file.close();
	
	// Теперь устанавливаем все блоки (чтобы чанки успели сгенерироваться)
	int loaded = 0;
	for (const auto& block : blocks) {
		// Устанавливаем блок (ChunkManager автоматически создаст чанк, если его нет)
		chunkManager.setVoxel(block.first.x, block.first.y, block.first.z, block.second);
		loaded++;
	}
	
	std::cout << "[LOAD] Loaded " << loaded << " blocks successfully" << std::endl;
	return loaded == numBlocks;
}

