#include "ChunkManager.h"
#include "HitInfo.h"
#include "VoxelUtils.h"
#include "WorldSave.h"
#include "BiomeDefinition.h"
#include "BiomeProviderFromImage.h"
#include "DecoObject.h"
#include "DecoManager.h"
#include "WorldBuilder.h"
#include "WaterSimulator.h"
#include "WaterEvaporationManager.h"
#include "WaterUtils.h"
#include "../maths/voxmaths.h"
#include "../graphics/MarchingCubes.h"
#include "../files/files.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <string>
#include <memory>
#include <cctype>
#include <sstream>
#include <fstream>
#include <cfloat>  // для std::isfinite
#include <ctime>   // для time()
#include <algorithm> // для std::sort
#include <deque>     // для meshBuildQueue
#ifdef _WIN32
const char PATH_SEP = '\\';
#else
const char PATH_SEP = '/';
#endif

ChunkManager::ChunkManager() 
	: noise(1337), baseFreq(1.0f / 256.0f), octaves(5), lacunarity(2.0f), gain(0.5f), 
	  baseHeight(40.0f), heightVariation(240.0f), waterLevel(38.0f), heightMap(nullptr),
	  heightMapBaseHeight(0.0f), heightMapScale(1.0f), worldSave(nullptr) {
	// Параметры настроены для красивого и разнообразного ландшафта:
	// - baseFreq = 1.0f/256.0f - ПРАВИЛЬНО: float деление, не int!
	// - octaves = 5 - оптимально для деталей без перегрузки
	// - baseHeight = 40.0f - уровень моря
	// - heightVariation = 240.0f - большой размах для заметных холмов, долин и гор (220-260 для выразительности)
	// - waterLevel = 38.0f - уровень воды (baseHeight - 2.0f, будет переустановлен при создании мира)
	// Террейн включает: континенты, домейн-варпинг, ридж-мультифрактал, террасы
	// Биомы определяются по температуре и влажности для более реалистичного распределения
}

ChunkManager::~ChunkManager() {
	clear();
}

void ChunkManager::clear() {
	for (auto& pair : chunks) {
		delete pair.second;
	}
	chunks.clear();
	visibleChunksCache.clear();
	meshBuildQueue.clear();
	chunksWithWater.clear();
}

std::string ChunkManager::chunkKey(int cx, int cy, int cz) const {
	return std::to_string(cx) + "," + std::to_string(cy) + "," + std::to_string(cz);
}

glm::ivec3 ChunkManager::worldToChunk(const glm::vec3& worldPos) const {
	// Для отрицательных координат нужно правильно вычислять чанк
	// floordiv() правильно работает для отрицательных чисел
	// Например: floordiv(-1, 32) = -1
	//           floordiv(-33, 32) = -2
	int cx = floordiv((int)worldPos.x, MCChunk::CHUNK_SIZE_X);
	int cy = floordiv((int)worldPos.y, MCChunk::CHUNK_SIZE_Y);
	int cz = floordiv((int)worldPos.z, MCChunk::CHUNK_SIZE_Z);
	
	return glm::ivec3(cx, cy, cz);
}

void ChunkManager::setWorldSave(class WorldSave* worldSave, const std::string& worldPath) {
	this->worldSave = worldSave;
	this->worldPath = worldPath;
}

std::string ChunkManager::getChunkFilePath(int cx, int cy, int cz) const {
	if (worldPath.empty()) {
		return "";
	}
	std::ostringstream oss;
	oss << worldPath << PATH_SEP << "regions" << PATH_SEP 
	    << cx << "_" << cy << "_" << cz << ".bin";
	return oss.str();
}

bool ChunkManager::saveChunk(MCChunk* chunk) {
	if (worldSave == nullptr || worldPath.empty() || chunk == nullptr) {
		return false;
	}
	
	// Сохраняем только изменённые чанки
	if (!chunk->dirty) {
		return true; // Уже сохранён
	}
	
	std::string chunkPath = getChunkFilePath(chunk->chunkPos.x, chunk->chunkPos.y, chunk->chunkPos.z);
	std::string tempPath = chunkPath + ".tmp";
	
	// Создаем папку regions если её нет
	std::string regionsPath = worldPath + PATH_SEP + "regions";
	if (!files::directory_exists(regionsPath)) {
		files::create_directory(regionsPath);
	}
	
	// Пишем во временный файл для атомарности
	std::ofstream file(tempPath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "[CHUNK] failed to open temp file for writing: " << tempPath << std::endl;
		return false;
	}
	
	// Магические байты
	const char magic[] = "RGON";
	file.write(magic, 4);
	
	// Версия формата региона
	int version = 1;
	file.write(reinterpret_cast<const char*>(&version), sizeof(int));
	
	// Координаты чанка
	file.write(reinterpret_cast<const char*>(&chunk->chunkPos.x), sizeof(int));
	file.write(reinterpret_cast<const char*>(&chunk->chunkPos.y), sizeof(int));
	file.write(reinterpret_cast<const char*>(&chunk->chunkPos.z), sizeof(int));
	
	// Собираем все блоки с id != 0
	std::vector<std::pair<glm::ivec3, uint8_t>> blocks;
	
	for (int y = 0; y < MCChunk::CHUNK_SIZE_Y; y++) {
		for (int z = 0; z < MCChunk::CHUNK_SIZE_Z; z++) {
			for (int x = 0; x < MCChunk::CHUNK_SIZE_X; x++) {
				voxel* vox = chunk->getVoxel(x, y, z);
				if (vox != nullptr && vox->id != 0) {
					blocks.push_back({glm::ivec3(x, y, z), vox->id});
				}
			}
		}
	}
	
	// Сохраняем количество блоков
	int numBlocks = (int)blocks.size();
	file.write(reinterpret_cast<const char*>(&numBlocks), sizeof(int));
	
	// Сохраняем все блоки (локальные координаты в чанке)
	for (const auto& block : blocks) {
		file.write(reinterpret_cast<const char*>(&block.first.x), sizeof(int));
		file.write(reinterpret_cast<const char*>(&block.first.y), sizeof(int));
		file.write(reinterpret_cast<const char*>(&block.first.z), sizeof(int));
		file.write(reinterpret_cast<const char*>(&block.second), sizeof(uint8_t));
	}
	
	file.close();
	
	// Атомарная запись: переименовываем временный файл в финальный
	#ifdef _WIN32
		// На Windows нужно удалить старый файл перед переименованием
		if (files::file_exists(chunkPath)) {
			std::remove(chunkPath.c_str());
		}
		std::rename(tempPath.c_str(), chunkPath.c_str());
	#else
		std::rename(tempPath.c_str(), chunkPath.c_str());
	#endif
	
	// Сбрасываем флаг dirty после успешного сохранения
	chunk->dirty = false;
	
	std::cout << "[CHUNK] save " << chunk->chunkPos.x << "," << chunk->chunkPos.y << "," << chunk->chunkPos.z
	          << " -> " << chunkPath << " blocks=" << numBlocks 
	          << " size=" << (numBlocks * (sizeof(int) * 3 + sizeof(uint8_t))) << " bytes" << std::endl;
	
	return true;
}

