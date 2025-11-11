#include "HeightMapUtils.h"
#include "../graphics/ImageData.h"
#include "../coders/png.h"
#include "../files/files.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <limits>

namespace HeightMapUtils {

// ==================== HeightData2D ====================

HeightData2D::HeightData2D(int w, int h) : width(w), height(h) {
	data.resize(w * h, 0.0f);
}

HeightData2D::HeightData2D(int w, int h, const float* src) : width(w), height(h) {
	data.resize(w * h);
	if (src != nullptr) {
		std::memcpy(data.data(), src, w * h * sizeof(float));
	}
}

HeightData2D::~HeightData2D() {
}

float& HeightData2D::at(int x, int y) {
	return data[y * width + x];
}

const float& HeightData2D::at(int x, int y) const {
	return data[y * width + x];
}

float HeightData2D::get(int x, int y) const {
	if (x < 0 || x >= width || y < 0 || y >= height) {
		return 0.0f;
	}
	return data[y * width + x];
}

void HeightData2D::set(int x, int y, float value) {
	if (x < 0 || x >= width || y < 0 || y >= height) {
		return;
	}
	data[y * width + x] = value;
}

// ==================== Загрузка DTM из PNG/TGA ====================

HeightData2D* convertDTMToHeightData(const std::string& filepath, bool flip) {
	// Определяем тип файла по расширению
	size_t dotPos = filepath.find_last_of(".");
	if (dotPos == std::string::npos) {
		std::cerr << "[HeightMapUtils] Invalid filepath (no extension): " << filepath << std::endl;
		return nullptr;
	}
	std::string ext = filepath.substr(dotPos + 1);
	// Преобразуем в нижний регистр
	for (char& c : ext) {
		c = std::tolower(c);
	}
	
	ImageData* image = nullptr;
	
	if (ext == "png") {
		image = png::load_image(filepath);
	} else if (ext == "tga") {
		std::cerr << "[HeightMapUtils] TGA format not supported yet: " << filepath << std::endl;
		return nullptr;
	} else {
		std::cerr << "[HeightMapUtils] Unsupported image format: " << ext << " (file: " << filepath << ")" << std::endl;
		return nullptr;
	}
	
	if (image == nullptr) {
		std::cerr << "[HeightMapUtils] Failed to load image: " << filepath << std::endl;
		return nullptr;
	}
	
	int width = image->getWidth();
	int height = image->getHeight();
	ImageFormat format = image->getFormat();
	unsigned char* pixels = (unsigned char*)image->getData();
	
	// Определяем количество каналов (поддержка grayscale)
	int channels = (format == ImageFormat::rgba8888) ? 4 :
	               (format == ImageFormat::rgb888)   ? 3 : 1;
	
	HeightData2D* result = convertDTMToHeightDataFromImage(pixels, width, height, channels, flip);
	
	// Логируем статистику загруженной карты
	if (result != nullptr) {
		HeightStats stats = getHeightStats(*result);
		std::cout << "[HeightMapUtils] Loaded height map: " << filepath 
		          << " (" << width << "x" << height << ")"
		          << " min=" << stats.min << " max=" << stats.max 
		          << " avg=" << stats.avg << std::endl;
	}
	
	delete image;
	return result;
}

HeightData2D* convertDTMToHeightDataFromImage(void* imageData, int width, int height, int channels, bool flip) {
	if (imageData == nullptr) {
		return nullptr;
	}
	
	HeightData2D* heightData = new HeightData2D(width, height);
	
	unsigned char* pixels = (unsigned char*)imageData;
	
	if (!flip) {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				float heightValue;
				if (channels == 1) {
					// Grayscale - один байт на пиксель
					heightValue = static_cast<float>(pixels[y * width + x]);
				} else {
					// RGB/RGBA - используем красный канал для высоты (как в 7DTD)
					int srcIndex = (y * width + x) * channels;
					heightValue = static_cast<float>(pixels[srcIndex]);
				}
				heightData->at(x, y) = heightValue; // 0..255
			}
		}
	} else {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				float heightValue;
				if (channels == 1) {
					// Grayscale
					heightValue = static_cast<float>(pixels[y * width + x]);
				} else {
					// RGB/RGBA - красный канал
					int srcIndex = (y * width + x) * channels;
					heightValue = static_cast<float>(pixels[srcIndex]);
				}
				// Переворачиваем по Y (верхняя строка становится нижней)
				heightData->at(x, height - y - 1) = heightValue; // 0..255
			}
		}
	}
	
	return heightData;
}

