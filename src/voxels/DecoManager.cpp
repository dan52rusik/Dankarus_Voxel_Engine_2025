#include "DecoManager.h"
#include "ChunkManager.h"
#include "GameUtils.h"
#include "../noise/OpenSimplex.h"
#include "../files/files.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <cmath>
#include <unordered_set>
#include <climits>
#ifdef _WIN32
const char PATH_SEP = '\\';
#else
const char PATH_SEP = '/';
#endif

DecoManager::DecoManager()
	: isEnabled(true), isHidden(false),
	  worldWidth(0), worldHeight(0), worldWidthHalf(0), worldHeightHalf(0),
	  chunkManager(nullptr),
	  checkDelayTicks(0), bDirty(false), chunkDistance(3) {
}

DecoManager::~DecoManager() {
	onWorldUnloaded();
}

void DecoManager::onWorldLoaded(int _worldWidth, int _worldHeight, ChunkManager* _chunkManager, const std::string& worldPath, int64_t seed) {
	if (!isEnabled) {
		return;
	}
	
	worldWidth = _worldWidth;
	worldHeight = _worldHeight;
	worldWidthHalf = worldWidth / 2;
	worldHeightHalf = worldHeight / 2;
	chunkManager = _chunkManager;
	
	// Создаём карту занятости
	occupiedMap = std::make_unique<DecoOccupiedMap>(worldWidth, worldHeight);
	
	// Создаём шум для ресурсов
	resourceNoise = std::make_unique<OpenSimplex3D>(seed + 1000);
	
	// Вычисляем границы чанков декораций
	int decoChunkPos1 = DecoChunk::toDecoChunkPos(-worldWidthHalf);
	int decoChunkPos2 = DecoChunk::toDecoChunkPos(worldWidthHalf);
	int decoChunkPos3 = DecoChunk::toDecoChunkPos(-worldHeightHalf);
	int decoChunkPos4 = DecoChunk::toDecoChunkPos(worldHeightHalf);
	
	// Создаём все чанки декораций
	for (int x = decoChunkPos1; x <= decoChunkPos2; x++) {
		for (int z = decoChunkPos3; z <= decoChunkPos4; z++) {
			int key = DecoChunk::makeKey16(x, z);
			decoChunks[key] = std::make_unique<DecoChunk>(x, z);
		}
	}
	
	// Путь к файлу сохранения
	if (!worldPath.empty()) {
		filenamePath = worldPath + PATH_SEP + "decoration.bin";
		bool fileLoaded = tryLoad();
		std::cout << "[DECO] Loaded " << (loadedDecos.size()) << " decorations from file" << std::endl;
	} else {
		filenamePath = "worlds/default/decoration.bin";
	}
	
	// Генерируем декорации для всех чанков (если не загружены)
	if (loadedDecos.empty()) {
		std::cout << "[DECO] Generating decorations for all chunks..." << std::endl;
		
		int chunkStartX = (-worldWidthHalf) / CHUNK_SIZE;
		int chunkEndX = worldWidthHalf / CHUNK_SIZE;
		int chunkStartZ = (-worldHeightHalf) / CHUNK_SIZE;
		int chunkEndZ = worldHeightHalf / CHUNK_SIZE;
		
		int totalDecorated = 0;
		for (int z = chunkStartZ; z <= chunkEndZ; z++) {
			for (int x = chunkStartX; x <= chunkEndX; x++) {
				int key = DecoChunk::makeKey16(x, z);
				auto it = decoChunks.find(key);
				if (it != decoChunks.end()) {
					// Используем seed на основе позиции чанка для детерминированности
					int64_t chunkSeed = seed + static_cast<int64_t>(x) * 1000 + static_cast<int64_t>(z);
					totalDecorated += decorateChunkRandom(it->second.get(), chunkSeed);
				}
			}
		}
		
		std::cout << "[DECO] Generated " << totalDecorated << " decorations" << std::endl;
	} else {
		// Добавляем загруженные декорации
		for (const auto& deco : loadedDecos) {
			addLoadedDecoration(deco);
		}
		loadedDecos.clear();
	}
	
	bDirty = true;
}

