#include "VoxelCubeSurface.h"
#include <vector>

namespace VoxelCubeSurface {
	
	std::vector<glm::ivec3> generateCubeSurfacePositions(const glm::ivec3& origin, int radius) {
		std::vector<glm::ivec3> positions;
		
		if (radius <= 0) {
			positions.push_back(origin);
			return positions;
		}
		
		// Предварительно выделяем память (приблизительная оценка)
		// Для куба с radius: 6 граней, каждая (2*radius+1)^2 или (2*radius-1)*(2*radius+1)
		size_t estimatedSize = 6 * (2 * radius + 1) * (2 * radius + 1);
		positions.reserve(estimatedSize);
		
		generateCubeSurfacePositions(origin, radius, [&positions](const glm::ivec3& pos) {
			positions.push_back(pos);
		});
		
		return positions;
	}
	
} // namespace VoxelCubeSurface