bool ChunkManager::loadChunk(int cx, int cy, int cz, MCChunk*& chunk) {
	if (worldPath.empty()) {
		return false;
	}
	
	std::string chunkPath = getChunkFilePath(cx, cy, cz);
	
	std::ifstream file(chunkPath, std::ios::binary);
	if (!file.is_open()) {
		std::cout << "[CHUNK] no file, generating " << cx << "," << cy << "," << cz << std::endl;
		return false;
	}
	
	char magic[4];
	file.read(magic, 4);
	if (magic[0] != 'R' || magic[1] != 'G' || magic[2] != 'O' || magic[3] != 'N') {
		std::cout << "[CHUNK] invalid magic bytes, regenerating " << cx << "," << cy << "," << cz << std::endl;
		file.close();
		return false;
	}
	
	int version;
	file.read(reinterpret_cast<char*>(&version), sizeof(int));
	if (!file.good()) {
		std::cout << "[CHUNK] failed to read version, regenerating " << cx << "," << cy << "," << cz << std::endl;
		file.close();
		return false;
	}
	
	if (version != 1) {
		std::cout << "[CHUNK] unsupported version " << version << ", regenerating " << cx << "," << cy << "," << cz << std::endl;
		file.close();
		return false;
	}
	
	int fileCx, fileCy, fileCz;
	file.read(reinterpret_cast<char*>(&fileCx), sizeof(int));
	file.read(reinterpret_cast<char*>(&fileCy), sizeof(int));
	file.read(reinterpret_cast<char*>(&fileCz), sizeof(int));
	if (!file.good()) {
		file.close();
		return false;
	}
	
	// Проверяем, что координаты совпадают
	if (fileCx != cx || fileCy != cy || fileCz != cz) {
		file.close();
		return false;
	}
	
	int numBlocks;
	file.read(reinterpret_cast<char*>(&numBlocks), sizeof(int));
	if (!file.good()) {
		file.close();
		return false;
	}
	
	// Создаем чанк
	chunk = new MCChunk(cx, cy, cz);
	
	// Загружаем блоки
	for (int i = 0; i < numBlocks; i++) {
		int lx, ly, lz;
		uint8_t id;
		file.read(reinterpret_cast<char*>(&lx), sizeof(int));
		file.read(reinterpret_cast<char*>(&ly), sizeof(int));
		file.read(reinterpret_cast<char*>(&lz), sizeof(int));
		file.read(reinterpret_cast<char*>(&id), sizeof(uint8_t));
		if (file.good()) {
			chunk->setVoxel(lx, ly, lz, id);
		} else {
			break;
		}
	}
	
	file.close();
	
	// После загрузки блоков нужно перегенерировать меш
	// Но если чанк был сгенерирован из высотной карты, нужно это учесть
	if (heightMap != nullptr) {
		// Генерируем поле плотности из высотной карты
		const int NX = MCChunk::CHUNK_SIZE_X;
		const int NY = MCChunk::CHUNK_SIZE_Y;
		const int NZ = MCChunk::CHUNK_SIZE_Z;
		const int SX = NX + 1;
		const int SY = NY + 1;
		const int SZ = NZ + 1;
		
		std::vector<float> densityField;
		densityField.resize(SX * SY * SZ);
		
		HeightMapUtils::convertHeightMapToDensityField(*heightMap, densityField,
		                                                cx, cy, cz,
		                                                NX, NY, NZ,
		                                                heightMapBaseHeight, heightMapScale,
		                                                true, 0);
		
		chunk->mesh = buildIsoSurface(densityField.data(), NX, NY, NZ, 0.0f);
	} else {
		// Обычная процедурная генерация меша (оптимизированная версия с callback)
		// ВАЖНО: сбрасываем флаг generated, чтобы generate() выполнился
		chunk->generated = false;
		// Используем оптимизированную версию с callback для согласованности с водой
		chunk->generate([this](float wx, float wz) {
			return this->evalSurfaceHeight(wx, wz);
		});
	}
	
	// Генерируем воду после загрузки/генерации
	// Используем функцию для получения переменного уровня воды
	// ВРЕМЕННО: можно отключить для теста, раскомментировав следующую строку
	// return; // Отключить генерацию воды для теста (раскомментируй для проверки террейна без воды)
	
	chunk->generateWater([this](int x, int z) {
		return this->getWaterLevelAt(x, z);
	});
	
	// Помечаем меш воды для пересборки после генерации
	chunk->waterMeshModified = true;
	
	chunk->generated = true;
	chunk->dirty = false; // Загруженный чанк не грязный
	chunk->voxelMeshModified = true; // Гарантируем пересборку меша блоков
	// Добавляем в очередь пересборки мешей
	meshBuildQueue.push_back(chunk);
	
	std::cout << "[CHUNK] load " << cx << "," << cy << "," << cz
	          << " <- " << chunkPath << " blocks=" << numBlocks << std::endl;
	
	return true;
}

