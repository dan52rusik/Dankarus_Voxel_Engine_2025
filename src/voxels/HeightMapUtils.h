#ifndef VOXELS_HEIGHTMAPUTILS_H_
#define VOXELS_HEIGHTMAPUTILS_H_

#include <string>
#include <vector>
#include <cstdint>

// Утилиты для работы с высотными картами (адаптировано из 7 Days To Die)
namespace HeightMapUtils {
	// Константы
	constexpr float GAME_HEIGHT_TO_U16_SCALE = 257.0f;
	constexpr float U8_TO_U16 = 257.0f;         // 0..255 -> 0..65535
	constexpr float U16_TO_U8 = 1.0f / 257.0f;  // 0..65535 -> 0..255
	
	// 2D массив высот (width, height)
	class HeightData2D {
	public:
		int width;
		int height;
		std::vector<float> data;
		
		HeightData2D(int w, int h);
		HeightData2D(int w, int h, const float* src);
		~HeightData2D();
		
		float& at(int x, int y);
		const float& at(int x, int y) const;
		float get(int x, int y) const;
		void set(int x, int y, float value);
	};
	
	// Загрузка DTM (Digital Terrain Model) из PNG/TGA
	// Возвращает массив высот [width][height], где значения от 0 до 255
	// flip: true = перевернуть по Y (стандарт для большинства DTM редакторов)
	HeightData2D* convertDTMToHeightData(const std::string& filepath, bool flip = true);
	
	// Загрузка DTM из ImageData (уже загруженного изображения)
	// Полное определение только в .cpp файле
	HeightData2D* convertDTMToHeightDataFromImage(void* imageData, int width, int height, int channels, bool flip = false);
	
	// Загрузка RAW формата (16-bit unsigned short, little-endian)
	// Формат: массив ushort значений, размер = sqrt(fileSize / 2)
	HeightData2D* loadRAWToHeightData(const std::string& filepath);
	
	// Загрузка RAW с указанными размерами
	HeightData2D* loadHeightMapRAW(const std::string& filepath, int width, int height, 
	                                float factor = 1.0f, int clampHeight = 65535);
	
	// Сохранение высотной карты в RAW формат
	void saveHeightMapRAW(const std::string& filepath, int width, int height, 
	                      const float* data);
	void saveHeightMapRAW(const std::string& filepath, int width, int height, 
	                      const HeightData2D& heightData);
	
	// Сохранение высотной карты в RAW формат (16-bit ushort)
	void saveHeightMapRAW(const std::string& filepath, int width, int height, 
	                      const uint16_t* data);
	
	// Сглаживание террейна (среднее значение соседних точек)
	// Passes - количество проходов сглаживания
	HeightData2D* smoothTerrain(int passes, const HeightData2D& heightData);
	
	// Применить сглаживание к существующему массиву
	void smoothTerrainInPlace(int passes, HeightData2D& heightData);
	
	// Конвертация высотной карты в поле плотности для Marching Cubes
	// heightData - 2D карта высот
	// densityField - выходной массив плотности [sizeX * sizeY * sizeZ]
	// chunkX, chunkY, chunkZ - позиция чанка
	// sizeX, sizeY, sizeZ - размеры чанка
	// baseHeight - базовая высота для масштабирования
	// heightScale - масштаб высот (умножается на значения из карты)
	// useBilinear - использовать билинейную интерполяцию (убирает "ступеньки")
	// edgeMode - режим обработки краев: 0=clamp, 1=wrap (для тайлинга)
	void convertHeightMapToDensityField(const HeightData2D& heightData,
	                                     std::vector<float>& densityField,
	                                     int chunkX, int chunkY, int chunkZ,
	                                     int sizeX, int sizeY, int sizeZ,
	                                     float baseHeight = 0.0f, 
	                                     float heightScale = 1.0f,
	                                     bool useBilinear = true,
	                                     int edgeMode = 0);
	
	// Получить статистику высотной карты (min, max, average)
	struct HeightStats {
		float min;
		float max;
		float avg;  // average (сокращение для краткости)
	};
	HeightStats getHeightStats(const HeightData2D& heightData);
	
	// Работа с тайлами (разбиение больших карт на части)
	struct TileInfo {
		int tileX;
		int tileZ;
		int tileWidth;
		int tileHeight;
	};
	
	// Получить информацию о тайле для координаты
	TileInfo getTileInfo(int worldX, int worldZ, int tileSize);
	
	// Извлечь тайл из большой высотной карты
	HeightData2D* extractTile(const HeightData2D& source, int tileX, int tileZ, 
	                           int tileSize, int overlap = 0);
}

#endif /* VOXELS_HEIGHTMAPUTILS_H_ */