// ==================== Загрузка RAW формата ====================

HeightData2D* loadRAWToHeightData(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "[HeightMapUtils] Failed to open RAW file: " << filepath << std::endl;
		return nullptr;
	}
	
	size_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	
	// Размер карты = sqrt(fileSize / 2) (каждый элемент - 2 байта)
	int size = (int)std::sqrt(fileSize / 2);
	if (size * size * 2 != fileSize) {
		std::cerr << "[HeightMapUtils] Invalid RAW file size: " << fileSize << std::endl;
		file.close();
		return nullptr;
	}
	
	HeightData2D* heightData = new HeightData2D(size, size);
	
	// Читаем данные построчно (little-endian ushort)
	// Формат: данные идут построчно, начиная с верхней строки
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			uint8_t bytes[2];
			file.read((char*)bytes, 2);
			// Little-endian: младший байт первый
			uint16_t value = bytes[0] | (bytes[1] << 8);
			// Конвертируем из ushort (0-65535) в float (0-255)
			heightData->at(x, y) = (float)value * U16_TO_U8;
		}
	}
	
	file.close();
	return heightData;
}

HeightData2D* loadHeightMapRAW(const std::string& filepath, int width, int height, 
                                float factor, int clampHeight) {
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[HeightMapUtils] Failed to open RAW file: " << filepath << std::endl;
		return nullptr;
	}
	
	HeightData2D* heightData = new HeightData2D(width, height);
	
	uint16_t maxValue = (uint16_t)std::min(clampHeight, 65535);
	
	// Читаем данные построчно (little-endian)
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint8_t bytes[2];
			file.read((char*)bytes, 2);
			// Little-endian: младший байт первый
			uint16_t value = bytes[0] | (bytes[1] << 8);
			
			// Ограничиваем значение
			if (value > maxValue) {
				value = maxValue;
			}
			
			// Конвертируем в float (0..255) и применяем фактор
			heightData->at(x, y) = (float)value * U16_TO_U8 * factor;
		}
	}
	
	file.close();
	return heightData;
}

// ==================== Сохранение RAW формата ====================

void saveHeightMapRAW(const std::string& filepath, int width, int height, 
                      const float* data) {
	std::ofstream file(filepath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[HeightMapUtils] Failed to create RAW file: " << filepath << std::endl;
		return;
	}
	
	// Сохраняем построчно
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float heightValue = data[y * width + x];
			// Ограничиваем диапазон и конвертируем из float (0..255) в ushort (0..65535)
			float clamped = std::clamp(heightValue, 0.0f, 255.0f);
			uint16_t ushortValue = (uint16_t)std::lround(clamped * U8_TO_U16);
			
			// Записываем little-endian
			file.write((char*)&ushortValue, 2);
		}
	}
	
	file.close();
}

void saveHeightMapRAW(const std::string& filepath, int width, int height, 
                      const HeightData2D& heightData) {
	saveHeightMapRAW(filepath, width, height, heightData.data.data());
}

void saveHeightMapRAW(const std::string& filepath, int width, int height, 
                      const uint16_t* data) {
	std::ofstream file(filepath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[HeightMapUtils] Failed to create RAW file: " << filepath << std::endl;
		return;
	}
	
	for (int i = 0; i < width * height; i++) {
		file.write((char*)&data[i], 2);
	}
	
	file.close();
}