void DecoManager::onWorldUnloaded() {
	if (!isEnabled) {
		return;
	}
	
	// Сохраняем перед выгрузкой
	save();
	
	// Очищаем все чанки
	for (auto& pair : decoChunks) {
		pair.second->destroy();
	}
	decoChunks.clear();
	visibleDecoChunks.clear();
	loadedDecos.clear();
	occupiedMap.reset();
	resourceNoise.reset();
	checkDelayTicks = 0;
	bDirty = false;
}

void DecoManager::updateTick(const glm::vec3& cameraPos, int renderDistance) {
	if (!isEnabled || isHidden) {
		return;
	}
	
	checkDelayTicks--;
	if (checkDelayTicks > 0) {
		return;
	}
	
	checkDelayTicks = 20; // Обновляем каждые 20 тиков
	
	updateDecorations(cameraPos, renderDistance);
}

void DecoManager::updateDecorations(const glm::vec3& cameraPos, int renderDistance) {
	// Очищаем список видимых чанков
	visibleDecoChunks.clear();
	
	// Вычисляем координаты чанка декораций для камеры
	int cameraDecoChunkX = DecoChunk::toDecoChunkPos(static_cast<int>(cameraPos.x));
	int cameraDecoChunkZ = DecoChunk::toDecoChunkPos(static_cast<int>(cameraPos.z));
	
	// Расстояние в чанках декораций
	int decoDistance = std::max(1, renderDistance / 2); // Примерно половина от renderDistance
	
	// Собираем чанки вокруг камеры
	std::unordered_set<int> chunksAroundCamera;
	for (int x = cameraDecoChunkX - decoDistance; x <= cameraDecoChunkX + decoDistance; x++) {
		for (int z = cameraDecoChunkZ - decoDistance; z <= cameraDecoChunkZ + decoDistance; z++) {
			chunksAroundCamera.insert(DecoChunk::makeKey16(x, z));
		}
	}
	
	// Обновляем видимость существующих чанков
	for (auto& pair : decoChunks) {
		bool shouldBeVisible = chunksAroundCamera.find(pair.first) != chunksAroundCamera.end();
		pair.second->setVisible(shouldBeVisible);
		
		if (shouldBeVisible) {
			visibleDecoChunks.push_back(pair.second.get());
			
			// Генерируем декорации, если чанк ещё не декорирован
			if (!pair.second->isDecorated && chunkManager != nullptr) {
				// Используем seed на основе позиции чанка
				int64_t seed = 1337; // Базовый seed (можно получить из worldSave)
				decorateChunkRandom(pair.second.get(), seed + pair.first);
			}
		} else {
			// Уничтожаем невидимые чанки (опционально, для экономии памяти)
			// pair.second->destroy();
		}
	}
}

void DecoManager::getDecorationsOnChunk(int chunkX, int chunkZ, std::vector<DecoObject>& decoList) {
	decoList.clear();
	
	if (!isEnabled) {
		return;
	}
	
	// Вычисляем координаты чанка декораций
	int decoChunkX = DecoChunk::toDecoChunkPos(chunkX * MCChunk::CHUNK_SIZE_X);
	int decoChunkZ = DecoChunk::toDecoChunkPos(chunkZ * MCChunk::CHUNK_SIZE_Z);
	
	int key = DecoChunk::makeKey16(decoChunkX, decoChunkZ);
	auto it = decoChunks.find(key);
	if (it == decoChunks.end()) {
		return;
	}
	
	DecoChunk* decoChunk = it->second.get();
	
	// Генерируем декорации, если чанк ещё не декорирован
	if (!decoChunk->isDecorated && chunkManager != nullptr) {
		int64_t seed = 1337; // Базовый seed
		decorateChunkRandom(decoChunk, seed + key);
	}
	
	// Вычисляем ключ маленького чанка (16x16)
	int64_t smallChunkKey = (static_cast<int64_t>(chunkX) << 32) | (static_cast<uint64_t>(chunkZ) & 0xFFFFFFFF);
	
	// Получаем декорации для этого маленького чанка
	auto smallChunkIt = decoChunk->decosPerSmallChunks.find(smallChunkKey);
	if (smallChunkIt != decoChunk->decosPerSmallChunks.end()) {
		for (const auto& deco : smallChunkIt->second) {
			if (deco.state != DecoState::GeneratedInactive) {
				decoList.push_back(deco);
			}
		}
	}
}

