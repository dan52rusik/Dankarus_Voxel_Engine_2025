#ifndef VOXELS_PATHFINDINGCONSTANTS_H_
#define VOXELS_PATHFINDINGCONSTANTS_H_

#include <cstdint>
#include <glm/glm.hpp>

// Константы для pathfinding и навигации (из 7 Days To Die AstarVoxelGrid)
// Используются для определения проходимости, высот подъема/спуска и типов препятствий
namespace PathfindingConstants {
	// ==================== Высоты ====================
	
	// Высота сетки навигации
	static constexpr float GridHeight = 320.0f;
	static constexpr float GridHeightPadded = 320.01f;
	
	// Минимальная высота слоя (для навигации по разным уровням)
	static constexpr float LayerMinHeight = 0.7f;
	
	// Высоты подъема (climbing)
	static constexpr float ClimbMinHeight = 0.6f;   // Минимальная высота для подъема
	static constexpr float ClimbMaxHeight = 1.51f;  // Максимальная высота для подъема
	
	// Высоты спуска (dropping)
	static constexpr float DropOnTopHeight = 0.95f; // Высота для безопасного спуска на блок
	static constexpr float DropMaxHeight = 9.4f;    // Максимальная высота для падения
	
	// ==================== Штрафы (Penalties) ====================
	
	// Штраф за прыжок
	static constexpr int JumpPenalty = 8;
	
	// Штраф за метр расстояния
	static constexpr int PenaltyPerMeter = 1000;
	
	// Базовый штраф за здоровье блока
	static constexpr int PenaltyHealthBase = 10;
	static constexpr int PenaltyHealthScale = 20;
	
	// Dummy penalty (максимальное значение для "непроходимых" соединений)
	static constexpr uint32_t DummyPenalty = 0xFFFFFFFF;
	
	// ==================== Теги (Tags) ====================
	
	// Теги для узлов навигации
	static constexpr int TagOpen = 0;      // Открытое пространство
	static constexpr int TagBreak = 1;    // Разрушаемый блок
	static constexpr int TagLowHeight = 2; // Низкая высота
	static constexpr int TagDoor = 3;     // Дверь
	static constexpr int TagLadder = 4;   // Лестница
	static constexpr int TagTest = 8;     // Тестовый тег
	
	// ==================== Флаги блокировки (Blocker Flags) ====================
	
	// Низкие флаги (младшие 4 бита) - блокировка по направлениям (N, E, S, W)
	static constexpr uint16_t BlockerFlagLow0 = 0x0001;  // Бит 0
	static constexpr uint16_t BlockerFlagLow = 0x000F;   // Младшие 4 бита (0-3)
	
	// Высокие флаги (биты 4-7) - блокировка сверху по направлениям
	static constexpr uint16_t BlockerFlagHigh0 = 0x0010; // Бит 4
	static constexpr uint16_t BlockerFlagHigh = 0x00F0;   // Биты 4-7
	
	// Комбинация высоких и низких флагов
	static constexpr uint16_t BlockerFlagHighLow0 = 0x0011;
	static constexpr uint16_t BlockerFlagHighLow = 0x00FF;
	
	// Флаги направления склона (биты 8-11)
	static constexpr uint16_t BlockerFlagSlopeDir0 = 0x0100; // Бит 8
	
	// Специальные флаги
	static constexpr uint16_t BlockerFlagFloor = 0x1000;  // Пол (бит 12)
	static constexpr uint16_t BlockerFlagLadder = 0x2000; // Лестница (бит 13)
	static constexpr uint16_t BlockerFlagDoor = 0x4000;   // Дверь (бит 14)
	
	// ==================== Маска коллизий ====================
	
	// Маска для raycast (определяет, какие объекты учитываются)
	// В 7DTD: 0x40010000 (Layer 15 + Layer 9)
	static constexpr uint32_t CollisionMask = 0x40010000;
	
	// ==================== Вспомогательные функции ====================
	
	// Проверить, заблокировано ли направление (0-3: N, E, S, W)
	inline bool IsDirectionBlocked(uint16_t flags, int direction) {
		return (flags & (1 << direction)) != 0;
	}
	
	// Проверить, заблокировано ли направление сверху
	inline bool IsDirectionBlockedHigh(uint16_t flags, int direction) {
		return (flags & (0x10 << direction)) != 0;
	}
	
	// Проверить, есть ли пол
	inline bool HasFloor(uint16_t flags) {
		return (flags & BlockerFlagFloor) != 0;
	}
	
	// Проверить, есть ли лестница
	inline bool HasLadder(uint16_t flags) {
		return (flags & BlockerFlagLadder) != 0;
	}
	
	// Проверить, есть ли дверь
	inline bool HasDoor(uint16_t flags) {
		return (flags & BlockerFlagDoor) != 0;
	}
	
	// Установить флаг блокировки для направления
	inline uint16_t SetDirectionBlocked(uint16_t flags, int direction) {
		return flags | (1 << direction);
	}
	