void ChunkManager::generateChunk(int cx, int cy, int cz) {
	// Ранний выход при выходе за границы мира
	if (isOutsideBounds(cx, cz)) {
		return;
	}
	
	std::string key = chunkKey(cx, cy, cz);
	
	// Проверяем, не загружен ли уже чанк
	if (chunks.find(key) != chunks.end()) {
		return;
	}
	
	// ДИАГНОСТИКА: проверяем, не установлена ли высотная карта
	if (heightMap != nullptr) {
		std::cout << "[CHUNK] WARNING: heightMap is set, using heightmap instead of procedural generation!" << std::endl;
	}
	
	// Пытаемся загрузить чанк с диска
	MCChunk* chunk = nullptr;
	if (loadChunk(cx, cy, cz, chunk)) {
		chunks[key] = chunk;
		return;
	}
	
	// Если не удалось загрузить, генерируем новый
	chunk = new MCChunk(cx, cy, cz);
	chunk->dirty = false; // Новый чанк не грязный (будет помечен при изменении)
	
	// Если есть высотная карта, используем её, иначе процедурная генерация
	if (heightMap != nullptr) {
		// Генерируем поле плотности из высотной карты
		const int NX = MCChunk::CHUNK_SIZE_X;
		const int NY = MCChunk::CHUNK_SIZE_Y;
		const int NZ = MCChunk::CHUNK_SIZE_Z;
		const int SX = NX + 1;
		const int SY = NY + 1;
		const int SZ = NZ + 1;
		
		std::vector<float> densityField;
		densityField.resize(SX * SY * SZ);
		
		// Используем утилиту для конвертации (с билинейной интерполяцией)
		HeightMapUtils::convertHeightMapToDensityField(*heightMap, densityField,
		                                                cx, cy, cz,
		                                                NX, NY, NZ,
		                                                heightMapBaseHeight, heightMapScale,
		                                                true,  // useBilinear = true
		                                                0);    // edgeMode = clamp
		
		// Генерируем меш из поля плотности
		chunk->mesh = buildIsoSurface(densityField.data(), NX, NY, NZ, 0.0f);
		chunk->generated = true;
	} else {
		// Обычная процедурная генерация (оптимизированная версия с callback)
		// ВАЖНО: сбрасываем флаг generated, чтобы generate() выполнился
		chunk->generated = false;
		// Используем оптимизированную версию с callback для согласованности с водой
		chunk->generate([this](float wx, float wz) {
			return this->evalSurfaceHeight(wx, wz);
		});
	}
	
	// Генерируем воду после генерации террейна
	// Используем функцию для получения переменного уровня воды
	chunk->generateWater([this](int x, int z) {
		return this->getWaterLevelAt(x, z);
	});
	
	// Помечаем меш воды для пересборки после генерации
	chunk->waterMeshModified = true;
	
	// Добавляем чанк в список чанков с водой (если там есть вода)
	if (chunk->waterData && chunk->waterData->hasActiveWater()) {
		addChunkWithWater(chunk);
	}
	
	// Добавляем декорации из DecoManager
	if (decoManager != nullptr) {
		std::vector<DecoObject> decos;
		decoManager->getDecorationsOnChunk(cx, cz, decos);
		
		for (const auto& deco : decos) {
			// Преобразуем координаты декорации в локальные координаты чанка
			int lx = deco.pos.x - (cx * MCChunk::CHUNK_SIZE_X);
			int ly = deco.pos.y - (cy * MCChunk::CHUNK_SIZE_Y);
			int lz = deco.pos.z - (cz * MCChunk::CHUNK_SIZE_Z);
			
			// Проверяем, что декорация находится в пределах чанка
			if (lx >= 0 && lx < MCChunk::CHUNK_SIZE_X &&
			    ly >= 0 && ly < MCChunk::CHUNK_SIZE_Y &&
			    lz >= 0 && lz < MCChunk::CHUNK_SIZE_Z) {
				chunk->setVoxel(lx, ly, lz, deco.blockId);
			}
		}
	}
	
	// Применяем модификации из WorldBuilder (дороги, озера, префабы)
	if (worldBuilder != nullptr) {
		const auto& roadMap = worldBuilder->GetRoadMap();
		const auto& waterMap = worldBuilder->GetWaterMap();
		int worldSize = worldBuilder->GetWorldSize();
		
		// Получаем PrefabManager из WorldBuilder (нужно добавить геттер)
		// Пока что передаем nullptr, так как PrefabManager приватный
		PrefabSystem::PrefabManager* prefabManager = nullptr;
		
		chunk->applyWorldBuilderModifications(roadMap, waterMap, worldSize, prefabManager);
	}
	
	chunks[key] = chunk;
}

void ChunkManager::addChunkWithWater(MCChunk* chunk) {
	if (!chunk) return;
	
	// Проверяем, нет ли уже в списке (используем std::find для эффективности)
	auto it = std::find(chunksWithWater.begin(), chunksWithWater.end(), chunk);
	if (it == chunksWithWater.end()) {
		chunksWithWater.push_back(chunk);
	}
}

void ChunkManager::removeChunkWithWater(MCChunk* chunk) {
	if (!chunk) return;
	
	// Удаляем из списка
	chunksWithWater.erase(
		std::remove(chunksWithWater.begin(), chunksWithWater.end(), chunk),
		chunksWithWater.end()
	);
}

void ChunkManager::unloadDistantChunks(const glm::vec3& cameraPos, int renderDistance) {
	glm::ivec3 cameraChunk = worldToChunk(cameraPos);
	
	// Гистерезис-зона: выгружаем на расстоянии renderDistance + 1
	// чтобы чанки не дергались при дрожании камеры на границе
	const int unloadDistance = renderDistance + 1;
	const int yRadius = 1; // Только текущий уровень и соседние по Y
	
	// Удаляем чанки, которые слишком далеко от камеры
	// ВАЖНО: используем безопасный паттерн итерации с правильным обновлением итератора
	for (auto it = chunks.begin(); it != chunks.end(); ) {
		MCChunk* chunk = it->second;
		glm::ivec3 chunkPos = chunk->chunkPos;
		
		// Вычисляем расстояние в чанках (L∞ норма)
		int dx = chunkPos.x - cameraChunk.x;
		int dy = chunkPos.y - cameraChunk.y;
		int dz = chunkPos.z - cameraChunk.z;
		int distXZ = std::max(std::abs(dx), std::abs(dz)); // По X/Z
		int distY = std::abs(dy);
		
		// Проверяем, слишком ли далеко
		bool tooFar = (distXZ > unloadDistance) || (distY > yRadius + 1);
		
		if (tooFar) {
			// 1. Убираем чанк из всех вспомогательных контейнеров ПЕРЕД удалением
			removeChunkWithWater(chunk);
			
			// Убрать из meshBuildQueue
			meshBuildQueue.erase(
				std::remove(meshBuildQueue.begin(), meshBuildQueue.end(), chunk),
				meshBuildQueue.end()
			);
			
			// Убрать из visibleChunksCache (если там есть)
			visibleChunksCache.erase(
				std::remove(visibleChunksCache.begin(), visibleChunksCache.end(), chunk),
				visibleChunksCache.end()
			);
			
			// 2. Сохраняем чанк перед удалением (если нужно)
			if (chunk->generated && chunk->dirty) {
				saveChunk(chunk);
				chunk->dirty = false;
			}
			
			// 3. Удаляем сам чанк
			delete chunk;
			
			// 4. Стереть из map ПРАВИЛЬНО (обновляем итератор)
			it = chunks.erase(it);
		} else {
			++it;
		}
	}
}

void ChunkManager::saveDirtyChunks() {
	if (worldPath.empty()) {
		return;
	}
	
	int savedCount = 0;
	for (auto& kv : chunks) {
		MCChunk* c = kv.second;
		if (c && c->generated && c->dirty) {
			if (saveChunk(c)) {
				c->dirty = false;
				savedCount++;
			}
		}
	}
	
	if (savedCount > 0) {
		std::cout << "[CHUNK] saved " << savedCount << " dirty chunks before unload" << std::endl;
	}
}

void ChunkManager::saveDirtyChunksBudgeted(int maxPerCall) {
	if (worldPath.empty()) {
		return;
	}
	
	int saved = 0;
	for (auto& kv : chunks) {
		MCChunk* c = kv.second;
		if (c && c->generated && c->dirty) {
			if (saveChunk(c)) {
				c->dirty = false;
				if (++saved >= maxPerCall) break;
			}
		}
	}
}

void ChunkManager::setWorldBoundsByMeters(int worldSizeXZ_Meters) {
	int half = worldSizeXZ_Meters / (2 * MCChunk::CHUNK_SIZE_X);
	minChunkX = -half;
	maxChunkX = +half;
	minChunkZ = -half;
	maxChunkZ = +half;
	std::cout << "[CHUNK] World bounds set: X=[" << minChunkX << ".." << maxChunkX 
	          << "], Z=[" << minChunkZ << ".." << maxChunkZ << "] (size=" << worldSizeXZ_Meters << "m)" << std::endl;
}

