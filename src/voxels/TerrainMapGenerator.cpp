#include "TerrainMapGenerator.h"
#include "ChunkManager.h"
#include "../graphics/ImageData.h"
#include "../coders/png.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdlib>

TerrainMapGenerator::TerrainMapGenerator() {
	heights.resize(DATA_SIZE, 0);
	densitySub.resize(DATA_SIZE, 0.0f);
	normals.resize(MAP_SIZE, glm::vec3(0.0f, 1.0f, 0.0f));
	colors.resize(MAP_SIZE, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

TerrainMapGenerator::~TerrainMapGenerator() {
	// Очистка не требуется, т.к. используются std::vector
}

bool TerrainMapGenerator::GenerateTerrain(const glm::vec3& centerPos, ChunkManager* chunkManager, const std::string& outputPath) {
	if (chunkManager == nullptr) {
		std::cerr << "[TerrainMapGenerator] ChunkManager is null" << std::endl;
		return false;
	}
	
	// Вычисляем начальные координаты (центр минус половина размера)
	int startX = static_cast<int>(centerPos.x) - MAP_WIDTH / 2;
	int startZ = static_cast<int>(centerPos.z) - MAP_HEIGHT / 2;
	
	std::cout << "[TerrainMapGenerator] Generating terrain map at (" << startX << ", " << startZ << ")" << std::endl;
	
	// Шаг 1: Кэшируем данные о высотах и блоках
	for (int z = 0; z < DATA_HEIGHT; z++) {
		for (int x = 0; x < DATA_WIDTH; x++) {
			int worldX = startX + x;
			int worldZ = startZ + z;
			
			// Получаем высоту поверхности
			int height = getSurfaceHeight(chunkManager, worldX, worldZ);
			heights[x + z * DATA_WIDTH] = height;
			
			// Получаем дополнительную плотность (для точности нормалей)
			// Пока используем 0.0, можно улучшить позже
			densitySub[x + z * DATA_WIDTH] = 0.0f;
		}
	}
	
	std::cout << "[TerrainMapGenerator] Cached " << DATA_SIZE << " height points" << std::endl;
	
	// Шаг 2: Вычисляем нормали и цвета для каждого пикселя карты
	for (int z = 0; z < MAP_HEIGHT; z++) {
		for (int x = 0; x < MAP_WIDTH; x++) {
			// Вычисляем нормаль поверхности
			glm::vec3 normal = calcNormal(x, z, 0, 1, 1, 0);
			
			// Улучшаем нормаль, используя соседние точки (если возможно)
			if (x > 0 && z > 0) {
				glm::vec3 n1 = calcNormal(x, z, 0, -1, -1, 0);
				glm::vec3 n2 = calcNormal(x, z, 1, 1, 1, -1);
				glm::vec3 n3 = calcNormal(x, z, -1, -1, -1, 1);
				normal = (normal + n1 + n2 + n3) * 0.25f;
				normal = glm::normalize(normal);
			}
			
			// Преобразуем нормаль в формат для сохранения (0..1 вместо -1..1)
			// Порядок: z, x, y (как в 7DTD)
			glm::vec3 normalForColor(normal.z, normal.x, normal.y);
			normalForColor = (normalForColor + 1.0f) * 0.5f;
			normals[x + z * MAP_WIDTH] = normalForColor;
			
			// Получаем цвет блока на поверхности
			int worldX = startX + x;
			int worldZ = startZ + z;
			int height = heights[x + z * DATA_WIDTH];
			glm::vec4 color = getBlockColor(chunkManager, worldX, height, worldZ);
			
			colors[x + z * MAP_WIDTH] = color;
		}
	}
	
	std::cout << "[TerrainMapGenerator] Generated " << MAP_SIZE << " map pixels" << std::endl;
	
	// Шаг 3: Сохраняем карту в PNG
	bool success = saveMapToPNG(outputPath);
	if (success) {
		std::cout << "[TerrainMapGenerator] Map saved to " << outputPath << std::endl;
	} else {
		std::cerr << "[TerrainMapGenerator] Failed to save map to " << outputPath << std::endl;
	}
	
	return success;
}

glm::vec3 TerrainMapGenerator::calcNormal(int x, int z, int xAdd1, int zAdd1, int xAdd2, int zAdd2) {
	// Получаем индексы для доступа к данным
	int idx0 = x + z * DATA_WIDTH;
	int idx1 = x + xAdd1 + (z + zAdd1) * DATA_WIDTH;
	int idx2 = x + xAdd2 + (z + zAdd2) * DATA_WIDTH;
	
	// Вычисляем Y координаты с учетом дополнительной плотности
	float y1 = static_cast<float>(heights[idx0]) - densitySub[idx0];
	float y2 = static_cast<float>(heights[idx1]) - densitySub[idx1];
	float y3 = static_cast<float>(heights[idx2]) - densitySub[idx2];
	
	// Вычисляем два вектора для cross product
	glm::vec3 v1(static_cast<float>(xAdd1), y3, static_cast<float>(zAdd1));
	glm::vec3 v2(static_cast<float>(xAdd2), y2, static_cast<float>(zAdd2));
	glm::vec3 origin(0.0f, y1, 0.0f);
	
	// Вычисляем нормаль через cross product
	glm::vec3 normal = glm::cross(v1 - origin, v2 - origin);
	normal = glm::normalize(normal);
	
	return normal;
}

int TerrainMapGenerator::getSurfaceHeight(ChunkManager* chunkManager, int worldX, int worldZ) {
	// Ищем высоту поверхности, начиная сверху вниз
	// В 7DTD используется GetHeight, но у нас нужно искать вручную
	
	// Начинаем с максимальной высоты и идем вниз
	for (int y = 255; y >= 0; y--) {
		voxel* vox = chunkManager->getVoxel(worldX, y, worldZ);
		if (vox != nullptr && vox->id != 0) {
			return y;
		}
	}
	
	return 0; // Если ничего не найдено, возвращаем 0
}

glm::vec4 TerrainMapGenerator::getBlockColor(ChunkManager* chunkManager, int worldX, int worldY, int worldZ) {
	// Получаем блок на поверхности
	voxel* vox = chunkManager->getVoxel(worldX, worldY, worldZ);
	
	if (vox == nullptr || vox->id == 0) {
		// Воздух или вода - синий цвет
		return glm::vec4(0.2f, 0.4f, 0.8f, 1.0f); // Голубой для воды/воздуха
	}
	
	// Простая цветовая схема на основе ID блока
	// Можно улучшить, добавив таблицу цветов для разных типов блоков
	switch (vox->id) {
		case 1: // Камень
			return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Серый
		case 2: // Земля
			return glm::vec4(0.6f, 0.4f, 0.2f, 1.0f); // Коричневый
		case 3: // Трава
			return glm::vec4(0.2f, 0.6f, 0.2f, 1.0f); // Зеленый
		case 4: // Песок
			return glm::vec4(0.9f, 0.8f, 0.6f, 1.0f); // Бежевый
		default:
			// Для других блоков используем цвет на основе ID
			float r = static_cast<float>((vox->id * 17) % 255) / 255.0f;
			float g = static_cast<float>((vox->id * 31) % 255) / 255.0f;
			float b = static_cast<float>((vox->id * 47) % 255) / 255.0f;
			return glm::vec4(r, g, b, 1.0f);
	}
}

bool TerrainMapGenerator::saveMapToPNG(const std::string& filepath) {
	// Создаем массив пикселей для PNG (RGBA, 8 бит на канал)
	// Важно: ImageData освобождает память через free(), поэтому используем malloc
	unsigned char* pixels = static_cast<unsigned char*>(malloc(MAP_SIZE * 4));
	if (pixels == nullptr) {
		std::cerr << "[TerrainMapGenerator] Failed to allocate memory for pixels" << std::endl;
		return false;
	}
	
	// Заполняем пиксели (переворачиваем по Y для PNG формата)
	for (int y = 0; y < MAP_HEIGHT; y++) {
		for (int x = 0; x < MAP_WIDTH; x++) {
			// Индекс в исходном массиве (сверху вниз)
			int srcIndex = x + y * MAP_WIDTH;
			// Индекс в PNG массиве (снизу вверх)
			int dstIndex = x + (MAP_HEIGHT - 1 - y) * MAP_WIDTH;
			
			// Преобразуем float цвета (0..1) в unsigned char (0..255)
			pixels[dstIndex * 4 + 0] = static_cast<unsigned char>(colors[srcIndex].r * 255.0f); // R
			pixels[dstIndex * 4 + 1] = static_cast<unsigned char>(colors[srcIndex].g * 255.0f); // G
			pixels[dstIndex * 4 + 2] = static_cast<unsigned char>(colors[srcIndex].b * 255.0f); // B
			pixels[dstIndex * 4 + 3] = static_cast<unsigned char>(colors[srcIndex].a * 255.0f); // A
		}
	}
	
	// Создаем ImageData и сохраняем через png::write_image
	// ImageData будет освобождать pixels через free() в деструкторе
	ImageData* image = new ImageData(ImageFormat::rgba8888, MAP_WIDTH, MAP_HEIGHT, pixels);
	
	if (image == nullptr) {
		std::cerr << "[TerrainMapGenerator] Failed to create ImageData" << std::endl;
		free(pixels);
		return false;
	}
	
	// Сохраняем PNG
	png::write_image(filepath, image);
	
	delete image; // Это также освободит pixels
	
	return true;
}

