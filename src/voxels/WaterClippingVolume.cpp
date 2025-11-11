#include "WaterClippingVolume.h"
#include "WaterClippingUtils.h"
#include <algorithm>

WaterClippingVolume::WaterClippingVolume()
	: count(-1), isSliced(false) {
	intersectionPoints.resize(6);
}

WaterClippingVolume::~WaterClippingVolume() {
}

void WaterClippingVolume::Prepare(const WaterClippingUtils::Plane& waterClipPlane) {
	this->waterClipPlane = waterClipPlane;
	this->isSliced = WaterClippingUtils::GetCubePlaneIntersectionEdgeLoop(
		waterClipPlane, intersectionPoints, count);
}

void WaterClippingVolume::ApplyClipping(glm::vec3& vertLocalPos) const {
	if (!isSliced || waterClipPlane.GetDistanceToPoint(vertLocalPos) <= 0.0f) {
		return; // Не обрезаем, если плоскость не пересекает куб или точка ниже плоскости
	}
	
	// Проецируем точку на плоскость
	vertLocalPos = waterClipPlane.ClosestPointOnPlane(vertLocalPos);
	
	// Если точка все еще внутри куба, оставляем как есть
	if (WaterClippingUtils::IsPointInCube(vertLocalPos)) {
		return;
	}
	
	// Иначе находим ближайшую точку на цикле пересечения
	vertLocalPos = WaterClippingUtils::NearestPointOnEdgeLoop(
		vertLocalPos, intersectionPoints, count);
}