void DecoManager::addDecorationAt(const glm::ivec3& pos, uint8_t blockId, uint8_t rotation, bool forceBlockYPos) {
	if (!isEnabled) {
		return;
	}
	
	float realYPos = static_cast<float>(pos.y);
	
	// Если не принудительная позиция, получаем высоту террейна
	if (!forceBlockYPos && pos.y > 0 && chunkManager != nullptr) {
		realYPos = chunkManager->evalSurfaceHeight(static_cast<float>(pos.x), static_cast<float>(pos.z)) + 1.0f;
	}
	
	bDirty = true;
	
	DecoChunk* decoChunk = getDecoChunkAt(pos.x, pos.z);
	if (decoChunk != nullptr) {
		// Удаляем существующую декорацию, если есть
		DecoObject* existing = decoChunk->getDecoObjectAt(pos);
		if (existing != nullptr) {
			decoChunk->removeDecoObject(*existing);
		}
		
		// Добавляем новую
		DecoObject deco;
		deco.init(pos, realYPos, blockId, rotation, DecoState::Dynamic);
		decoChunk->addDecoObject(deco, true);
	} else {
		// Чанк не существует, добавляем в загруженные (для последующей загрузки)
		if (loadedDecos.empty()) {
			// Инициализируем, если нужно
		}
		DecoObject deco;
		deco.init(pos, realYPos, blockId, rotation, DecoState::Dynamic);
		loadedDecos.insert(deco);
	}
}

bool DecoManager::removeDecorationAt(const glm::ivec3& pos) {
	if (!isEnabled) {
		return false;
	}
	
	DecoChunk* decoChunk = getDecoChunkAt(pos.x, pos.z);
	if (decoChunk == nullptr) {
		return false;
	}
	
	bDirty = true;
	return decoChunk->removeDecoObject(pos);
}

EnumDecoOccupied DecoManager::getDecoOccupiedAt(int x, int z) {
	if (!isEnabled || occupiedMap == nullptr) {
		return EnumDecoOccupied::Free;
	}
	
	int offs = checkPosition(worldWidth, worldHeight, x, z);
	if (offs < 0) {
		return EnumDecoOccupied::NoneAllowed;
	}
	
	// Получаем чанк и проверяем, декорирован ли он
	DecoChunk* decoChunk = getDecoChunkAt(x, z);
	if (decoChunk != nullptr && !decoChunk->isDecorated) {
		// Генерируем декорации, если нужно
		if (chunkManager != nullptr) {
			int64_t seed = 1337;
			int key = DecoChunk::makeKey16(DecoChunk::toDecoChunkPos(x), DecoChunk::toDecoChunkPos(z));
			decorateChunkRandom(decoChunk, seed + key);
		}
	}
	
	return occupiedMap->get(x, z);
}

void DecoManager::setChunkDistance(int distance) {
	chunkDistance = distance;
}

BiomeDefinition::BiomeType DecoManager::getBiomeAt(int x, int z) const {
	if (chunkManager == nullptr) {
		return BiomeDefinition::BiomeType::Plains;
	}
	return chunkManager->getBiomeAt(static_cast<float>(x), static_cast<float>(z));
}

DecoChunk* DecoManager::getDecoChunkAt(int x, int z) {
	int decoChunkX = DecoChunk::toDecoChunkPos(x);
	int decoChunkZ = DecoChunk::toDecoChunkPos(z);
	int key = DecoChunk::makeKey16(decoChunkX, decoChunkZ);
	
	auto it = decoChunks.find(key);
	if (it == decoChunks.end()) {
		return nullptr;
	}
	
	return it->second.get();
}

