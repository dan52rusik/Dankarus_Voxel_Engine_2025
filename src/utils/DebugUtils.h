#ifndef UTILS_DEBUGUTILS_H_
#define UTILS_DEBUGUTILS_H_

#include <glm/glm.hpp>
#include <string>
#include <sstream>

// Утилиты для отладки и логирования
// Адаптировано из 7 Days To Die DynamicMeshVoxel
namespace DebugUtils {
	
	// Преобразовать координаты чанка в строку для отладки
	// Формат: "x,z" (как в 7DTD ToDebugLocation)
	inline std::string toDebugLocation(const glm::ivec3& pos) {
		std::ostringstream oss;
		oss << pos.x << "," << pos.z;
		return oss.str();
	}
	
	// Преобразовать координаты чанка в строку для отладки (полный формат)
	inline std::string toDebugLocationFull(const glm::ivec3& pos) {
		std::ostringstream oss;
		oss << pos.x << "," << pos.y << "," << pos.z;
		return oss.str();
	}
	
	// Преобразовать координаты в строку для отладки (только X, Z)
	inline std::string toDebugLocation2D(int x, int z) {
		std::ostringstream oss;
		oss << x << "," << z;
		return oss.str();
	}
	
} // namespace DebugUtils

#endif /* UTILS_DEBUGUTILS_H_ */

