#include "VoxelUtils.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <cctype>

namespace VoxelUtils {
	
	glm::ivec3 OneVoxelStep(
		const glm::ivec3& _voxelPos,
		const glm::vec3& _origin,
		const glm::vec3& _direction,
		glm::vec3& hitPos,
		HitInfo::BlockFace& blockFace
	) {
		int x = _voxelPos.x;
		int y = _voxelPos.y;
		int z = _voxelPos.z;
		
		int signX = (_direction.x > 0.0f) ? 1 : ((_direction.x < 0.0f) ? -1 : 0);
		int signY = (_direction.y > 0.0f) ? 1 : ((_direction.y < 0.0f) ? -1 : 0);
		int signZ = (_direction.z > 0.0f) ? 1 : ((_direction.z < 0.0f) ? -1 : 0);
		
		glm::ivec3 nextVoxel(
			x + (signX > 0 ? 1 : 0),
			y + (signY > 0 ? 1 : 0),
			z + (signZ > 0 ? 1 : 0)
		);
		
		constexpr float epsilon = 1e-6f;
		constexpr float maxFloat = std::numeric_limits<float>::max();
		
		glm::vec3 tMax(
			(std::abs(_direction.x) > epsilon) ? ((float)nextVoxel.x - _origin.x) / _direction.x : maxFloat,
			(std::abs(_direction.y) > epsilon) ? ((float)nextVoxel.y - _origin.y) / _direction.y : maxFloat,
			(std::abs(_direction.z) > epsilon) ? ((float)nextVoxel.z - _origin.z) / _direction.z : maxFloat
		);
		
		glm::vec3 tDelta(
			(std::abs(_direction.x) > epsilon) ? (float)signX / _direction.x : maxFloat,
			(std::abs(_direction.y) > epsilon) ? (float)signY / _direction.y : maxFloat,
			(std::abs(_direction.z) > epsilon) ? (float)signZ / _direction.z : maxFloat
		);
		
		hitPos = _origin;
		blockFace = HitInfo::BlockFace::Top;
		
		if (tMax.x < tMax.y && tMax.x < tMax.z && signX != 0) {
			x += signX;
			hitPos = _origin + tMax.x * _direction;
			blockFace = (signX > 0) ? HitInfo::BlockFace::West : HitInfo::BlockFace::East;
		} else if (tMax.y < tMax.z && signY != 0) {
			y += signY;
			hitPos = _origin + tMax.y * _direction;
			blockFace = (signY > 0) ? HitInfo::BlockFace::Bottom : HitInfo::BlockFace::Top;
		} else if (signZ != 0) {
			z += signZ;
			hitPos = _origin + tMax.z * _direction;
			blockFace = (signZ > 0) ? HitInfo::BlockFace::South : HitInfo::BlockFace::North;
		} else {
			// Ошибка: все направления нулевые
			return glm::ivec3(0);
		}
		
		return glm::ivec3(x, y, z);
	}
	
	int ToHitMask(const std::string& maskNames) {
		int hitMask = 0;
		
		if (maskNames.empty()) {
			return hitMask;
		}
		
		std::istringstream iss(maskNames);
		std::string token;
		
		while (std::getline(iss, token, ',')) {
			// Убираем пробелы
			token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
			
			// Преобразуем в нижний регистр для сравнения
			std::string lowerToken = token;
			std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), ::tolower);
			
			if (lowerToken == "arrow" || lowerToken == "arrows") {
				hitMask |= HM_Arrows;
			} else if (lowerToken == "bullet" || lowerToken == "bullets") {
				hitMask |= HM_Bullet;
			} else if (lowerToken == "liquidonly" || lowerToken == "liquid") {
				hitMask |= HM_LiquidOnly;
			} else if (lowerToken == "melee") {
				hitMask |= HM_Melee;
			} else if (lowerToken == "moveable" || lowerToken == "movable") {
				hitMask |= HM_Moveable;
			} else if (lowerToken == "notmoveable" || lowerToken == "notmovable") {
				hitMask |= HM_NotMoveable;
			} else if (lowerToken == "rocket" || lowerToken == "rockets") {
				hitMask |= HM_Rocket;
			} else if (lowerToken == "transparent") {
				hitMask |= HM_Transparent;
			}
		}
		
		return hitMask;
	}
	
} // namespace VoxelUtils