bool DecoManager::tryAddToOccupiedMap(uint8_t blockId, int xWorld, int zWorld, uint8_t rotation, bool enableStopBigDecoCheck) {
	if (occupiedMap == nullptr) {
		return true;
	}
	
	// Упрощённая версия: считаем все блоки размером 1x1
	// В полной версии нужно учитывать multiBlock и rotation
	
	if (enableStopBigDecoCheck) {
		if (occupiedMap->get(xWorld, zWorld) == EnumDecoOccupied::Stop_BigDeco) {
			return false;
		}
	}
	
	occupiedMap->set(xWorld, zWorld, EnumDecoOccupied::Deco);
	return true;
}

void DecoManager::addLoadedDecoration(const DecoObject& deco) {
	if (!isEnabled) {
		return;
	}
	
	DecoChunk* decoChunk = getDecoChunkAt(deco.pos.x, deco.pos.z);
	if (decoChunk == nullptr) {
		return;
	}
	
	decoChunk->addDecoObject(deco);
	
	if (deco.state != DecoState::Dynamic) {
		// Добавляем в карту занятости
		tryAddToOccupiedMap(deco.blockId, deco.pos.x, deco.pos.z, deco.rotation, false);
	}
	
	bDirty = true;
}

int DecoManager::checkPosition(int worldWidth, int worldHeight, int x, int z) {
	int halfWidth = worldWidth / 2;
	int halfHeight = worldHeight / 2;
	
	if (x < -halfWidth || x >= halfWidth || z < -halfHeight || z >= halfHeight) {
		return -1;
	}
	
	return (x + halfWidth) + (z + halfHeight) * worldWidth;
}

// Генерация декораций для чанка
int DecoManager::decorateChunkRandom(DecoChunk* decoChunk, int64_t seed) {
	if (decoChunk == nullptr || chunkManager == nullptr) {
		return 0;
	}
	
	if (decoChunk->isDecorated) {
		return 0;
	}
	
	// Создаём генератор случайных чисел на основе seed
	std::mt19937 rng(static_cast<unsigned int>(seed));
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	
	// Границы чанка в мировых координатах
	int minX = decoChunk->decoChunkX * CHUNK_SIZE;
	int maxX = minX + CHUNK_SIZE;
	int minZ = decoChunk->decoChunkZ * CHUNK_SIZE;
	int maxZ = minZ + CHUNK_SIZE;
	
	int decoratedCount = 0;
	
	// Пробуем разместить декорации (до 1000 попыток на чанк)
	for (int attempt = 0; attempt < 1000; attempt++) {
		// Случайная позиция в чанке
		int x = minX + (rng() % CHUNK_SIZE);
		int z = minZ + (rng() % CHUNK_SIZE);
		
		// Проверяем занятость
		if (occupiedMap != nullptr) {
			EnumDecoOccupied occupied = occupiedMap->get(x, z);
			if (occupied != EnumDecoOccupied::Free && occupied != EnumDecoOccupied::Perimeter) {
				continue;
			}
		}
		
		// Получаем биом в этой точке
		BiomeDefinition::BiomeType biome = getBiomeAt(x, z);
		
		// Получаем высоту террейна
		float terrainHeight = chunkManager->evalSurfaceHeight(static_cast<float>(x), static_cast<float>(z));
		int y = static_cast<int>(terrainHeight + 1.0f);
		
		// Определяем тип декорации на основе биома
		uint8_t blockId = 0;
		float probability = 0.0f;
		int checkResourceOffsetY = INT_MAX; // Смещение для проверки руд (по умолчанию не проверяем)
		
		switch (biome) {
			case BiomeDefinition::BiomeType::Forest:
			case BiomeDefinition::BiomeType::PineForest:
				// Деревья в лесах
				blockId = 2; // Предполагаем, что ID 2 = дерево
				probability = 0.15f; // 15% вероятность
				break;
			case BiomeDefinition::BiomeType::Plains:
				// Трава/камни на равнинах
				blockId = 3; // Предполагаем, что ID 3 = трава/камень
				probability = 0.08f; // 8% вероятность
				// Проверяем руды на глубине -5 блоков
				checkResourceOffsetY = -5;
				break;
			case BiomeDefinition::BiomeType::Desert:
				// Кактусы/камни в пустыне
				blockId = 4; // Предполагаем, что ID 4 = кактус/камень
				probability = 0.05f; // 5% вероятность
				checkResourceOffsetY = -3;
				break;
			case BiomeDefinition::BiomeType::Snow:
				// Снежные блоки/камни
				blockId = 5; // Предполагаем, что ID 5 = снежный блок
				probability = 0.1f; // 10% вероятность
				break;
			default:
				// Для других биомов - низкая вероятность
				blockId = 3;
				probability = 0.03f;
				break;
		}
		
		// Проверяем руды, если нужно
		if (checkResourceOffsetY < INT_MAX && resourceNoise != nullptr) {
			int checkY = y + checkResourceOffsetY;
			if (!GameUtils::CheckOreNoiseAt(*resourceNoise, x, checkY, z)) {
				continue; // Пропускаем, если нет руды
			}
		}
		
		// Проверяем вероятность размещения
		if (dist(rng) > probability) {
			continue;
		}
		
		// Случайный поворот
		uint8_t rotation = static_cast<uint8_t>(rng() % 4);
		
		// Пытаемся добавить в карту занятости
		if (!tryAddToOccupiedMap(blockId, x, z, rotation, true)) {
			continue;
		}
		
		// Создаём декорацию
		DecoObject deco;
		deco.init(glm::ivec3(x, y, z), terrainHeight + 1.0f, blockId, rotation, DecoState::GeneratedActive);
		
		// Добавляем в чанк
		decoChunk->addDecoObject(deco);
		decoratedCount++;
	}
	
	decoChunk->isDecorated = true;
	return decoratedCount;
}

