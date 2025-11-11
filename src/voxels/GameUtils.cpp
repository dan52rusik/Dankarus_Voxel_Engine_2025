#include "GameUtils.h"
#include "ChunkManager.h"
#include "HitInfo.h"
#include "../noise/OpenSimplex.h"
#include "../noise/PerlinNoise.h"
#include <algorithm>
#include <cmath>
#include <climits>
#include <sstream>
#include <iomanip>

namespace GameUtils {
	// Соседи в 8 направлениях
	const std::vector<glm::ivec2> NeighborsEightWay = {
		glm::ivec2(0, 1),   // N
		glm::ivec2(1, 1),   // NE
		glm::ivec2(1, 0),   // E
		glm::ivec2(1, -1),  // SE
		glm::ivec2(0, -1),  // S
		glm::ivec2(-1, -1), // SW
		glm::ivec2(-1, 0),  // W
		glm::ivec2(-1, 1)   // NW
	};
	
	// ==================== Работа с рудами ====================
	
	// Основная версия с PerlinNoise (как в оригинале 7DTD)
	float GetOreNoiseAt(PerlinNoise& noise, int x, int y, int z) {
		// Используем частоту 0.05 для генерации рудных жил
		// PerlinNoise::Noise возвращает значение в диапазоне [-1, 1]
		double n = noise.Noise(
			static_cast<double>(x) * 0.05,
			static_cast<double>(y) * 0.05,
			static_cast<double>(z) * 0.05
		);
		// Преобразуем в диапазон [-1, 1] с смещением
		// В оригинале 7DTD используется формула: (n - 0.333) * 3.0
		return static_cast<float>((n - 0.333) * 3.0);
	}
	
	// Перегрузка для OpenSimplex3D (для совместимости)
	float GetOreNoiseAt(OpenSimplex3D& noise, int x, int y, int z) {
		// Используем частоту 0.05 для генерации рудных жил
		double n = noise.noise(
			static_cast<double>(x) * 0.05,
			static_cast<double>(y) * 0.05,
			static_cast<double>(z) * 0.05
		);
		// Преобразуем в диапазон [-1, 1] с смещением
		return static_cast<float>((n - 0.333) * 3.0);
	}
	
	// Проверить, есть ли руда в точке (x, y, z) на основе шума (PerlinNoise)
	bool CheckOreNoiseAt(PerlinNoise& noise, int x, int y, int z) {
		return GetOreNoiseAt(noise, x, y, z) > 0.0f;
	}
	
	// Проверить, есть ли руда в точке (x, y, z) на основе шума (OpenSimplex3D)
	bool CheckOreNoiseAt(OpenSimplex3D& noise, int x, int y, int z) {
		return GetOreNoiseAt(noise, x, y, z) > 0.0f;
	}
	
	// ==================== Работа с водой ====================
	
