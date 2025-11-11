#ifndef VOXELS_VOXELUTILS_H_
#define VOXELS_VOXELUTILS_H_

#include <glm/glm.hpp>
#include "HitInfo.h"
#include <cstdint>
#include <functional>
#include <string>

// Утилиты для работы с вокселями
// Адаптировано из 7 Days To Die Voxel
namespace VoxelUtils {
	
	// ==================== Константы HitMask ====================
	// Маски для различных типов попаданий при raycast
	static constexpr int HM_All = 4095;          // 0x0FFF - все типы
	static constexpr int HM_None = 0;            // Нет попаданий
	static constexpr int HM_Transparent = 1;     // Прозрачные блоки
	static constexpr int HM_LiquidOnly = 2;      // Только жидкости
	static constexpr int HM_Moveable = 4;        // Подвижные блоки
	static constexpr int HM_Bullet = 8;          // Пули
	static constexpr int HM_Rocket = 16;         // 0x10 - ракеты
	static constexpr int HM_Arrows = 32;        // 0x20 - стрелы
	static constexpr int HM_NotMoveable = 64;    // 0x40 - неподвижные блоки
	static constexpr int HM_Melee = 128;         // 0x80 - ближний бой
	static constexpr int HM_FirstNotEmptyBlock = 256; // 0x0100 - первый непустой блок
	
	// ==================== Нормали граней блоков ====================
	// Порядок: Bottom, North, West, Top, South, East
	// Соответствует BlockFace enum
	static constexpr glm::ivec3 normalsI[6] = {
		glm::ivec3(0, -1, 0),  // Bottom (-Y)
		glm::ivec3(0, 0, -1),  // North (-Z)
		glm::ivec3(-1, 0, 0),  // West (-X)
		glm::ivec3(0, 1, 0),   // Top (+Y)
		glm::ivec3(0, 0, 1),   // South (+Z)
		glm::ivec3(1, 0, 0)    // East (+X)
	};
	
	static constexpr glm::vec3 normals[6] = {
		glm::vec3(0.0f, -1.0f, 0.0f),  // Bottom
		glm::vec3(0.0f, 0.0f, -1.0f),  // North
		glm::vec3(-1.0f, 0.0f, 0.0f), // West
		glm::vec3(0.0f, 1.0f, 0.0f),  // Top
		glm::vec3(0.0f, 0.0f, 1.0f),  // South
		glm::vec3(1.0f, 0.0f, 0.0f)   // East
	};
	
	// Маппинг нормалей на грани блоков
	static constexpr HitInfo::BlockFace normalToFaces[6] = {
		HitInfo::BlockFace::Bottom,
		HitInfo::BlockFace::North,
		HitInfo::BlockFace::West,
		HitInfo::BlockFace::Top,
		HitInfo::BlockFace::South,
		HitInfo::BlockFace::East
	};
	
	// ==================== Функции ====================
	
	// Перемещение на один воксель по лучу (DDA алгоритм)
	// Адаптировано из Voxel.OneVoxelStep
	// _voxelPos - текущая позиция вокселя (целочисленные координаты)
	// _origin - начальная точка луча
	// _direction - направление луча (нормализованное)
	// hitPos - выходная точка попадания
	// blockFace - выходная грань блока, в которую попали
	// Возвращает новую позицию вокселя
	glm::ivec3 OneVoxelStep(
		const glm::ivec3& _voxelPos,
		const glm::vec3& _origin,
		const glm::vec3& _direction,
		glm::vec3& hitPos,
		HitInfo::BlockFace& blockFace
	);
	
	// Итерация по всем ячейкам на пути луча
	// callback вызывается для каждой ячейки: callback(x, y, z)
	// Возвращает false, чтобы остановить итерацию
	// Адаптировано из Voxel.GetCellsOnRay
	template<typename Callback>
	void GetCellsOnRay(const glm::vec3& origin, const glm::vec3& direction, Callback callback) {
		glm::ivec3 voxelPos(
			static_cast<int>(std::floor(origin.x)),
			static_cast<int>(std::floor(origin.y)),
			static_cast<int>(std::floor(origin.z))
		);
		
		int x = voxelPos.x;
		int y = voxelPos.y;
		int z = voxelPos.z;
		
		int signX = (direction.x > 0.0f) ? 1 : ((direction.x < 0.0f) ? -1 : 0);
		int signY = (direction.y > 0.0f) ? 1 : ((direction.y < 0.0f) ? -1 : 0);
		int signZ = (direction.z > 0.0f) ? 1 : ((direction.z < 0.0f) ? -1 : 0);
		
		glm::ivec3 nextVoxel(
			x + (signX > 0 ? 1 : 0),
			y + (signY > 0 ? 1 : 0),
			z + (signZ > 0 ? 1 : 0)
		);
		
		constexpr float epsilon = 1e-6f;
		constexpr float maxFloat = std::numeric_limits<float>::max();
		
		glm::vec3 tMax(
			(std::abs(direction.x) > epsilon) ? ((float)nextVoxel.x - origin.x) / direction.x : maxFloat,
			(std::abs(direction.y) > epsilon) ? ((float)nextVoxel.y - origin.y) / direction.y : maxFloat,
			(std::abs(direction.z) > epsilon) ? ((float)nextVoxel.z - origin.z) / direction.z : maxFloat
		);
		
		glm::vec3 tDelta(
			(std::abs(direction.x) > epsilon) ? (float)signX / direction.x : maxFloat,
			(std::abs(direction.y) > epsilon) ? (float)signY / direction.y : maxFloat,
			(std::abs(direction.z) > epsilon) ? (float)signZ / direction.z : maxFloat
		);
		
		while (callback(x, y, z)) {
			if (tMax.x < tMax.y && tMax.x < tMax.z && signX != 0) {
				x += signX;
				tMax.x += tDelta.x;
			} else if (tMax.y < tMax.z && signY != 0) {
				y += signY;
				tMax.y += tDelta.y;
			} else if (signZ != 0) {
				z += signZ;
				tMax.z += tDelta.z;
			} else {
				// Ошибка: все направления нулевые
				break;
			}
		}
	}
	
	// Конвертировать строку с именами масок в битовую маску
	// Например: "Transparent,LiquidOnly" -> HM_Transparent | HM_LiquidOnly
	int ToHitMask(const std::string& maskNames);
	
} // namespace VoxelUtils

#endif /* VOXELS_VOXELUTILS_H_ */

