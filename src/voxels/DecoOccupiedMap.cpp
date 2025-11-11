#include "DecoOccupiedMap.h"
#include <algorithm>
#include <cstring>

DecoOccupiedMap::DecoOccupiedMap(int _worldWidth, int _worldHeight)
	: worldWidth(_worldWidth), worldHeight(_worldHeight) {
	data.resize(worldWidth * worldHeight, EnumDecoOccupied::Free);
}

int DecoOccupiedMap::toIndex(int x, int z) const {
	// Преобразуем мировые координаты в индексы (центр в 0,0)
	int offsetX = x + worldWidth / 2;
	int offsetZ = z + worldHeight / 2;
	
	if (offsetX < 0 || offsetX >= worldWidth || offsetZ < 0 || offsetZ >= worldHeight) {
		return -1; // Вне границ
	}
	
	return offsetZ * worldWidth + offsetX;
}

bool DecoOccupiedMap::isValid(int x, int z) const {
	return toIndex(x, z) >= 0;
}

EnumDecoOccupied DecoOccupiedMap::get(int x, int z) const {
	int idx = toIndex(x, z);
	if (idx < 0) {
		return EnumDecoOccupied::NoneAllowed;
	}
	return data[idx];
}

void DecoOccupiedMap::set(int x, int z, EnumDecoOccupied value) {
	int idx = toIndex(x, z);
	if (idx >= 0) {
		data[idx] = value;
	}
}

void DecoOccupiedMap::setArea(int x, int z, EnumDecoOccupied value, int sizeX, int sizeZ) {
	for (int dz = 0; dz < sizeZ; dz++) {
		for (int dx = 0; dx < sizeX; dx++) {
			set(x + dx, z + dz, value);
		}
	}
}

bool DecoOccupiedMap::checkArea(int x, int z, EnumDecoOccupied value, int sizeX, int sizeZ) const {
	for (int dz = 0; dz < sizeZ; dz++) {
		for (int dx = 0; dx < sizeX; dx++) {
			if (get(x + dx, z + dz) == value) {
				return true;
			}
		}
	}
	return false;
}