// Вспомогательная функция для генерации чанков по кольцам (ring order)
static void ringOrder(int R, std::vector<glm::ivec3>& out, int cy, int yRadius) {
	// Добавляем клетки на расстоянии R по L∞ (только граница кольца)
	for (int dx = -R; dx <= R; ++dx) {
		for (int dz = -R; dz <= R; ++dz) {
			if (std::max(std::abs(dx), std::abs(dz)) != R) continue; // только граница кольца
			for (int dy = -yRadius; dy <= yRadius; ++dy) {
				out.emplace_back(dx, dy, dz);
			}
		}
	}
}

void ChunkManager::update(const glm::vec3& cameraPos, int renderDistance, float deltaTime) {
	glm::ivec3 cc = worldToChunk(cameraPos);
	
	// Только 3 уровня по высоте: текущий, над и под
	const int yRadius = 1;
	
	// Бюджет на кадр: сколько чанков генерить/читать
	const int budgetPerFrame = 6;
	
	int issued = 0;
	for (int r = 0; r <= renderDistance && issued < budgetPerFrame; ++r) {
		std::vector<glm::ivec3> ring;
		ring.reserve((8 * r + 4) * (2 * yRadius + 1));
		ringOrder(r, ring, cc.y, yRadius);
		
		for (const auto& d : ring) {
			int cx = cc.x + d.x;
			int cy = cc.y + d.y;
			int cz = cc.z + d.z;
			
			if (isOutsideBounds(cx, cz)) continue;
			
			std::string key = chunkKey(cx, cy, cz);
			if (chunks.find(key) == chunks.end()) {
				generateChunk(cx, cy, cz);
				if (++issued >= budgetPerFrame) break;
			}
		}
		if (issued >= budgetPerFrame) break;
	}
	
	// Обновляем симуляцию воды ПЕРЕД выгрузкой чанков
	// (чтобы вода успела обработать чанки до их удаления)
	if (waterSimulator != nullptr) {
		waterSimulator->Update(deltaTime);
	}
	if (waterEvaporationManager != nullptr) {
		waterEvaporationManager->Update(deltaTime);
	}
	
	// Выгружаем далекие чанки (ПОСЛЕ симуляции воды, но ДО обновления кэша)
	unloadDistantChunks(cameraPos, renderDistance);
	
	// Обновляем кэш видимых чанков ПОСЛЕ выгрузки (чтобы не было мёртвых указателей)
	visibleChunksCache.clear();
	for (auto& kv : chunks) {
		MCChunk* c = kv.second;
		if (!c->generated || c->mesh == nullptr) continue;
		
		glm::ivec3 d = c->chunkPos - cc;
		int dist = std::max({std::abs(d.x), std::abs(d.y), std::abs(d.z)});
		if (dist > renderDistance) continue; // защита от далеких чанков
		
		visibleChunksCache.push_back(c);
	}
	
	// Опционально — отсортировать по расстоянию для красивой подгрузки
	std::sort(visibleChunksCache.begin(), visibleChunksCache.end(),
	          [cc](MCChunk* a, MCChunk* b){
	              glm::ivec3 da = a->chunkPos - cc;
	              glm::ivec3 db = b->chunkPos - cc;
	              int ra = std::max({std::abs(da.x), std::abs(da.y), std::abs(da.z)});
	              int rb = std::max({std::abs(db.x), std::abs(db.y), std::abs(db.z)});
	              return ra < rb;
	          });
	
	// Периодическое сохранение (раз в 2 секунды)
	double currentTime = static_cast<double>(std::time(nullptr));
	if (currentTime - lastSaveTime > 2.0) {
		saveDirtyChunksBudgeted(2); // Сохраняем по 2 чанка за раз
		lastSaveTime = currentTime;
	}
}

void ChunkManager::updateWaterSimulation(float deltaTime) {
	if (waterSimulator != nullptr) {
		waterSimulator->Update(deltaTime);
	}
	if (waterEvaporationManager != nullptr) {
		waterEvaporationManager->Update(deltaTime);
	}
}

MCChunk* ChunkManager::getChunk(const std::string& chunkKey) const {
	auto it = chunks.find(chunkKey);
	if (it != chunks.end()) {
		return it->second;
	}
	return nullptr;
}

MCChunk* ChunkManager::getChunk(int cx, int cy, int cz) const {
	auto it = chunks.find(chunkKey(cx, cy, cz));
	return (it == chunks.end()) ? nullptr : it->second;
}

std::vector<MCChunk*> ChunkManager::getAllChunks() const {
	std::vector<MCChunk*> all;
	for (const auto& pair : chunks) {
		if (pair.second->generated) {
			all.push_back(pair.second);
		}
	}
	return all;
}

void ChunkManager::setNoiseParams(float baseFreq, int octaves, float lacunarity, float gain, float baseHeight, float heightVariation) {
	this->baseFreq = baseFreq;
	this->octaves = octaves;
	this->lacunarity = lacunarity;
	this->gain = gain;
	this->baseHeight = baseHeight;
	this->heightVariation = heightVariation;
}

void ChunkManager::getNoiseParams(float& baseFreq, int& octaves, float& lacunarity, float& gain, float& baseHeight, float& heightVariation) const {
	baseFreq = this->baseFreq;
	octaves = this->octaves;
	lacunarity = this->lacunarity;
	gain = this->gain;
	baseHeight = this->baseHeight;
	heightVariation = this->heightVariation;
}

void ChunkManager::setSeed(int64_t seed) {
	// Пересоздаем объект noise с новым seed используя placement new
	noise.~OpenSimplex3D();
	new (&noise) OpenSimplex3D(seed);
}

void ChunkManager::setWaterLevel(float waterLevel) {
	this->waterLevel = waterLevel;
}

// Унифицированная конфигурация через GeneratorParams
void ChunkManager::configure(const GeneratorParams& p) {
	baseFreq = p.baseFreq;
	octaves = p.octaves;
	lacunarity = p.lacunarity;
	gain = p.gain;
	baseHeight = p.baseHeight;
	heightVariation = p.heightVariation;
	waterLevel = p.waterLevel;
	setSeed(p.seed);
	std::cout << "[GEN] baseFreq=" << baseFreq << " oct=" << octaves << " lac=" << lacunarity
	          << " gain=" << gain << " baseH=" << baseHeight << " var=" << heightVariation
	          << " water=" << waterLevel << " seed=" << p.seed << std::endl;
}

// Утилиты для evalSurfaceHeight
namespace {
	// remap [-1..1] -> [0..1]
	inline float remap01(float v) { return 0.5f * (v + 1.0f); }
	
	// ridge из [-1..1]
	inline float ridge(float v) { return 1.0f - std::fabs(v); }
	
	// Мягкие террасы
	inline float smooth_terrace(float h, float steps, float sharp) {
		float t = std::floor(h * steps) / steps;
		float f = (h * steps - std::floor(h * steps));
		float s = glm::smoothstep(0.0f, 1.0f, f);
		return glm::mix(h, glm::mix(t, t + 1.0f / steps, s), sharp);
	}
	
	// Domain warp структура
	struct Warp {
		float dx, dz;
	};
	