	// Установить флаг блокировки сверху для направления
	inline uint16_t SetDirectionBlockedHigh(uint16_t flags, int direction) {
		return flags | (0x10 << direction);
	}
	
	// ==================== Структуры данных ====================
	
	// Данные о пересечении raycast (для pathfinding)
	struct HitData {
		glm::vec3 point;        // Точка пересечения
		uint16_t blockerFlags; // Флаги блокировки
		
		HitData() : point(0.0f), blockerFlags(0) {}
		HitData(const glm::vec3& p, uint16_t flags) : point(p), blockerFlags(flags) {}
	};
	
	// ==================== Смещения соседей ====================
	
	// Массив смещений для проверки 4 соседних ячеек (N, E, S, W)
	// Порядок: Север (0, -1), Восток (1, 0), Юг (0, 1), Запад (-1, 0)
	static constexpr glm::ivec2 NeighboursOffsetV2[4] = {
		glm::ivec2(0, -1),  // Север
		glm::ivec2(1,  0),  // Восток
		glm::ivec2(0,  1),  // Юг
		glm::ivec2(-1, 0)   // Запад
	};
	
	// Получить смещение соседа по направлению (0-3: N, E, S, W)
	inline glm::ivec2 GetNeighbourOffset(int direction) {
		if (direction >= 0 && direction < 4) {
			return NeighboursOffsetV2[direction];
		}
		return glm::ivec2(0, 0);
	}
	
	// Получить обратное направление (для двусторонних проверок)
	inline int GetOppositeDirection(int direction) {
		return (direction + 2) % 4; // N<->S, E<->W
	}
}

// ==================== Функции вычисления флагов блокировки ====================

// Вычислить флаги блокировки на основе raycast/коллизий
// pos - позиция для проверки
// offsetY - смещение по Y для проверки (0.0f = на уровне, 1.5f = выше)
// raycastCallback - функция для выполнения raycast (должна возвращать true если есть препятствие)
//   Сигнатура: bool raycastCallback(const glm::vec3& origin, const glm::vec3& direction, 
//                                   float distance, glm::vec3& hitPoint, glm::vec3& hitNormal)
//
// Возвращает флаги блокировки (биты 0-3: направления, биты 8-11: направления склонов)
//
// Пример использования:
//   auto raycast = [](const glm::vec3& origin, const glm::vec3& dir, float dist, 
//                     glm::vec3& hit, glm::vec3& norm) -> bool {
//       // Выполнить raycast через вашу физическую систему
//       return false; // или true если есть препятствие
//   };
//   uint16_t flags = CalcBlockingFlags(pos, 0.2f, raycast);
template<typename RaycastFunc>
uint16_t CalcBlockingFlags(const glm::vec3& pos, float offsetY, RaycastFunc raycastCallback) {
	uint16_t flags = 0;
	
	// Позиция для проверки (немного выше поверхности)
	glm::vec3 checkPos = pos;
	checkPos.y += 0.2f + offsetY;
	
	// Проверяем 4 направления (N, E, S, W)
	for (int dir = 0; dir < 4; dir++) {
		glm::ivec2 offset = PathfindingConstants::GetNeighbourOffset(dir);
		glm::vec3 direction(static_cast<float>(offset.x), 0.0f, static_cast<float>(offset.y));
		
		// Точка начала raycast (немного сдвинута назад)
		glm::vec3 origin = checkPos - direction * 0.2f;
		
		// Выполняем raycast
		glm::vec3 hitPoint, hitNormal;
		float distance = 0.59f; // Расстояние проверки
		
		if (raycastCallback(origin, direction, distance, hitPoint, hitNormal)) {
			// Есть препятствие
			// Проверяем угол наклона
			if (offsetY > 0.5f || hitNormal.y < 0.643f) {
				// Вертикальное препятствие или крутой склон
				flags |= (1 << dir);
			} else if (glm::dot(direction, hitNormal) > -0.35f) {
				// Препятствие под углом
				flags |= (1 << dir);
			} else {
				// Склон (можно подняться)
				flags |= (0x0100 << dir); // Флаг направления склона
			}
		}
	}
	
	return flags;
}

// Упрощенная версия без callback (для случаев, когда raycast уже выполнен)
// Используется для ручного вычисления флагов на основе известных препятствий
inline uint16_t CalcBlockingFlagsSimple(const glm::vec3& pos, float offsetY, 
                                        const bool* blockedDirections, 
                                        const bool* slopeDirections = nullptr) {
	uint16_t flags = 0;
	
	for (int dir = 0; dir < 4; dir++) {
		if (blockedDirections && blockedDirections[dir]) {
			flags |= (1 << dir);
		}
		if (slopeDirections && slopeDirections[dir]) {
			flags |= (0x0100 << dir); // Флаг направления склона
		}
	}
	
	return flags;
}

#endif /* VOXELS_PATHFINDINGCONSTANTS_H_ */

