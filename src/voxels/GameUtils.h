#ifndef VOXELS_GAMEUTILS_H_
#define VOXELS_GAMEUTILS_H_

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <string>
#include "HitInfo.h"

// Forward declarations
class OpenSimplex3D;
class PerlinNoise;
class ChunkManager;

// Направления (8-сторонние)
enum class DirEightWay : int8_t {
	None = -1,
	N = 0,    // Север
	NE = 1,   // Северо-восток
	E = 2,    // Восток
	SE = 3,   // Юго-восток
	S = 4,    // Юг
	SW = 5,   // Юго-запад
	W = 6,    // Запад
	NW = 7,   // Северо-запад
	COUNT = 8
};

// Утилиты для игры
// Адаптировано из 7 Days To Die GameUtils
namespace GameUtils {
	// Соседи в 8 направлениях (для pathfinding, water flow и т.д.)
	extern const std::vector<glm::ivec2> NeighborsEightWay;
	
	// ==================== Работа с рудами ====================
	
	// Получить значение шума для руды в точке (x, y, z)
	// Используется для генерации рудных жил
	// В оригинале 7DTD используется PerlinNoise для генерации руд
	float GetOreNoiseAt(PerlinNoise& noise, int x, int y, int z);
	
	// Перегрузка для OpenSimplex3D (для совместимости)
	float GetOreNoiseAt(OpenSimplex3D& noise, int x, int y, int z);
	
	// Проверить, есть ли руда в точке (x, y, z) на основе шума (PerlinNoise)
	bool CheckOreNoiseAt(PerlinNoise& noise, int x, int y, int z);
	
	// Проверить, есть ли руда в точке (x, y, z) на основе шума (OpenSimplex3D)
	bool CheckOreNoiseAt(OpenSimplex3D& noise, int x, int y, int z);
	
	// ==================== Работа с водой ====================
	
	// Заполнение водой (Flood Fill алгоритм)
	// _cols - карта цветов/состояний (0 = пусто, _colWater = вода, _colBorder = граница)
	// _waterChunks16x16Height - массив высот воды для чанков 16x16
	// _heightMap - высотная карта
	// _posX, _posZ - начальная позиция
	// _maxY - максимальная высота воды
	// _colWater - цвет/значение для воды
	// _colBorder - цвет/значение для границы
	// _listPos - список позиций для обработки (используется как стек)
	void WaterFloodFill(
		std::vector<uint8_t>& cols,           // Карта состояний
		std::vector<uint8_t>& waterChunks16x16Height, // Высоты воды для чанков
		int width,                            // Ширина карты
		ChunkManager* heightMap,              // Менеджер чанков для получения высоты
		int posX,                            // Начальная X позиция
		int maxY,                            // Максимальная высота воды
		int posZ,                            // Начальная Z позиция
		uint8_t colWater,                    // Значение для воды
		uint8_t colBorder,                   // Значение для границы
		std::vector<glm::ivec2>& listPos,    // Список позиций (стек)
		int minX = INT_MIN,                  // Минимальная X граница
		int maxX = INT_MAX,                   // Максимальная X граница
		int minZ = INT_MIN,                  // Минимальная Z граница
		int maxZ = INT_MAX,                   // Максимальная Z граница
		int worldScale = 1                    // Масштаб мира
	);
	
	// ==================== Работа с направлениями ====================
	
	// Получить направление по нормали (2D)
	DirEightWay GetDirByNormal(const glm::vec2& normal);
	
	// Получить направление по нормали (2D, целочисленная версия)
	DirEightWay GetDirByNormal(const glm::ivec2& normal);
	
	// Получить ближайшее направление по углу поворота
	// _rotation - угол в градусах (0-360)
	// _limitTo90Degrees - ограничить до 4 направлений (N, E, S, W)
	DirEightWay GetClosestDirection(float rotation, bool limitTo90Degrees = false);
	
	// ==================== Работа с блоками ====================
	
	// Получить нормаль из информации о попадании
	// Используется для определения грани блока при клике
	glm::vec3 GetNormalFromHitInfo(
		const glm::ivec3& blockPos,
		const glm::vec3& hitPoint,
		const glm::vec3& hitNormal
	);
	
	// Получить грань блока из информации о попадании
	// Использует BlockFace из HitInfo.h
	HitInfo::BlockFace GetBlockFaceFromHitInfo(
		const glm::ivec3& blockPos,
		const glm::vec3& hitPoint,
		const glm::vec3& hitNormal
	);
	
	// Проверить, находится ли коллайдер в пределах блока
	// Используется для проверки пересечений при размещении блоков
	bool IsColliderWithinBlock(
		const glm::ivec3& blockPosition,
		uint8_t blockId,
		const glm::vec3& colliderCenter,
		const glm::vec3& colliderSize
	);
	
	// ==================== Работа с цветами ====================
	
	// Конвертировать uint32 в Color32 (RGBA)
	glm::u8vec4 UIntToColor(uint32_t color, bool includeAlpha = false);
	
	// Конвертировать Color32 в uint32
	uint32_t ColorToUInt(const glm::u8vec4& color, bool includeAlpha = false);
	
	// ==================== Работа со временем ====================
	
	// Конвертировать игровое время в дни
	int WorldTimeToDays(uint64_t worldTime);
	
	// Конвертировать игровое время в часы
	int WorldTimeToHours(uint64_t worldTime);
	
	// Конвертировать игровое время в минуты
	int WorldTimeToMinutes(uint64_t worldTime);
	
	// Конвертировать игровое время в общее количество секунд
	float WorldTimeToTotalSeconds(float worldTime);
	
	// Конвертировать игровое время в общее количество минут
	uint32_t WorldTimeToTotalMinutes(uint64_t worldTime);
	
	// Конвертировать игровое время в общее количество часов
	int WorldTimeToTotalHours(uint64_t worldTime);
	
	// Конвертировать общее количество минут в игровое время
	uint64_t TotalMinutesToWorldTime(uint32_t totalMinutes);
	
	// Конвертировать игровое время в строку (формат: "День ЧЧ:ММ")
	std::string WorldTimeToString(uint64_t worldTime);
	
	// Конвертировать дни в игровое время
	uint64_t DaysToWorldTime(int day);
	
	// Конвертировать день и время в игровое время
	uint64_t DayTimeToWorldTime(int day, int hours, int minutes);
	
	// ==================== Работа с валидацией ====================
	
	// Валидация имени игры (только буквы, цифры, подчеркивания, дефисы)
	char ValidateGameNameInput(const std::string& text, int charIndex, char addedChar);
	
	// Валидация имени игры (полная проверка)
	bool ValidateGameName(const std::string& gameName);
	
	// Валидация hex-ввода
	char ValidateHexInput(const std::string& text, int charIndex, char addedChar);
	
	// ==================== Работа с векторами ====================
	
	// Конвертировать Vector3i в uint64_t (для использования в качестве ключа)
	uint64_t Vector3iToUInt64(const glm::ivec3& v);
	
	// Конвертировать uint64_t в Vector3i
	glm::ivec3 UInt64ToVector3i(uint64_t fullValue);
	
	// ==================== Утилиты для отладки ====================
	
	// Безопасное форматирование строки (экранирование фигурных скобок)
	std::string SafeStringFormat(const std::string& s);
}

#endif /* VOXELS_GAMEUTILS_H_ */