	// Безопасный domain warp
	inline Warp domain_warp(const OpenSimplex3D& noise, float x, float z, float baseFreq) {
		float wf = baseFreq * 0.25f;  // warp частота ниже базовой (в 4 раза)
		float wx = noise.fbm_norm(x * wf, 0.0f, z * wf, 3, 2.1f, 0.55f);
		float wz = noise.fbm_norm((x + 37.7f) * wf, 0.0f, (z - 19.3f) * wf, 3, 2.1f, 0.55f);
		// Фикс по миру: 8..24 вокселей (не через обратную частоту!)
		float maxWarp = 14.0f;     // оптимально для выразительности (12-16)
		return { wx * maxWarp, wz * maxWarp };
	}
}

// Вычислить высоту поверхности в точке (wx, wz) используя правильную композицию слоев
float ChunkManager::evalSurfaceHeight(float wx, float wz) const {
	// Страховка от кривых рук: проверка валидности baseFreq
	if (baseFreq <= 0.0f || !std::isfinite(baseFreq)) {
		// fallback, чтобы не получить «плоский мир»
		return baseHeight;
	}
	
	// Убеждаемся, что координаты float
	float wx_f = static_cast<float>(wx);
	float wz_f = static_cast<float>(wz);
	
	float seaLevel = baseHeight;
	float heightScale = heightVariation;
	
	// 1) Безопасный domain warp
	Warp w = domain_warp(noise, wx_f, wz_f, baseFreq);
	float x = wx_f + w.dx;
	float z = wz_f + w.dz;
	
	// 2) Континенты (низкая частота) - шире окно
	float cont = noise.fbm_norm(x * baseFreq * 0.25f, 0.0f, z * baseFreq * 0.25f, 4, 2.0f, 0.5f);
	
	// БЫСТРЫЙ ЧЕК: что шум реально меняется
	static bool noiseProbeOnce = false;
	if (!noiseProbeOnce) {
		float cA = cont;
		float cB = noise.fbm_norm((x + 100.0f) * baseFreq * 0.25f, 0.0f, z * baseFreq * 0.25f, 4, 2.0f, 0.5f);
		float cC = noise.fbm_norm(x * baseFreq * 0.25f, 0.0f, (z + 100.0f) * baseFreq * 0.25f, 4, 2.0f, 0.5f);
		std::cout << "[NOISE_PROBE] cont: A=" << cA << " B=" << cB << " C=" << cC << " baseFreq=" << baseFreq << std::endl;
		noiseProbeOnce = true;
	}
	
	float cont01 = glm::smoothstep(-0.35f, 0.35f, cont); // было -0.25..0.45 → перекос
	
	// 3) Холмы (средняя частота)
	float hills = noise.fbm_norm(x * baseFreq * 0.6f, 0.0f, z * baseFreq * 0.6f, 5, 2.0f, 0.5f);
	hills = remap01(hills);
	
	// 4) Риджи (гребни)
	float rsrc = noise.fbm_norm(x * baseFreq * 0.35f, 0.0f, z * baseFreq * 0.35f, 4, 2.1f, 0.5f);
	float ridg = glm::clamp((ridge(rsrc) - 0.2f) * 1.4f, 0.0f, 1.0f);
	
	// 5) Детали (высокие частоты) - не душим полностью вне гребней
	float det = noise.fbm_norm(x * baseFreq * 1.8f, 0.0f, z * baseFreq * 1.8f, 3, 2.2f, 0.5f);
	det = det * det; // подавить мелкий шум внизу
	float detWeight = glm::mix(0.05f, 1.0f, glm::smoothstep(0.45f, 0.8f, ridg)); // оставь хвост
	
	// 6) Смешивание слоёв по маскам - усилить риджи для выразительности
	float hillsWeight = glm::mix(0.40f, 0.55f, cont01);   // немного ослабить холмы
	float ridgWeight = glm::mix(0.35f, 0.65f, cont01);   // усилить риджи (было 0.25..0.55)
	float h01 = glm::clamp(
		hillsWeight * hills +
		ridgWeight * ridg +
		detWeight * (det * 0.25f),
		0.0f, 1.0f);
	
	// 7) Мягкие террасы - мягче и выше порог
	float terraceSharp = glm::smoothstep(0.6f, 0.92f, h01) * 0.35f; // мягче и выше порог (было 0.45f)
	h01 = smooth_terrace(h01, 9.0f, terraceSharp); // steps 8-10, НЕ ниже 6
	
	// 8) Высота в метрах (или вокселях)
	float height = seaLevel + h01 * heightScale;
	
	// ДИАГНОСТИКА: выводим min/max h01 для проверки диапазона
	static float hmin = 1e9f, hmax = -1e9f;
	static int debugCount = 0;
	static int callCount = 0;
	callCount++;
	hmin = std::min(hmin, h01);
	hmax = std::max(hmax, h01);
	if (debugCount < 1 && callCount >= 1000) {  // выводим после 1000 вызовов для статистики
		std::cout << "[HEIGHT_DEBUG] h01 min=" << hmin << " max=" << hmax 
		          << " range=" << (hmax - hmin) << " baseFreq=" << baseFreq << std::endl;
		debugCount++;
	}
	
	// ДИАГНОСТИКА: сечения высоты для проверки макро-формы
	static int probeCount = 0;
	probeCount++;
	if (probeCount == 1) {
		// Выводим высоты в разных точках для проверки разброса
		// Используем const_cast для вызова не-const функции (evalSurfaceHeight не изменяет состояние)
		float h00 = evalSurfaceHeight(0.0f, 0.0f);
		float h512_0 = evalSurfaceHeight(512.0f, 0.0f);
		float h0_512 = evalSurfaceHeight(0.0f, 512.0f);
		float h512_512 = evalSurfaceHeight(512.0f, 512.0f);
		std::cout << "[H_PROBES] (0,0)=" << h00 << " (512,0)=" << h512_0 
		          << " (0,512)=" << h0_512 << " (512,512)=" << h512_512 << std::endl;
	}
	
	return height;
}

BiomeDefinition::BiomeType ChunkManager::getBiomeAt(float wx, float wz) const {
	// Если есть провайдер биомов из изображения, используем его
	if (biomeProviderFromImage != nullptr) {
		return biomeProviderFromImage->GetBiomeAt(static_cast<int>(wx), static_cast<int>(wz));
	}
	
	// Иначе используем процедурную генерацию через шум
	// Используем const_cast для передачи noise в не-const функцию
	// (noise не изменяется в GetBiomeAt, но сигнатура требует не-const ссылку)
	return BiomeDefinition::GetBiomeAt(wx, wz, const_cast<OpenSimplex3D&>(noise));
}

void ChunkManager::setBiomeProviderFromImage(BiomeProviderFromImage* provider) {
	biomeProviderFromImage = provider;
}

