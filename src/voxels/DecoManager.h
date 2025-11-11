#ifndef VOXELS_DECOMANAGER_H_
#define VOXELS_DECOMANAGER_H_

#include "DecoChunk.h"
#include "DecoObject.h"
#include "DecoOccupiedMap.h"
#include "BiomeDefinition.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <string>
#include <memory>
#include <mutex>

// Forward declarations
class ChunkManager;
class OpenSimplex3D;
class WorldSave;

// Менеджер декораций
// Адаптировано из 7 Days To Die DecoManager
class DecoManager {
public:
	static const int FILE_VERSION = 6;
	static const int CHUNK_SIZE = 128; // Размер чанка декораций (128x128)
	static constexpr float UPDATE_DELAY = 1.0f;
	static const int UPDATE_CO_MAX_TIME_US = 900;
	
	bool isEnabled;
	bool isHidden;
	
	DecoManager();
	~DecoManager();
	
	// Инициализация при загрузке мира
	void onWorldLoaded(int worldWidth, int worldHeight, ChunkManager* chunkManager, const std::string& worldPath, int64_t seed);
	
	// Выгрузка мира
	void onWorldUnloaded();
	
	// Обновление (вызывается каждый тик)
	void updateTick(const glm::vec3& cameraPos, int renderDistance);
	
	// Получить декорации в чанке (для генерации воксельных блоков)
	void getDecorationsOnChunk(int chunkX, int chunkZ, std::vector<DecoObject>& decoList);
	
	// Добавить декорацию
	void addDecorationAt(const glm::ivec3& pos, uint8_t blockId, uint8_t rotation = 0, bool forceBlockYPos = false);
	
	// Удалить декорацию
	bool removeDecorationAt(const glm::ivec3& pos);
	
	// Получить занятость в точке
	EnumDecoOccupied getDecoOccupiedAt(int x, int z);
	
	// Сохранение/загрузка
	void save();
	bool tryLoad();
	
	// Установить расстояние отрисовки
	void setChunkDistance(int distance);
	
	// Получить биом в точке (для генерации декораций)
	BiomeDefinition::BiomeType getBiomeAt(int x, int z) const;
	
private:
	// Чанки декораций
	std::unordered_map<int, std::unique_ptr<DecoChunk>> decoChunks;
	
	// Видимые чанки
	std::vector<DecoChunk*> visibleDecoChunks;
	
	// Загруженные декорации (для инициализации)
	std::unordered_set<DecoObject, DecoObjectHash> loadedDecos;
	
	// Параметры мира
	int worldWidth;
	int worldHeight;
	int worldWidthHalf;
	int worldHeightHalf;
	
	// Карта занятости
	std::unique_ptr<DecoOccupiedMap> occupiedMap;
	
	// Ссылки на другие системы
	ChunkManager* chunkManager;
	std::unique_ptr<OpenSimplex3D> resourceNoise; // Шум для ресурсов
	
	// Параметры обновления
	int checkDelayTicks;
	bool bDirty;
	
	// Путь к файлу сохранения
	std::string filenamePath;
	
	// Расстояние отрисовки
	int chunkDistance;
	
	// Генерация декораций для чанка
	int decorateChunkRandom(DecoChunk* decoChunk, int64_t seed);
	
	// Получить чанк декораций
	DecoChunk* getDecoChunkAt(int x, int z);
	
	// Попытаться добавить в карту занятости
	bool tryAddToOccupiedMap(uint8_t blockId, int xWorld, int zWorld, uint8_t rotation, bool enableStopBigDecoCheck);
	
	// Обновление видимости декораций
	void updateDecorations(const glm::vec3& cameraPos, int renderDistance);
	
	// Добавить загруженную декорацию
	void addLoadedDecoration(const DecoObject& deco);
	
	// Чтение/запись
	void read(std::istream& stream, int version);
	void write(std::ostream& stream) const;
	
	// Генерация списка для записи
	void generateDecoWriteList(std::vector<DecoObject>& writeList) const;
	
	// Проверка позиции
	static int checkPosition(int worldWidth, int worldHeight, int x, int z);
};

#endif /* VOXELS_DECOMANAGER_H_ */

