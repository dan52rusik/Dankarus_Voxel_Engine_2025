#ifndef VOXELS_CHUNKMANAGER_H_
#define VOXELS_CHUNKMANAGER_H_

#include "MCChunk.h"
#include "voxel.h"
#include "noise/OpenSimplex.h"
#include "HeightMapUtils.h"
#include "BiomeDefinition.h"
#include "GeneratorParams.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <limits>
#include <memory>

// Forward declaration
namespace HitInfo {
	struct HitInfoDetails;
}
class BiomeProviderFromImage;

class ChunkManager {
public:
	ChunkManager();
	~ChunkManager();
	
	// Обновляет видимые чанки вокруг камеры
	void update(const glm::vec3& cameraPos, int renderDistance);
	
	// Получить все видимые чанки для отрисовки
	std::vector<MCChunk*> getVisibleChunks() const;
	
	// Получить все загруженные чанки (для сохранения)
	std::vector<MCChunk*> getAllChunks() const;
	
	// Параметры генерации
	void setNoiseParams(float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation);
	void getNoiseParams(float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation) const;
	void setSeed(int64_t seed); // Установить seed для генерации мира
	void setWaterLevel(float waterLevel); // Установить базовый уровень воды (в мировых координатах)
	float getWaterLevel() const { return waterLevel; }
	
	// Унифицированная конфигурация через GeneratorParams
	void configure(const GeneratorParams& params);
	
	// Получить уровень воды в точке (x, z) с учетом типа водоема
	// Возвращает уровень воды: море > озеро > река > суша
	float getWaterLevelAt(int worldX, int worldZ) const;
	
	// Вычислить высоту поверхности в точке (wx, wz) используя ту же формулу, что и в генерации террейна
	// Используется для согласования воды с рельефом
	float evalSurfaceHeight(float wx, float wz) const;
	
	// Получить биом в точке (wx, wz)
	BiomeDefinition::BiomeType getBiomeAt(float wx, float wz) const;
	
	// Установить провайдер биомов из изображения
	void setBiomeProviderFromImage(BiomeProviderFromImage* provider);
	
	// Получить провайдер биомов из изображения
	BiomeProviderFromImage* getBiomeProviderFromImage() const { return biomeProviderFromImage; }
	
	// Получить менеджер декораций (для интеграции)
	class DecoManager* getDecoManager() const { return decoManager; }
	void setDecoManager(class DecoManager* deco) { decoManager = deco; }
	
	// Получить WorldBuilder (для интеграции)
	class WorldBuilder* getWorldBuilder() const { return worldBuilder; }
	void setWorldBuilder(class WorldBuilder* builder) { worldBuilder = builder; }
	
	// Получить симулятор воды
	class WaterSimulator* getWaterSimulator() const { return waterSimulator; }
	void setWaterSimulator(class WaterSimulator* simulator) { waterSimulator = simulator; }
	
	// Получить менеджер испарения воды
	class WaterEvaporationManager* getWaterEvaporationManager() const { return waterEvaporationManager; }
	void setWaterEvaporationManager(class WaterEvaporationManager* manager) { waterEvaporationManager = manager; }
	
	// Обновить симуляцию воды (вызывается из игрового цикла)
	void updateWaterSimulation(float deltaTime);
	
	// Получить чанк по ключу (для внутреннего использования)
	MCChunk* getChunk(const std::string& chunkKey) const;
	
	// Работа с высотными картами
	void setHeightMap(const std::string& filepath); // Загрузить высотную карту из файла (PNG/RAW)
	void setHeightMap(HeightMapUtils::HeightData2D* heightMap); // Установить высотную карту напрямую
	void clearHeightMap(); // Очистить высотную карту (вернуться к процедурной генерации)
	bool hasHeightMap() const { return heightMap != nullptr; }
	void setHeightMapScale(float baseHeight, float heightScale); // Настройка масштаба высотной карты
	
	// Очистка всех чанков (для создания нового мира)
	void clear();
	