float ChunkManager::getWaterLevelAt(int worldX, int worldZ) const {
	// Используем ту же функцию вычисления высоты, что и в генерации террейна
	float wx = static_cast<float>(worldX);
	float wz = static_cast<float>(worldZ);
	float surfaceHeight = evalSurfaceHeight(wx, wz);
	
	// Отладочный вывод для первых нескольких точек
	static int debugWaterLevelCount = 0;
	if (debugWaterLevelCount < 5) {
		std::cout << "[WATER_LEVEL] baseHeight=" << baseHeight
		          << " heightVariation=" << heightVariation
		          << " waterLevel=" << waterLevel
		          << " surfaceHeight=" << surfaceHeight
		          << " at (" << worldX << ", " << worldZ << ")" << std::endl;
		debugWaterLevelCount++;
	}
	
	// Используем шум для определения типа водоема
	// Разные частоты шума для разных типов водоемов
	
	// Морской шум (очень крупные области) - низкие области
	float seaNoise = noise.fbm(wx * 0.001f, 0.0f, wz * 0.001f, 2, 2.0f, 0.5f);
	
	// Озерный шум (средние области) - долины
	float lakeNoise = noise.fbm(wx * 0.005f + 1000.0f, 0.0f, wz * 0.005f + 2000.0f, 2, 2.0f, 0.5f);
	
	// Речной шум (узкие полосы) - извилистые долины
	float riverNoise1 = noise.fbm(wx * 0.02f + 5000.0f, 0.0f, wz * 0.02f + 3000.0f, 1, 2.0f, 0.5f);
	float riverNoise2 = noise.fbm(wx * 0.015f + 7000.0f, 0.0f, wz * 0.015f + 4000.0f, 1, 2.0f, 0.5f);
	float riverDist = std::sqrt(riverNoise1 * riverNoise1 + riverNoise2 * riverNoise2);
	
	// Определяем тип водоема на основе шума и высоты поверхности
	// ВАЖНО: вода должна быть только в низинах, а не везде!
	
	// Море: большие низкие области (seaNoise > 0.3 и surfaceHeight < waterLevel + 2)
	// Только в действительно низких местах
	if (seaNoise > 0.3f && surfaceHeight < waterLevel + 2.0f) {
		return waterLevel + 1.5f; // Море немного выше базового уровня
	}
	
	// Озеро: средние долины (lakeNoise > 0.6 и surfaceHeight < waterLevel + 1)
	// Только в глубоких долинах
	if (lakeNoise > 0.6f && surfaceHeight < waterLevel + 1.0f) {
		return waterLevel + 0.5f; // Озеро на уровне воды
	}
	
	// Река: узкие долины (riverDist < 0.15 и surfaceHeight < waterLevel)
	// Только в очень узких и низких долинах
	if (riverDist < 0.15f && surfaceHeight < waterLevel) {
		// Высота реки зависит от глубины долины
		float riverDepth = waterLevel - surfaceHeight;
		float riverHeight = waterLevel - 0.3f + std::min(riverDepth * 0.2f, 0.8f);
		return riverHeight;
	}
	
	// Если поверхность ниже базового уровня воды, заполняем водой
	// Это основное условие для генерации воды
	if (surfaceHeight < waterLevel) {
		return waterLevel; // Базовый уровень воды
	}
	
	// Нет воды (суша)
	return -1000.0f; // Специальное значение, означающее отсутствие воды
}

// ==================== Работа с высотными картами ====================

void ChunkManager::setHeightMap(const std::string& filepath) {
	// Определяем тип файла по расширению
	size_t dotPos = filepath.find_last_of(".");
	if (dotPos == std::string::npos) {
		std::cerr << "[ChunkManager] Invalid filepath (no extension): " << filepath << std::endl;
		return;
	}
	std::string ext = filepath.substr(dotPos + 1);
	// Преобразуем в нижний регистр
	for (char& c : ext) {
		c = std::tolower(c);
	}
	
	HeightMapUtils::HeightData2D* loadedMap = nullptr;
	
	if (ext == "raw") {
		// Пробуем загрузить RAW (автоопределение размера)
		loadedMap = HeightMapUtils::loadRAWToHeightData(filepath);
		if (loadedMap == nullptr) {
			// Если не получилось, пробуем с указанными размерами (например, 1024x1024)
			loadedMap = HeightMapUtils::loadHeightMapRAW(filepath, 1024, 1024);
		}
	} else if (ext == "png" || ext == "tga") {
		loadedMap = HeightMapUtils::convertDTMToHeightData(filepath);
	}
	
	if (loadedMap != nullptr) {
		heightMap.reset(loadedMap);
		std::cout << "[ChunkManager] Height map loaded: " << filepath 
		          << " (" << loadedMap->width << "x" << loadedMap->height << ")" << std::endl;
		// Очищаем все чанки, чтобы перегенерировать с новой картой
		clear();
	} else {
		std::cerr << "[ChunkManager] Failed to load height map: " << filepath << std::endl;
	}
}

void ChunkManager::setHeightMap(HeightMapUtils::HeightData2D* heightMap) {
	if (heightMap != nullptr) {
		this->heightMap.reset(heightMap);
		clear(); // Перегенерируем чанки
	}
}

void ChunkManager::clearHeightMap() {
	heightMap.reset();
	clear(); // Перегенерируем чанки
}

void ChunkManager::setHeightMapScale(float baseHeight, float scale) {
	heightMapBaseHeight = baseHeight;
	heightMapScale = scale;
	// Перегенерируем все чанки с новым масштабом
	clear();
}

voxel* ChunkManager::getVoxel(int x, int y, int z) {
	glm::ivec3 chunkPos = worldToChunk(glm::vec3(x, y, z));
	std::string key = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z);
	
	auto it = chunks.find(key);
	if (it == chunks.end()) {
		return nullptr;
	}
	
	MCChunk* chunk = it->second;
	int lx = x - chunkPos.x * MCChunk::CHUNK_SIZE_X;
	int ly = y - chunkPos.y * MCChunk::CHUNK_SIZE_Y;
	int lz = z - chunkPos.z * MCChunk::CHUNK_SIZE_Z;
	
	// Корректировка для отрицательных координат
	if (lx < 0) {
		lx += MCChunk::CHUNK_SIZE_X;
	}
	if (ly < 0) {
		ly += MCChunk::CHUNK_SIZE_Y;
	}
	if (lz < 0) {
		lz += MCChunk::CHUNK_SIZE_Z;
	}
	
	return chunk->getVoxel(lx, ly, lz);
}