bool DecoManager::tryLoad() {
	if (filenamePath.empty() || !files::file_exists(filenamePath)) {
		return false;
	}
	
	std::ifstream file(filenamePath, std::ios::binary);
	if (!file.is_open()) {
		return false;
	}
	
	// Читаем версию
	uint8_t version;
	file.read(reinterpret_cast<char*>(&version), sizeof(uint8_t));
	
	if (version != FILE_VERSION) {
		std::cout << "[DECO] Saved decoration data is out of date. Version: " << static_cast<int>(version) 
		          << ", current: " << FILE_VERSION << ". Regenerating..." << std::endl;
		file.close();
		return false;
	}
	
	read(file, version);
	file.close();
	
	return true;
}

void DecoManager::save() {
	if (!isEnabled || !bDirty || filenamePath.empty()) {
		return;
	}
	
	std::ofstream file(filenamePath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[DECO] Failed to open file for writing: " << filenamePath << std::endl;
		return;
	}
	
	// Записываем версию
	uint8_t version = FILE_VERSION;
	file.write(reinterpret_cast<const char*>(&version), sizeof(uint8_t));
	
	write(file);
	file.close();
	
	bDirty = false;
	std::cout << "[DECO] Saved decorations to " << filenamePath << std::endl;
}

void DecoManager::read(std::istream& stream, int version) {
	loadedDecos.clear();
	
	int count;
	stream.read(reinterpret_cast<char*>(&count), sizeof(int));
	
	for (int i = 0; i < count; i++) {
		DecoObject deco;
		deco.read(stream);
		loadedDecos.insert(deco);
	}
}

void DecoManager::write(std::ostream& stream) const {
	// Генерируем список для записи
	std::vector<DecoObject> writeList;
	generateDecoWriteList(writeList);
	
	// Записываем количество
	int count = static_cast<int>(writeList.size());
	stream.write(reinterpret_cast<const char*>(&count), sizeof(int));
	
	// Записываем все декорации
	for (const auto& deco : writeList) {
		deco.write(stream);
	}
}

void DecoManager::generateDecoWriteList(std::vector<DecoObject>& writeList) const {
	writeList.clear();
	
	for (const auto& pair : decoChunks) {
		for (const auto& smallChunkPair : pair.second->decosPerSmallChunks) {
			for (const auto& deco : smallChunkPair.second) {
				writeList.push_back(deco);
			}
		}
	}
}

