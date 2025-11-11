#ifndef VOXELS_TERRAINMAPGENERATOR_H_
#define VOXELS_TERRAINMAPGENERATOR_H_

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>

// Forward declarations
class ChunkManager;

// Генератор карты террейна
// Адаптировано из 7 Days To Die TerrainMapGenerator
class TerrainMapGenerator {
public:
	// Размер карты (1024x1024 пикселей)
	static constexpr int MAP_WIDTH = 1024;
	static constexpr int MAP_HEIGHT = 1024;
	static constexpr int MAP_SIZE = MAP_WIDTH * MAP_HEIGHT;
	
	// Размер данных (1025x1025 для интерполяции)
	static constexpr int DATA_WIDTH = MAP_WIDTH + 1;
	static constexpr int DATA_HEIGHT = MAP_HEIGHT + 1;
	static constexpr int DATA_SIZE = DATA_WIDTH * DATA_HEIGHT;
	
	TerrainMapGenerator();
	~TerrainMapGenerator();
	
	// Генерация карты террейна
	// centerPos - центр карты в мировых координатах (обычно позиция игрока)
	// chunkManager - менеджер чанков для получения данных
	// outputPath - путь для сохранения PNG файла (например, "Map.png")
	bool GenerateTerrain(const glm::vec3& centerPos, ChunkManager* chunkManager, const std::string& outputPath);
	
private:
	// Данные для генерации карты
	std::vector<int> heights;        // Высоты поверхности (DATA_SIZE)
	std::vector<float> densitySub;  // Дополнительная плотность для точности (DATA_SIZE)
	std::vector<glm::vec3> normals; // Нормали поверхности (MAP_SIZE)
	std::vector<glm::vec4> colors;  // Цвета для карты (MAP_SIZE) - RGBA
	
	// Вспомогательные функции
	glm::vec3 calcNormal(int x, int z, int xAdd1, int zAdd1, int xAdd2, int zAdd2);
	
	// Получить высоту поверхности в точке (x, z) в мировых координатах
	int getSurfaceHeight(ChunkManager* chunkManager, int worldX, int worldZ);
	
	// Получить цвет блока в точке (x, y, z)
	glm::vec4 getBlockColor(ChunkManager* chunkManager, int worldX, int worldY, int worldZ);
	
	// Сохранить карту в PNG файл
	bool saveMapToPNG(const std::string& filepath);
};

#endif /* VOXELS_TERRAINMAPGENERATOR_H_ */

