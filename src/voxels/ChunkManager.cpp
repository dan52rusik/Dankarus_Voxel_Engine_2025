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
#ifdef _WIN32
const char PATH_SEP = '\\';
#else
const char PATH_SEP = '/';
#endif

ChunkManager::ChunkManager() 
	: noise(1337), baseFreq(0.06f), octaves(6), lacunarity(2.0f), gain(0.5f), 
	  baseHeight(50.0f), heightVariation(70.0f), waterLevel(8.0f), heightMap(nullptr),
	  heightMapBaseHeight(0.0f), heightMapScale(1.0f), worldSave(nullptr) {
	// Параметры настроены для красивого и разнообразного ландшафта:
	// - baseFreq = 0.06f - оптимальная частота для заметного рельефа внутри чанка
	// - octaves = 6 - больше деталей для более интересного рельефа
	// - baseHeight = 50.0f - средняя высота материка
	// - heightVariation = 70.0f - большой размах для заметных холмов, долин и гор
	// - waterLevel = 8.0f - уровень воды в низинах (будет переустановлен при создании мира)
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
	
	std::cout << "[CHUNK] load " << cx << "," << cy << "," << cz
	          << " <- " << chunkPath << " blocks=" << numBlocks << std::endl;
	
	return true;
}

void ChunkManager::generateChunk(int cx, int cy, int cz) {
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

void ChunkManager::unloadDistantChunks(const glm::vec3& cameraPos, int renderDistance) {
	glm::ivec3 cameraChunk = worldToChunk(cameraPos);
	
	// Гистерезис-зона: выгружаем на расстоянии renderDistance + 1
	// чтобы чанки не дергались при дрожании камеры на границе
	const int unloadDistance = renderDistance + 1;
	
	// Удаляем чанки, которые слишком далеко от камеры
	auto it = chunks.begin();
	while (it != chunks.end()) {
		MCChunk* chunk = it->second;
		glm::ivec3 chunkPos = chunk->chunkPos;
		
		// Вычисляем расстояние в чанках
		int dx = chunkPos.x - cameraChunk.x;
		int dy = chunkPos.y - cameraChunk.y;
		int dz = chunkPos.z - cameraChunk.z;
		int dist = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
		
		if (dist > unloadDistance) {
			// Сохраняем чанк перед удалением только если он был изменен
			if (chunk->generated && chunk->dirty) {
				saveChunk(chunk);
				chunk->dirty = false;
			}
			delete chunk;
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

void ChunkManager::update(const glm::vec3& cameraPos, int renderDistance) {
	glm::ivec3 cameraChunk = worldToChunk(cameraPos);
	
	// Генерируем чанки вокруг камеры
	for (int x = -renderDistance; x <= renderDistance; x++) {
		for (int y = -renderDistance; y <= renderDistance; y++) {
			for (int z = -renderDistance; z <= renderDistance; z++) {
				// Генерируем только чанки в пределах радиуса
				if (std::max({std::abs(x), std::abs(y), std::abs(z)}) <= renderDistance) {
					int cx = cameraChunk.x + x;
					int cy = cameraChunk.y + y;
					int cz = cameraChunk.z + z;
					generateChunk(cx, cy, cz);
				}
			}
		}
	}
	
	// Выгружаем далекие чанки
	unloadDistantChunks(cameraPos, renderDistance);
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

std::vector<MCChunk*> ChunkManager::getVisibleChunks() const {
	std::vector<MCChunk*> visible;
	for (const auto& pair : chunks) {
		if (pair.second->generated && pair.second->mesh != nullptr) {
			visible.push_back(pair.second);
		}
	}
	return visible;
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

// Вычислить высоту поверхности в точке (wx, wz) используя ту же формулу, что и в генерации террейна
float ChunkManager::evalSurfaceHeight(float wx, float wz) const {
	// 1) Domain warping — «кривим» входные координаты, чтобы исчезла регулярность FBM
	float w1 = noise.fbm(wx * 0.008f, 0.0f, wz * 0.008f, 3, 2.0f, 0.5f);
	float w2 = noise.fbm(wx * 0.008f + 100.0f, 0.0f, wz * 0.008f + 100.0f, 3, 2.0f, 0.5f);
	float warpAmp = 40.0f; // Увеличено для более интересных изгибов рельефа
	float wxw = wx + w1 * warpAmp;
	float wzw = wz + w2 * warpAmp;
	
	// 2) Континентальность (очень низкая частота) — задаёт большие области суши/впадин
	float continents = 0.5f * noise.fbm(wx * 0.0015f, 0.0f, wz * 0.0015f, 2, 2.0f, 0.5f) + 0.5f;
	// «поднимаем» материки и немного притапливаем впадины
	float continentBias = glm::mix(-0.7f, 0.7f, continents);  // Увеличено для более выраженных континентов
	
	// 3) Базовый FBM (мягкие холмы) — уже по варпнутым координатам
	float hills = noise.fbm(wxw * baseFreq, 0.0f, wzw * baseFreq, octaves, lacunarity, gain); // -1..1
	
	// 4) Средне-высокочастотная полоса (для дополнительного разнообразия)
	float midNoise = noise.fbm(wxw * baseFreq * 1.5f, 0.0f, wzw * baseFreq * 1.5f, 3, 2.0f, 0.5f) * 0.5f; // Усилено
	
	// 5) Ридж-мультифрактал (острые хребты/скалы) - более заметные горы
	auto ridge = [](float n) { n = 1.0f - std::fabs(n); return n * n; };
	float ridgesFBM = noise.fbm(wxw * baseFreq * 0.5f, 0.0f, wzw * baseFreq * 0.5f, 4, 2.0f, 0.5f); // -1..1
	float ridges = ridge(ridgesFBM);  // 0..1
	// Делаем горы более заметными
	ridges = std::max(0.0f, ridges - 0.2f) * 1.5f; // Усилено для более выраженных гор
	
	// 6) Детальки (мелкие формы) - усиленные для заметности внутри чанка
	float details = noise.fbm(wxw * baseFreq * 3.5f, 0.0f, wzw * baseFreq * 3.5f, 3, 2.0f, 0.5f) * 0.7f; // Усилено
	
	// 7) Склеиваем: материки -> холмы -> средние -> хребты -> детали.
	//    Вес хребтов растёт на «краях континентов» (там интереснее рельеф)
	float ridgeWeight = glm::smoothstep(0.25f, 0.85f, continents); // 0..1
	float combined =
		hills * 0.6f
		+ midNoise                                    // Средне-высокочастотная полоса
		+ (ridges * 2.0f - 1.0f) * (1.0f * ridgeWeight)   // Горы с большим весом
		+ details                                     // Усиленные детали
		+ continentBias;                              // большой макро-тренд
	
	// 8) Курация амплитуды и опциональные «террасы»
	combined = glm::clamp(combined, -1.3f, 1.3f); // Расширен диапазон
	float remapped = 0.5f * combined + 0.5f; // remap01
	float t = std::floor(remapped * 8.0f) / 8.0f; // Больше террас
	float shaped = glm::mix(remapped, t, 0.2f); // Меньше террас для более плавного рельефа
	shaped = shaped * 2.0f - 1.0f;                             // обратно в [-1..1]
	
	// 9) Высота поверхности
	return baseHeight + shaped * heightVariation;
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
	
	// Проверяем, что блок установился
	voxel* checkVox = chunk->getVoxel(lx, ly, lz);
	if (checkVox == nullptr || checkVox->id != id) {
		std::cout << "[DEBUG] Block not set correctly: world(" << x << ", " << y << ", " << z 
		          << ") chunk(" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z 
		          << ") local(" << lx << ", " << ly << ", " << lz << ") id=" << (int)id << std::endl;
	}
	
	// Помечаем соседние чанки как измененные, если блок на границе
	if (lx == 0) {
		std::string neighborKey = chunkKey(chunkPos.x - 1, chunkPos.y, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (lx == MCChunk::CHUNK_SIZE_X - 1) {
		std::string neighborKey = chunkKey(chunkPos.x + 1, chunkPos.y, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (ly == 0) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y - 1, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (ly == MCChunk::CHUNK_SIZE_Y - 1) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y + 1, chunkPos.z);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (lz == 0) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z - 1);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
		}
	}
	if (lz == MCChunk::CHUNK_SIZE_Z - 1) {
		std::string neighborKey = chunkKey(chunkPos.x, chunkPos.y, chunkPos.z + 1);
		auto neighborIt = chunks.find(neighborKey);
		if (neighborIt != chunks.end()) {
			neighborIt->second->voxelMeshModified = true;
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