void ChunkManager::setVoxel(int x, int y, int z, uint8_t id) {
	glm::ivec3 chunkPos = worldToChunk(glm::vec3(x, y, z));
	std::string key = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z);
	
	auto it = chunks.find(key);
	if (it == chunks.end()) {
		// Чанк не найден - создаем его (для загрузки сохранений)
		generateChunk(chunkPos.x, chunkPos.y, chunkPos.z);
		it = chunks.find(key);
		if (it == chunks.end()) {
			std::cout << "[DEBUG] Failed to create chunk for world coords (" << x << ", " << y << ", " << z 
			          << ") chunk coords (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
			return;
		}
	}
	
	MCChunk* chunk = it->second;
	
	// Вычисляем локальные координаты правильно
	int lx = x - chunkPos.x * MCChunk::CHUNK_SIZE_X;
	int ly = y - chunkPos.y * MCChunk::CHUNK_SIZE_Y;
	int lz = z - chunkPos.z * MCChunk::CHUNK_SIZE_Z;
	
	// Корректировка для отрицательных координат
	// Например, для x = -1 и chunkPos.x = -1, lx должно быть 31 (CHUNK_SIZE_X - 1)
	if (lx < 0) {
		lx += MCChunk::CHUNK_SIZE_X;
	}
	if (ly < 0) {
		ly += MCChunk::CHUNK_SIZE_Y;
	}
	if (lz < 0) {
		lz += MCChunk::CHUNK_SIZE_Z;
	}
	
	// Проверяем границы
	if (lx < 0 || lx >= MCChunk::CHUNK_SIZE_X || 
	    ly < 0 || ly >= MCChunk::CHUNK_SIZE_Y || 
	    lz < 0 || lz >= MCChunk::CHUNK_SIZE_Z) {
		std::cout << "[DEBUG] Local coords out of bounds: world(" << x << ", " << y << ", " << z 
		          << ") chunk(" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z 
		          << ") local(" << lx << ", " << ly << ", " << lz << ")" << std::endl;
		return;
	}
	
	// Специальная обработка для воды (ID = 10)
	if (id == 10) {
		// Размещаем воду в системе WaterData вместо обычного блока
		if (chunk->waterData) {
			int waterIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(lx, ly, lz);
			chunk->waterData->setVoxelMass(waterIndex, WaterUtils::WATER_MASS_MAX);
			chunk->waterData->setVoxelActive(waterIndex);
			chunk->waterMeshModified = true;
			chunk->dirty = true;
			std::cout << "[WATER] Placed water at (" << x << ", " << y << ", " << z << ") "
			          << "mass=" << WaterUtils::WATER_MASS_MAX << " active=" 
			          << (chunk->waterData->isVoxelActive(waterIndex) ? "true" : "false") << std::endl;
		} else {
			std::cout << "[WATER] ERROR: waterData is nullptr for chunk at (" 
			          << chunk->chunkPos.x << ", " << chunk->chunkPos.y << ", " << chunk->chunkPos.z << ")" << std::endl;
		}
		// Не устанавливаем обычный блок для воды
		return;
	}
	
	// Устанавливаем блок
	chunk->setVoxel(lx, ly, lz, id);
	
	// Если удаляем блок (id=0), также удаляем воду, если она была
	if (id == 0 && chunk->waterData) {
		int waterIndex = WaterUtils::GetVoxelIndex<MCChunk::CHUNK_SIZE_X, MCChunk::CHUNK_SIZE_Y, MCChunk::CHUNK_SIZE_Z>(lx, ly, lz);
		if (chunk->waterData->getVoxelMass(waterIndex) > 0) {
			chunk->waterData->setVoxelMass(waterIndex, 0);
			chunk->waterData->setVoxelInactive(waterIndex);
			chunk->waterMeshModified = true;
			chunk->dirty = true;
		}
	}
	
	// Добавляем чанк в очередь пересборки мешей (если еще не добавлен)
	if (chunk->voxelMeshModified) {
		// Проверяем, нет ли уже в очереди
		bool alreadyInQueue = false;
		for (auto* queued : meshBuildQueue) {
			if (queued == chunk) {
				alreadyInQueue = true;
				break;
			}
		}
		if (!alreadyInQueue) {
			meshBuildQueue.push_back(chunk);
		}
	}
	
	// Проверяем, что блок установился
	voxel* checkVox = chunk->getVoxel(lx, ly, lz);
	if (checkVox == nullptr || checkVox->id != id) {
		std::cout << "[DEBUG] Block not set correctly: world(" << x << ", " << y << ", " << z 
		          << ") chunk(" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z 
		          << ") local(" << lx << ", " << ly << ", " << lz << ") id=" << (int)id << std::endl;
	}
	
	// Добавляем соседние чанки в очередь пересборки мешей (если блок на границе)
	auto addToQueue = [this](MCChunk* c) {
		if (c && c->voxelMeshModified) {
			// Проверяем, нет ли уже в очереди
			bool alreadyInQueue = false;
			for (auto* queued : meshBuildQueue) {
				if (queued == c) {
					alreadyInQueue = true;
					break;
				}
			}
			if (!alreadyInQueue) {
				meshBuildQueue.push_back(c);
			}
		}
	};
	
	if (lx == 0) {
		MCChunk* neighbor = getChunk(chunkPos.x - 1, chunkPos.y, chunkPos.z);
		if (neighbor) {
			neighbor->voxelMeshModified = true;
			addToQueue(neighbor);
		}
	}
	if (lx == MCChunk::CHUNK_SIZE_X - 1) {
		MCChunk* neighbor = getChunk(chunkPos.x + 1, chunkPos.y, chunkPos.z);
		if (neighbor) {
			neighbor->voxelMeshModified = true;
			addToQueue(neighbor);
		}
	}
	if (ly == 0) {
		MCChunk* neighbor = getChunk(chunkPos.x, chunkPos.y - 1, chunkPos.z);
		if (neighbor) {
			neighbor->voxelMeshModified = true;
			addToQueue(neighbor);
		}
	}
	if (ly == MCChunk::CHUNK_SIZE_Y - 1) {
		MCChunk* neighbor = getChunk(chunkPos.x, chunkPos.y + 1, chunkPos.z);
		if (neighbor) {
			neighbor->voxelMeshModified = true;
			addToQueue(neighbor);
		}
	}
	if (lz == 0) {
		MCChunk* neighbor = getChunk(chunkPos.x, chunkPos.y, chunkPos.z - 1);
		if (neighbor) {
			neighbor->voxelMeshModified = true;
			addToQueue(neighbor);
		}
	}
	if (lz == MCChunk::CHUNK_SIZE_Z - 1) {
		MCChunk* neighbor = getChunk(chunkPos.x, chunkPos.y, chunkPos.z + 1);
		if (neighbor) {
			neighbor->voxelMeshModified = true;
			addToQueue(neighbor);
		}
	}
}