// ==================== Сглаживание террейна ====================

HeightData2D* smoothTerrain(int passes, const HeightData2D& heightData) {
	HeightData2D* result = new HeightData2D(heightData.width, heightData.height, heightData.data.data());
	smoothTerrainInPlace(passes, *result);
	return result;
}

void smoothTerrainInPlace(int passes, HeightData2D& heightData) {
	int width = heightData.width;
	int height = heightData.height;
	
	HeightData2D temp(width, height);
	
	while (passes > 0) {
		passes--;
		
		// Создаем временную копию
		temp = heightData;
		
		// Применяем сглаживание
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int count = 0;
				float sum = 0.0f;
				
				// Соседние точки (8-связность)
				if (x - 1 >= 0) {
					sum += temp.get(x - 1, y);
					count++;
					if (y - 1 >= 0) {
						sum += temp.get(x - 1, y - 1);
						count++;
					}
					if (y + 1 < height) {
						sum += temp.get(x - 1, y + 1);
						count++;
					}
				}
				if (x + 1 < width) {
					sum += temp.get(x + 1, y);
					count++;
					if (y - 1 >= 0) {
						sum += temp.get(x + 1, y - 1);
						count++;
					}
					if (y + 1 < height) {
						sum += temp.get(x + 1, y + 1);
						count++;
					}
				}
				if (y - 1 >= 0) {
					sum += temp.get(x, y - 1);
					count++;
				}
				if (y + 1 < height) {
					sum += temp.get(x, y + 1);
					count++;
				}
				
				// Усредняем: 50% текущее значение + 50% среднее соседей
				if (count > 0) {
					heightData.at(x, y) = (temp.get(x, y) + sum / (float)count) * 0.5f;
				}
			}
		}
	}
}

// ==================== Конвертация в поле плотности ====================

// Helper функции для безопасной индексации
static inline int wrapIdx(int i, int n) {
	// Корректное wrap для отрицательных индексов
	int r = i % n;
	return (r < 0) ? (r + n) : r;
}

static inline int clampIdx(int i, int n) {
	return (i < 0) ? 0 : (i >= n ? n - 1 : i);
}

// Билинейная интерполяция высоты из карты
static float sampleHeightBilinear(
	const HeightMapUtils::HeightData2D& hm,
	float fx, float fz,
	int edgeMode  // 0=clamp, 1=wrap
) {
	const int W = hm.width;
	const int H = hm.height;
	
	// Базовые индексы
	int x0 = static_cast<int>(std::floor(fx));
	int z0 = static_cast<int>(std::floor(fz));
	float tx = fx - static_cast<float>(x0);
	float tz = fz - static_cast<float>(z0);
	
	int x1 = x0 + 1;
	int z1 = z0 + 1;
	
	// Lambda для безопасной индексации
	auto idx = [&](int x, int z) -> float {
		if (edgeMode == 1) {  // wrap
			return hm.get(wrapIdx(x, W), wrapIdx(z, H));
		} else {  // clamp
			return hm.get(clampIdx(x, W), clampIdx(z, H));
		}
	};
	
	float h00 = idx(x0, z0);
	float h10 = idx(x1, z0);
	float h01 = idx(x0, z1);
	float h11 = idx(x1, z1);
	
	// Билинейная интерполяция
	float hx0 = h00 + (h10 - h00) * tx;
	float hx1 = h01 + (h11 - h01) * tx;
	return hx0 + (hx1 - hx0) * tz;
}