	void WaterFloodFill(
		std::vector<uint8_t>& cols,
		std::vector<uint8_t>& waterChunks16x16Height,
		int width,
		ChunkManager* heightMap,
		int posX,
		int maxY,
		int posZ,
		uint8_t colWater,
		uint8_t colBorder,
		std::vector<glm::ivec2>& listPos,
		int minX,
		int maxX,
		int minZ,
		int maxZ,
		int worldScale
	) {
		if (heightMap == nullptr) {
			return;
		}
		
		// Получаем высоту карты (предполагаем квадратную)
		int height = width; // Для квадратной карты
		int scaledHeight = height * worldScale;
		
		// Функция для получения индекса в массиве cols
		auto getColIndex = [width, scaledHeight](int x, int z) -> int {
			int offsetX = x + width / 2;
			int offsetZ = z + scaledHeight / 2;
			if (offsetX < 0 || offsetX >= width || offsetZ < 0 || offsetZ >= scaledHeight) {
				return -1;
			}
			return offsetZ * width + offsetX;
		};
		
		// Функция для получения значения из cols
		auto getColValue = [&cols, &getColIndex](int x, int z) -> uint8_t {
			int idx = getColIndex(x, z);
			if (idx < 0 || idx >= static_cast<int>(cols.size())) {
				return 255; // Вне границ
			}
			return cols[idx];
		};
		
		// Функция для установки значения в cols
		auto setColValue = [&cols, &getColIndex](int x, int z, uint8_t value) {
			int idx = getColIndex(x, z);
			if (idx >= 0 && idx < static_cast<int>(cols.size())) {
				cols[idx] = value;
			}
		};
		
		// Основной цикл flood fill
		do {
			int x = posX + width / 2;
			int z = posZ + scaledHeight / 2;
			
			// Получаем высоту террейна в этой точке
			float terrainHeight = heightMap->evalSurfaceHeight(static_cast<float>(posX), static_cast<float>(posZ));
			int terrainY = static_cast<int>(terrainHeight);
			
			// Проверяем, можно ли заполнить водой
			if (terrainY < maxY + 1) {
				setColValue(x, z, colWater);
				
				// Обновляем высоту воды для чанка 16x16
				int chunkX = x / 16;
				int chunkZ = z / 16;
				int chunkWidth = width / 16;
				if (chunkX >= 0 && chunkX < chunkWidth && chunkZ >= 0 && chunkZ < chunkWidth) {
					int chunkIdx = chunkX + chunkZ * chunkWidth;
					if (chunkIdx >= 0 && chunkIdx < static_cast<int>(waterChunks16x16Height.size())) {
						waterChunks16x16Height[chunkIdx] = static_cast<uint8_t>(maxY);
					}
				}
				
				// Добавляем соседей в стек
				if (x < width - 1 && posX < maxX && getColValue(x + 1, z) == 0 && listPos.size() < 100000) {
					listPos.push_back(glm::ivec2(posX + 1, posZ));
				}
				if (x > 0 && posX > minX && getColValue(x - 1, z) == 0 && listPos.size() < 100000) {
					listPos.push_back(glm::ivec2(posX - 1, posZ));
				}
				if (z > 0 && posZ > minZ && getColValue(x, z - 1) == 0 && listPos.size() < 100000) {
					listPos.push_back(glm::ivec2(posX, posZ - 1));
				}
				if (z < scaledHeight - 1 && posZ < maxZ && getColValue(x, z + 1) == 0 && listPos.size() < 100000) {
					listPos.push_back(glm::ivec2(posX, posZ + 1));
				}
			} else {
				// Высота слишком большая - это граница
				setColValue(x, z, colBorder);
			}
			
			// Берем следующую позицию из стека
			if (listPos.size() > 0) {
				glm::ivec2 nextPos = listPos.back();
				listPos.pop_back();
				posX = nextPos.x;
				posZ = nextPos.y;
			} else {
				break;
			}
		} while (listPos.size() > 0);
	}
	
	// ==================== Работа с направлениями ====================
	
	DirEightWay GetDirByNormal(const glm::vec2& normal) {
		glm::vec2 normalized = glm::normalize(normal);
		glm::ivec2 rounded(
			static_cast<int>(std::round(normalized.x)),
			static_cast<int>(std::round(normalized.y))
		);
		return GetDirByNormal(rounded);
	}
	
	DirEightWay GetDirByNormal(const glm::ivec2& normal) {
		for (size_t i = 0; i < NeighborsEightWay.size(); i++) {
			if (NeighborsEightWay[i] == normal) {
				return static_cast<DirEightWay>(i);
			}
		}
		return DirEightWay::None;
	}
	
	DirEightWay GetClosestDirection(float rotation, bool limitTo90Degrees) {
		// Нормализуем угол в диапазон [0, 360)
		rotation = std::fmod(rotation, 360.0f);
		if (rotation < 0.0f) {
			rotation += 360.0f;
		}
		
		if (limitTo90Degrees) {
			// Только 4 направления: N, E, S, W
			if (rotation > 315.0f || rotation <= 45.0f) {
				return DirEightWay::N;
			}
			if (rotation <= 135.0f) {
				return DirEightWay::E;
			}
			if (rotation <= 225.0f) {
				return DirEightWay::S;
			}
			return DirEightWay::W;
		} else {
			// 8 направлений
			if (rotation > 337.5f || rotation <= 22.5f) {
				return DirEightWay::N;
			}
			if (rotation <= 67.5f) {
				return DirEightWay::NE;
			}
			if (rotation <= 112.5f) {
				return DirEightWay::E;
			}
			if (rotation <= 157.5f) {
				return DirEightWay::SE;
			}
			if (rotation <= 202.5f) {
				return DirEightWay::S;
			}
			if (rotation <= 247.5f) {
				return DirEightWay::SW;
			}
			if (rotation <= 292.5f) {
				return DirEightWay::W;
			}
			return DirEightWay::NW;
		}
	}
	
	// ==================== Работа с блоками ====================
	
	glm::vec3 GetNormalFromHitInfo(
		const glm::ivec3& blockPos,
		const glm::vec3& hitPoint,
		const glm::vec3& hitNormal
	) {
		// Просто возвращаем нормаль (можно улучшить, используя информацию о блоке)
		return glm::normalize(hitNormal);
	}
	
