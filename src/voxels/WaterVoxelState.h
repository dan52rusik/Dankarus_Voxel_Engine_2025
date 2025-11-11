#ifndef VOXELS_WATERVOXELSTATE_H_
#define VOXELS_WATERVOXELSTATE_H_

#include <cstdint>
#include <glm/glm.hpp>

// Состояние вокселя воды (какие грани твердые)
// Адаптировано из 7 Days To Die WaterVoxelState
struct WaterVoxelState {
	uint8_t stateBits; // Биты для каждой грани (6 граней = 6 бит)
	
	WaterVoxelState() : stateBits(0) {}
	WaterVoxelState(uint8_t bits) : stateBits(bits) {}
	
	// Проверка на значение по умолчанию (все грани проницаемы)
	bool isDefault() const {
		return stateBits == 0;
	}
	
	// Проверка твердости граней
	bool isSolidYPos() const { return (stateBits & 1) != 0; }  // +Y (верх)
	bool isSolidYNeg() const { return (stateBits & 2) != 0; }  // -Y (низ)
	bool isSolidZPos() const { return (stateBits & 4) != 0; }  // +Z (юг)
	bool isSolidZNeg() const { return (stateBits & 16) != 0; }  // -Z (север)
	bool isSolidXPos() const { return (stateBits & 32) != 0; }  // +X (восток)
	bool isSolidXNeg() const { return (stateBits & 8) != 0; }   // -X (запад)
	
	// Проверить, является ли воксель полностью твердым (все грани твердые)
	bool isSolid() const {
		return stateBits != 0 && (~stateBits & 63) == 0; // 63 = 0x3F (6 бит)
	}
	
	// Установить твердость граней (flags - битовая маска)
	void setSolid(uint8_t flags) {
		stateBits = flags;
	}
	
	// Установить/снять твердость конкретной грани
	void setSolidMask(uint8_t mask, bool value) {
		if (value) {
			stateBits |= mask;
		} else {
			stateBits &= ~mask;
		}
	}
	
	// Проверить твердость по направлению (для XZ плоскости)
	bool isSolidXZ(const glm::ivec2& side) const {
		if (side.x > 0) return isSolidXPos();
		if (side.x < 0) return isSolidXNeg();
		if (side.y > 0) return isSolidZPos();
		if (side.y < 0) return isSolidZNeg();
		return isSolid();
	}
	
	// Сравнение
	bool equals(const WaterVoxelState& other) const {
		return stateBits == other.stateBits;
	}
};

// Флаги граней блока (для воды)
namespace BlockFaceFlags {
	static constexpr uint8_t YPos = 1;   // +Y (верх)
	static constexpr uint8_t YNeg = 2;   // -Y (низ)
	static constexpr uint8_t ZPos = 4;   // +Z (юг)
	static constexpr uint8_t XNeg = 8;   // -X (запад)
	static constexpr uint8_t ZNeg = 16;  // -Z (север)
	static constexpr uint8_t XPos = 32;  // +X (восток)
	static constexpr uint8_t All = 63;   // Все грани (0x3F)
}

#endif /* VOXELS_WATERVOXELSTATE_H_ */