voxel* ChunkManager::rayCast(const glm::vec3& a, const glm::vec3& dir, float maxDist, glm::vec3& end, glm::vec3& norm, glm::ivec3& iend) {
	float px = a.x;
	float py = a.y;
	float pz = a.z;
	
	float dx = dir.x;
	float dy = dir.y;
	float dz = dir.z;
	
	float t = 0.0f;
	int ix = (int)std::floor(px);
	int iy = (int)std::floor(py);
	int iz = (int)std::floor(pz);
	
	float stepx = (dx > 0.0f) ? 1.0f : -1.0f;
	float stepy = (dy > 0.0f) ? 1.0f : -1.0f;
	float stepz = (dz > 0.0f) ? 1.0f : -1.0f;
	
	float infinity = std::numeric_limits<float>::infinity();
	
	float txDelta = (dx == 0.0f) ? infinity : std::abs(1.0f / dx);
	float tyDelta = (dy == 0.0f) ? infinity : std::abs(1.0f / dy);
	float tzDelta = (dz == 0.0f) ? infinity : std::abs(1.0f / dz);
	
	float xdist = (stepx > 0) ? (ix + 1 - px) : (px - ix);
	float ydist = (stepy > 0) ? (iy + 1 - py) : (py - iy);
	float zdist = (stepz > 0) ? (iz + 1 - pz) : (pz - iz);
	
	float txMax = (txDelta < infinity) ? txDelta * xdist : infinity;
	float tyMax = (tyDelta < infinity) ? tyDelta * ydist : infinity;
	float tzMax = (tzDelta < infinity) ? tzDelta * zdist : infinity;
	
	int steppedIndex = -1;
	
	while (t <= maxDist){
		voxel* vox = getVoxel(ix, iy, iz);
		if (vox != nullptr && vox->id != 0){
			end.x = px + t * dx;
			end.y = py + t * dy;
			end.z = pz + t * dz;
			
			iend.x = ix;
			iend.y = iy;
			iend.z = iz;
			
			norm.x = norm.y = norm.z = 0.0f;
			if (steppedIndex == 0) norm.x = -stepx;
			if (steppedIndex == 1) norm.y = -stepy;
			if (steppedIndex == 2) norm.z = -stepz;
			return vox;
		}
		if (txMax < tyMax) {
			if (txMax < tzMax) {
				ix += (int)stepx;
				t = txMax;
				txMax += txDelta;
				steppedIndex = 0;
			} else {
				iz += (int)stepz;
				t = tzMax;
				tzMax += tzDelta;
				steppedIndex = 2;
			}
		} else {
			if (tyMax < tzMax) {
				iy += (int)stepy;
				t = tyMax;
				tyMax += tyDelta;
				steppedIndex = 1;
			} else {
				iz += (int)stepz;
				t = tzMax;
				tzMax += tzDelta;
				steppedIndex = 2;
			}
		}
	}
	iend.x = ix;
	iend.y = iy;
	iend.z = iz;
	
	end.x = px + t * dx;
	end.y = py + t * dy;
	end.z = pz + t * dz;
	norm.x = norm.y = norm.z = 0.0f;
	return nullptr;
}

bool ChunkManager::rayCastDetailed(const glm::vec3& start, const glm::vec3& dir, float maxDist, HitInfo::HitInfoDetails& hitInfo) {
	hitInfo.clear();
	
	// Используем улучшенный DDA алгоритм из VoxelUtils
	glm::vec3 normalizedDir = glm::normalize(dir);
	glm::ivec3 voxelPos(
		static_cast<int>(std::floor(start.x)),
		static_cast<int>(std::floor(start.y)),
		static_cast<int>(std::floor(start.z))
	);
	
	float distanceSq = maxDist * maxDist;
	glm::vec3 currentPos = start;
	
	// Итерация по вокселям на пути луча
	for (int step = 0; step < 1000 && glm::dot(currentPos - start, currentPos - start) < distanceSq; ++step) {
		glm::vec3 hitPos;
		HitInfo::BlockFace blockFace;
		
		glm::ivec3 nextVoxelPos = VoxelUtils::OneVoxelStep(voxelPos, currentPos, normalizedDir, hitPos, blockFace);
		
		// Проверяем блок в новой позиции
		voxel* vox = getVoxel(nextVoxelPos.x, nextVoxelPos.y, nextVoxelPos.z);
		
		if (vox != nullptr && vox->id != 0) {
			// Вычисляем квадрат расстояния
			glm::vec3 diff = hitPos - start;
			float distSq = glm::dot(diff, diff);
			
			// Получаем нормаль для грани
			glm::vec3 normal = VoxelUtils::normals[static_cast<int>(blockFace)];
			
			hitInfo.setFromRaycast(hitPos, normal, nextVoxelPos, vox, distSq);
			return true;
		}
		
		voxelPos = nextVoxelPos;
		currentPos = hitPos + normalizedDir * 0.01f; // Небольшое смещение для следующей итерации
	}
	
	return false;
}

bool ChunkManager::rayCastSurface(const glm::vec3& start, const glm::vec3& dir, float maxDist, glm::vec3& hitPos, glm::vec3& hitNorm) {
	// Простой raycast по поверхности Marching Cubes
	// Ищем пересечение луча с изосерфейсом (где плотность = 0)
	
	float stepSize = 0.1f; // Шаг для проверки плотности
	float t = 0.0f;
	float lastDensity = 0.0f;
	
	while (t < maxDist) {
		glm::vec3 pos = start + dir * t;
		
		// Получаем плотность в этой точке
		float density = 0.0f;
		glm::ivec3 chunkPos = worldToChunk(pos);
		std::string key = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z);
		
		auto it = chunks.find(key);
		if (it != chunks.end()) {
			MCChunk* chunk = it->second;
			density = chunk->getDensity(pos);
		}
		
		// Если плотность изменила знак, значит пересекли изосерфейс
		if (lastDensity != 0.0f && (lastDensity > 0.0f) != (density > 0.0f)) {
			// Нашли пересечение - используем бисекцию для точности
			float t0 = t - stepSize;
			float t1 = t;
			float d0 = lastDensity;
			float d1 = density;
			
			// Биссекция для точного нахождения точки пересечения
			for (int i = 0; i < 5; i++) {
				float tm = (t0 + t1) * 0.5f;
				glm::vec3 pm = start + dir * tm;
				
				float dm = 0.0f;
				glm::ivec3 chunkPosM = worldToChunk(pm);
				std::string keyM = chunkKey(chunkPosM.x, chunkPosM.y, chunkPosM.z);
				auto itM = chunks.find(keyM);
				if (itM != chunks.end()) {
					dm = itM->second->getDensity(pm);
				}
				
				if ((d0 > 0.0f) == (dm > 0.0f)) {
					t0 = tm;
					d0 = dm;
				} else {
					t1 = tm;
					d1 = dm;
				}
			}
			
			hitPos = start + dir * ((t0 + t1) * 0.5f);
			
			// Вычисляем нормаль через градиент плотности
			float eps = 0.1f;
			float dx = 0.0f, dy = 0.0f, dz = 0.0f;
			
			glm::ivec3 chunkPosG = worldToChunk(hitPos);
			std::string keyG = chunkKey(chunkPosG.x, chunkPosG.y, chunkPosG.z);
			auto itG = chunks.find(keyG);
			if (itG != chunks.end()) {
				MCChunk* chunkG = itG->second;
				dx = chunkG->getDensity(hitPos + glm::vec3(eps, 0, 0)) - 
				     chunkG->getDensity(hitPos - glm::vec3(eps, 0, 0));
				dy = chunkG->getDensity(hitPos + glm::vec3(0, eps, 0)) - 
				     chunkG->getDensity(hitPos - glm::vec3(0, eps, 0));
				dz = chunkG->getDensity(hitPos + glm::vec3(0, 0, eps)) - 
				     chunkG->getDensity(hitPos - glm::vec3(0, 0, eps));
			}
			
			hitNorm = glm::normalize(glm::vec3(dx, dy, dz));
			if (hitNorm.y < 0.0f) {
				hitNorm = -hitNorm; // Нормаль должна быть направлена вверх
			}
			
			return true;
		}
		
		lastDensity = density;
		t += stepSize;
	}
	
	return false;
}