	HitInfo::BlockFace GetBlockFaceFromHitInfo(
		const glm::ivec3& blockPos,
		const glm::vec3& hitPoint,
		const glm::vec3& hitNormal
	) {
		// Определяем грань блока на основе нормали
		glm::vec3 normalized = glm::normalize(hitNormal);
		
		// Находим компонент с максимальным абсолютным значением
		float absX = std::abs(normalized.x);
		float absY = std::abs(normalized.y);
		float absZ = std::abs(normalized.z);
		
		if (absY > absX && absY > absZ) {
			// Вертикальная грань (Top или Bottom)
			return normalized.y > 0.0f ? HitInfo::BlockFace::Top : HitInfo::BlockFace::Bottom;
		} else if (absX > absZ) {
			// Грань по оси X (East или West)
			return normalized.x > 0.0f ? HitInfo::BlockFace::East : HitInfo::BlockFace::West;
		} else {
			// Грань по оси Z (North или South)
			return normalized.z > 0.0f ? HitInfo::BlockFace::North : HitInfo::BlockFace::South;
		}
	}
	
	bool IsColliderWithinBlock(
		const glm::ivec3& blockPosition,
		uint8_t blockId,
		const glm::vec3& colliderCenter,
		const glm::vec3& colliderSize
	) {
		// Преобразуем позицию блока в мировые координаты
		glm::vec3 blockWorldPos = glm::vec3(blockPosition);
		
		// Границы блока (1x1x1)
		glm::vec3 blockMin = blockWorldPos;
		glm::vec3 blockMax = blockWorldPos + glm::vec3(1.0f);
		
		// Границы коллайдера
		glm::vec3 colliderMin = colliderCenter - colliderSize * 0.5f;
		glm::vec3 colliderMax = colliderCenter + colliderSize * 0.5f;
		
		// Проверяем пересечение
		return (colliderMin.x < blockMax.x && colliderMax.x > blockMin.x &&
		        colliderMin.y < blockMax.y && colliderMax.y > blockMin.y &&
		        colliderMin.z < blockMax.z && colliderMax.z > blockMin.z);
	}
	
	// ==================== Работа с цветами ====================
	
	glm::u8vec4 UIntToColor(uint32_t color, bool includeAlpha) {
		if (includeAlpha) {
			return glm::u8vec4(
				static_cast<uint8_t>((color >> 16) & 0xFF),
				static_cast<uint8_t>((color >> 8) & 0xFF),
				static_cast<uint8_t>(color & 0xFF),
				static_cast<uint8_t>((color >> 24) & 0xFF)
			);
		} else {
			return glm::u8vec4(
				static_cast<uint8_t>((color >> 16) & 0xFF),
				static_cast<uint8_t>((color >> 8) & 0xFF),
				static_cast<uint8_t>(color & 0xFF),
				255
			);
		}
	}
	
	uint32_t ColorToUInt(const glm::u8vec4& color, bool includeAlpha) {
		if (includeAlpha) {
			return (static_cast<uint32_t>(color.r) << 24) |
			       (static_cast<uint32_t>(color.r) << 16) |
			       (static_cast<uint32_t>(color.g) << 8) |
			       static_cast<uint32_t>(color.b);
		} else {
			return (static_cast<uint32_t>(color.r) << 16) |
			       (static_cast<uint32_t>(color.g) << 8) |
			       static_cast<uint32_t>(color.b);
		}
	}
	
	// ==================== Работа со временем ====================
	
	int WorldTimeToDays(uint64_t worldTime) {
		return static_cast<int>((worldTime / 24000ULL) + 1ULL);
	}
	
	int WorldTimeToHours(uint64_t worldTime) {
		return static_cast<int>((worldTime / 1000ULL) % 24ULL);
	}
	
	int WorldTimeToMinutes(uint64_t worldTime) {
		return static_cast<int>((static_cast<double>(worldTime) / 1000.0 * 60.0)) % 60;
	}
	
	float WorldTimeToTotalSeconds(float worldTime) {
		return worldTime * 3.6f;
	}
	
	uint32_t WorldTimeToTotalMinutes(uint64_t worldTime) {
		return static_cast<uint32_t>(static_cast<double>(worldTime) * 0.06);
	}
	
	int WorldTimeToTotalHours(uint64_t worldTime) {
		return static_cast<int>(worldTime / 1000ULL);
	}
	