void convertHeightMapToDensityField(const HeightData2D& heightData,
                                     std::vector<float>& densityField,
                                     int chunkX, int chunkY, int chunkZ,
                                     int sizeX, int sizeY, int sizeZ,
                                     float baseHeight, float heightScale,
                                     bool useBilinear, int edgeMode) {
	const int sxDim = sizeX + 1;
	const int syDim = sizeY + 1;
	const int szDim = sizeZ + 1;
	
	densityField.resize(sxDim * syDim * szDim);
	
	for (int y = 0; y < syDim; y++) {
		for (int z = 0; z < szDim; z++) {
			for (int x = 0; x < sxDim; x++) {
				// Мировые координаты (в воксельных единицах)
				// Если карта 1:1 к миру, то wx/wz напрямую соответствуют пикселям карты
				float wx = static_cast<float>(chunkX * sizeX + x);
				float wz = static_cast<float>(chunkZ * sizeZ + z);
				int wy = chunkY * sizeY + y;
				
				// Получаем высоту из карты
				float mh;
				if (useBilinear) {
					// Билинейная интерполяция (убирает "ступеньки")
					mh = sampleHeightBilinear(heightData, wx, wz, edgeMode);
				} else {
					// Точечная выборка (быстрее, но с "ступеньками")
					int sx = (edgeMode == 1) ? wrapIdx(static_cast<int>(std::round(wx)), heightData.width)
					                        : clampIdx(static_cast<int>(std::round(wx)), heightData.width);
					int sz = (edgeMode == 1) ? wrapIdx(static_cast<int>(std::round(wz)), heightData.height)
					                        : clampIdx(static_cast<int>(std::round(wz)), heightData.height);
					mh = heightData.get(sx, sz);
				}
				
				// Применяем масштаб и смещение
				float mapHeight = mh * heightScale + baseHeight;
				
				// Ограничиваем mapHeight для безопасности
				mapHeight = std::clamp(mapHeight, -1000.0f, 10000.0f);
				
				// Плотность: если y < mapHeight, то плотность положительная
				float density = mapHeight - static_cast<float>(wy);
				densityField[(y * szDim + z) * sxDim + x] = density;
			}
		}
	}
}

// ==================== Работа с тайлами ====================

TileInfo getTileInfo(int worldX, int worldZ, int tileSize) {
	TileInfo info;
	info.tileX = (worldX < 0) ? (worldX - tileSize + 1) / tileSize : worldX / tileSize;
	info.tileZ = (worldZ < 0) ? (worldZ - tileSize + 1) / tileSize : worldZ / tileSize;
	info.tileWidth = tileSize;
	info.tileHeight = tileSize;
	return info;
}

HeightData2D* extractTile(const HeightData2D& source, int tileX, int tileZ, 
                           int tileSize, int overlap) {
	int tileWidth = tileSize + overlap * 2;
	int tileHeight = tileSize + overlap * 2;
	
	HeightData2D* tile = new HeightData2D(tileWidth, tileHeight);
	
	int startX = tileX * tileSize - overlap;
	int startZ = tileZ * tileSize - overlap;
	
	for (int z = 0; z < tileHeight; z++) {
		for (int x = 0; x < tileWidth; x++) {
			int srcX = startX + x;
			int srcZ = startZ + z;
			
			// Если координаты выходят за границы, используем ближайшее значение
			srcX = std::max(0, std::min(srcX, source.width - 1));
			srcZ = std::max(0, std::min(srcZ, source.height - 1));
			
			tile->at(x, z) = source.get(srcX, srcZ);
		}
	}
	
	return tile;
}

// ==================== Статистика высотной карты ====================

HeightStats getHeightStats(const HeightData2D& hm) {
	HeightStats s;
	s.min = std::numeric_limits<float>::max();
	s.max = std::numeric_limits<float>::lowest();
	double acc = 0.0;
	const int N = hm.width * hm.height;
	
	// Прямой доступ к data для производительности
	for (int i = 0; i < N; ++i) {
		float v = hm.data[i];
		if (v < s.min) s.min = v;
		if (v > s.max) s.max = v;
		acc += v;
	}
	
	s.avg = (N > 0) ? static_cast<float>(acc / static_cast<double>(N)) : 0.0f;
	return s;
}

} // namespace HeightMapUtils

