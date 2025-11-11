#ifndef VOXELS_HITINFO_H_
#define VOXELS_HITINFO_H_

#include <glm/glm.hpp>
#include "voxel.h"

// Детальная информация о попадании при raycast
// Адаптировано из 7 Days To Die HitInfoDetails
namespace HitInfo {
	
	// Грань блока (направление нормали)
	enum class BlockFace {
		Bottom = 0,  // -Y (низ)
		Top = 1,     // +Y (верх)
		North = 2,   // -Z (север/зад)
		South = 3,   // +Z (юг/перед)
		West = 4,    // -X (запад/лево)
		East = 5     // +X (восток/право)
	};
	
	// Преобразовать нормаль в грань блока
	inline BlockFace normalToFace(const glm::vec3& normal) {
		// Находим компонент с максимальной абсолютной величиной
		float absX = std::abs(normal.x);
		float absY = std::abs(normal.y);
		float absZ = std::abs(normal.z);
		
		if (absY >= absX && absY >= absZ) {
			return (normal.y > 0.0f) ? BlockFace::Top : BlockFace::Bottom;
		} else if (absX >= absZ) {
			return (normal.x > 0.0f) ? BlockFace::East : BlockFace::West;
		} else {
			return (normal.z > 0.0f) ? BlockFace::South : BlockFace::North;
		}
	}
	
	// Структура для хранения данных о вокселе
	struct VoxelData {
		voxel* voxelPtr;  // Указатель на воксель (может быть nullptr)
		uint8_t blockId;  // ID блока (0 = воздух)
		
		VoxelData() : voxelPtr(nullptr), blockId(0) {}
		VoxelData(voxel* v) : voxelPtr(v), blockId(v ? v->id : 0) {}
		VoxelData(uint8_t id) : voxelPtr(nullptr), blockId(id) {}
		
		// Проверка на воздух
		bool isAir() const {
			return blockId == 0 || voxelPtr == nullptr;
		}
		
		// Очистить данные
		void clear() {
			voxelPtr = nullptr;
			blockId = 0;
		}
		
		// Сравнение
		bool equals(const VoxelData& other) const {
			return blockId == other.blockId;
		}
	};
	
	// Детальная информация о попадании
	struct HitInfoDetails {
		glm::vec3 pos;              // Точка попадания в мировых координатах
		glm::ivec3 blockPos;        // Позиция блока (целочисленные координаты)
		VoxelData voxelData;        // Данные о вокселе
		BlockFace blockFace;        // Грань блока, в которую попали
		float distanceSq;           // Квадрат расстояния до попадания (для сортировки)
		int clrIdx;                 // Индекс цвета/материала (для будущего использования)
		
		HitInfoDetails() 
			: pos(0.0f), blockPos(0), blockFace(BlockFace::Top), 
			  distanceSq(0.0f), clrIdx(0) {
		}
		
		// Очистить все данные
		void clear() {
			pos = glm::vec3(0.0f);
			blockPos = glm::ivec3(0);
			blockFace = BlockFace::Top;
			voxelData.clear();
			clrIdx = 0;
			distanceSq = 0.0f;
		}
		
		// Копировать из другого HitInfoDetails
		void copyFrom(const HitInfoDetails& other) {
			pos = other.pos;
			blockPos = other.blockPos;
			blockFace = other.blockFace;
			voxelData = other.voxelData;
			clrIdx = other.clrIdx;
			distanceSq = other.distanceSq;
		}
		
		// Создать копию
		HitInfoDetails clone() const {
			HitInfoDetails result;
			result.copyFrom(*this);
			return result;
		}
		
		// Установить данные из результата raycast
		void setFromRaycast(const glm::vec3& hitPos, const glm::vec3& hitNormal, 
		                   const glm::ivec3& hitBlockPos, voxel* hitVoxel, float distSq) {
			pos = hitPos;
			blockPos = hitBlockPos;
			voxelData = VoxelData(hitVoxel);
			blockFace = normalToFace(hitNormal);
			distanceSq = distSq;
		}
		
		// Проверка валидности (есть ли попадание)
		bool isValid() const {
			return !voxelData.isAir() && distanceSq > 0.0f;
		}
	};
	
} // namespace HitInfo

#endif /* VOXELS_HITINFO_H_ */