	// Система воксельных блоков
	voxel* getVoxel(int x, int y, int z); // Получить воксель по мировым координатам
	void setVoxel(int x, int y, int z, uint8_t id); // Установить воксель по мировым координатам
	voxel* rayCast(const glm::vec3& start, const glm::vec3& dir, float maxDist, glm::vec3& end, glm::vec3& norm, glm::ivec3& iend); // Raycast для определения блока
	bool rayCastSurface(const glm::vec3& start, const glm::vec3& dir, float maxDist, glm::vec3& hitPos, glm::vec3& hitNorm); // Raycast по поверхности Marching Cubes
	bool rayCastDetailed(const glm::vec3& start, const glm::vec3& dir, float maxDist, HitInfo::HitInfoDetails& hitInfo); // Raycast с полной информацией о попадании
	
	// Сохранение/загрузка чанков
	void setWorldSave(class WorldSave* worldSave, const std::string& worldPath); // Установить WorldSave и путь к миру для автосохранения
	void saveDirtyChunks(); // Сохранить все измененные чанки (для выхода из мира)
	void saveDirtyChunksBudgeted(int maxPerCall); // Сохранить измененные чанки с ограничением (для периодического вызова)
	
	// Границы мира (в координатах чанков)
	void setWorldBoundsByMeters(int worldSizeXZ_Meters); // Установить границы мира в метрах (10к×10к)
	inline bool isOutsideBounds(int cx, int cz) const {
		return (cx < minChunkX || cx > maxChunkX || cz < minChunkZ || cz > maxChunkZ);
	}
	// Геттеры для границ мира (для использования в WorldManager)
	int getMinChunkX() const { return minChunkX; }
	int getMaxChunkX() const { return maxChunkX; }
	int getMinChunkZ() const { return minChunkZ; }
	int getMaxChunkZ() const { return maxChunkZ; }
	
private:
	std::unordered_map<std::string, MCChunk*> chunks;
	OpenSimplex3D noise;
	
	// Параметры генерации
	float baseFreq;
	int octaves;
	float lacunarity;
	float gain;
	float baseHeight;
	float heightVariation;
	float waterLevel; // Уровень воды в мировых координатах
	
	// Высотная карта (опционально)
	std::unique_ptr<HeightMapUtils::HeightData2D> heightMap;
	float heightMapBaseHeight; // Базовая высота для масштабирования
	float heightMapScale; // Масштаб высот (умножается на значения из карты)
	
	// Сохранение/загрузка
	class WorldSave* worldSave = nullptr; // Указатель на WorldSave для сохранения чанков
	std::string worldPath; // Путь к миру для сохранения чанков
	
	// Менеджер декораций
	class DecoManager* decoManager = nullptr;
	
	// WorldBuilder для применения дорог/озер/префабов
	class WorldBuilder* worldBuilder = nullptr;
	
	// Симулятор воды
	class WaterSimulator* waterSimulator = nullptr;
	
	// Менеджер испарения воды
	class WaterEvaporationManager* waterEvaporationManager = nullptr;
	
	// Провайдер биомов из изображения (опционально)
	class BiomeProviderFromImage* biomeProviderFromImage = nullptr;
	
	// Границы мира (в координатах чанков)
	int minChunkX = -156, maxChunkX = 156;  // По умолчанию: 10к×10к (312 чанков по оси)
	int minChunkZ = -156, maxChunkZ = 156;
	
	// Таймер для периодического сохранения
	double lastSaveTime = 0.0;
	
	// Вспомогательные функции
	std::string chunkKey(int cx, int cy, int cz) const;
	glm::ivec3 worldToChunk(const glm::vec3& worldPos) const;
	void generateChunk(int cx, int cy, int cz);
	void unloadDistantChunks(const glm::vec3& cameraPos, int renderDistance);
	bool saveChunk(MCChunk* chunk); // Сохранить чанк на диск
	bool loadChunk(int cx, int cy, int cz, MCChunk*& chunk); // Загрузить чанк с диска
	std::string getChunkFilePath(int cx, int cy, int cz) const; // Получить путь к файлу чанка
};

#endif /* VOXELS_CHUNKMANAGER_H_ */

