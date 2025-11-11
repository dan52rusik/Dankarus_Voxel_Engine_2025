#ifndef VOXELS_WATERCLIPPINGVOLUME_H_
#define VOXELS_WATERCLIPPINGVOLUME_H_

#include "WaterClippingUtils.h"
#include <glm/glm.hpp>
#include <vector>

// Объем для обрезки воды по плоскости
// Адаптировано из 7 Days To Die WaterClippingVolume
class WaterClippingVolume {
public:
	WaterClippingVolume();
	~WaterClippingVolume();
	
	// Подготовить объем для обрезки по плоскости
	void Prepare(const WaterClippingUtils::Plane& waterClipPlane);
	
	// Применить обрезку к вершине (изменяет vertLocalPos)
	void ApplyClipping(glm::vec3& vertLocalPos) const;
	
	// Проверить, обрезан ли объем
	bool IsSliced() const { return isSliced; }
	
private:
	WaterClippingUtils::Plane waterClipPlane;
	std::vector<glm::vec3> intersectionPoints; // Точки пересечения плоскости с кубом
	int count;  // Количество точек пересечения
	bool isSliced; // Пересекает ли плоскость куб
};

#endif /* VOXELS_WATERCLIPPINGVOLUME_H_ */