	uint64_t TotalMinutesToWorldTime(uint32_t totalMinutes) {
		return static_cast<uint64_t>(static_cast<double>(totalMinutes) / 0.06);
	}
	
	std::string WorldTimeToString(uint64_t worldTime) {
		int days = WorldTimeToDays(worldTime);
		int hours = WorldTimeToHours(worldTime);
		int minutes = WorldTimeToMinutes(worldTime);
		
		std::ostringstream oss;
		oss << days << " " << std::setfill('0') << std::setw(2) << hours << ":" 
		    << std::setfill('0') << std::setw(2) << minutes;
		return oss.str();
	}
	
	uint64_t DaysToWorldTime(int day) {
		return day < 1 ? 0ULL : (static_cast<uint64_t>(day) - 1ULL) * 24000ULL;
	}
	
	uint64_t DayTimeToWorldTime(int day, int hours, int minutes) {
		if (day < 1) {
			return 0ULL;
		}
		return (static_cast<uint64_t>(day) - 1ULL) * 24000ULL +
		       static_cast<uint64_t>(hours) * 1000ULL +
		       static_cast<uint64_t>(minutes) * 1000ULL / 60ULL;
	}
	
	// ==================== Работа с валидацией ====================
	
	char ValidateGameNameInput(const std::string& text, int charIndex, char addedChar) {
		// Разрешаем: буквы (a-z, A-Z), цифры (0-9), подчеркивание, дефис
		// Точка и пробел разрешены только не в начале
		if (addedChar >= 0x80) { // Не-ASCII символы
			return addedChar;
		}
		if ((addedChar >= 'a' && addedChar <= 'z') ||
		    (addedChar >= 'A' && addedChar <= 'Z') ||
		    (addedChar >= '0' && addedChar <= '9') ||
		    addedChar == '_' || addedChar == '-') {
			return addedChar;
		}
		if (charIndex > 0 && (addedChar == '.' || addedChar == ' ')) {
			return addedChar;
		}
		return '\0'; // Невалидный символ
	}
	
	bool ValidateGameName(const std::string& gameName) {
		std::string trimmed = gameName;
		// Убираем пробелы в начале и конце
		trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
		trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
		
		if (trimmed.empty() || trimmed.length() != gameName.length()) {
			return false;
		}
		
		// Проверяем каждый символ
		for (size_t i = 0; i < gameName.length(); i++) {
			if (ValidateGameNameInput(gameName, static_cast<int>(i), gameName[i]) == '\0') {
				return false;
			}
		}
		
		// Не должно заканчиваться точкой
		if (!gameName.empty() && gameName.back() == '.') {
			return false;
		}
		
		return true;
	}
	
	char ValidateHexInput(const std::string& text, int charIndex, char addedChar) {
		if ((addedChar >= 'a' && addedChar <= 'f') ||
		    (addedChar >= 'A' && addedChar <= 'F') ||
		    (addedChar >= '0' && addedChar <= '9')) {
			return addedChar;
		}
		return '\0';
	}
	
	// ==================== Работа с векторами ====================
	
	uint64_t Vector3iToUInt64(const glm::ivec3& v) {
		// Преобразуем координаты в диапазон [0, 65535] (uint16_t)
		uint64_t x = static_cast<uint64_t>(v.x + 32768) & 0xFFFF;
		uint64_t y = static_cast<uint64_t>(v.y + 32768) & 0xFFFF;
		uint64_t z = static_cast<uint64_t>(v.z + 32768) & 0xFFFF;
		
		// Упаковываем: x в старшие 32 бита, y в средние 16, z в младшие 16
		return (x << 32) | (y << 16) | z;
	}
	
	glm::ivec3 UInt64ToVector3i(uint64_t fullValue) {
		int x = static_cast<int>((fullValue >> 32) & 0xFFFF) - 32768;
		int y = static_cast<int>((fullValue >> 16) & 0xFFFF) - 32768;
		int z = static_cast<int>(fullValue & 0xFFFF) - 32768;
		return glm::ivec3(x, y, z);
	}
	
	// ==================== Утилиты для отладки ====================
	
	std::string SafeStringFormat(const std::string& s) {
		std::string result = s;
		// Экранируем фигурные скобки
		size_t pos = 0;
		while ((pos = result.find('{', pos)) != std::string::npos) {
			result.insert(pos, "{");
			pos += 2;
		}
		pos = 0;
		while ((pos = result.find('}', pos)) != std::string::npos) {
			result.insert(pos, "}");
			pos += 2;
		}
		return result;
	}
}

