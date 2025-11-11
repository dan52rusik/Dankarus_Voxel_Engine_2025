#ifndef VOXELS_WATERCLIPPINGPLANE_H_
#define VOXELS_WATERCLIPPINGPLANE_H_

#include "WaterVoxelState.h"
#include <glm/glm.hpp>
#include <string>

// Настройки формы для обрезки воды
// Адаптировано из 7 Days To Die WaterClippingTool.ShapeSettings
struct ShapeSettings {
	std::string shapeName;
	glm::vec3 modelOffset;
	glm::vec4 plane; // Плоскость в формате (normal.x, normal.y, normal.z, distance)
	uint8_t waterFlowMask; // Маска для блокировки потока воды (BlockFaceFlags)
	
	// Проверка, активна ли плоскость
	bool hasPlane() const {
		return plane != DisabledPlaneVec;
	}
	
	// Сброс к значениям по умолчанию
	void ResetToDefault() {
		shapeName = "";
		modelOffset = DefaultModelOffset;
		plane = DisabledPlaneVec;
		waterFlowMask = BlockFaceFlags::All;
	}
	
	// Копирование из другого объекта
	void CopyFrom(const ShapeSettings& other) {
		shapeName = other.shapeName;
		modelOffset = other.modelOffset;
		plane = other.plane;
		waterFlowMask = other.waterFlowMask;
	}
	
	// Сравнение
	bool Equals(const ShapeSettings& other) const {
		return plane == other.plane &&
		       shapeName == other.shapeName &&
		       modelOffset == other.modelOffset &&
		       waterFlowMask == other.waterFlowMask;
	}
	
	// Статические константы
	static constexpr glm::vec4 DisabledPlaneVec = glm::vec4(0.0f, 1.0f, 0.0f, 1000.0f);
	static constexpr glm::vec3 DefaultModelOffset = glm::vec3(1.0f, 0.0f, 1.0f);
	
	ShapeSettings() {
		ResetToDefault();
	}
};

// Плоскость обрезки воды
// Адаптировано из 7 Days To Die WaterClippingPlanePlacer
class WaterClippingPlane {
public:
	// Статические константы
	static constexpr glm::vec4 DisabledPlaneVec = glm::vec4(0.0f, 1.0f, 0.0f, 1000.0f);
	static constexpr glm::vec3 DefaultModelOffset = glm::vec3(1.0f, 0.0f, 1.0f);
	
	// Настройки формы
	ShapeSettings liveSettings;
	
	// Позиция и ориентация плоскости
	glm::vec3 position;
	glm::vec3 forward; // Направление нормали плоскости
	
	WaterClippingPlane() 
		: position(0.0f), forward(0.0f, 0.0f, 1.0f) {
		liveSettings.ResetToDefault();
	}
	
	WaterClippingPlane(const glm::vec3& pos, const glm::vec3& fwd)
		: position(pos), forward(glm::normalize(fwd)) {
		liveSettings.ResetToDefault();
	}
	
	// Получить плоскость в формате (normal, distance)
	// Плоскость: normal.x * x + normal.y * y + normal.z * z + distance = 0
	glm::vec4 GetPlane() const {
		glm::vec3 normal = glm::normalize(forward);
		float distance = -glm::dot(normal, position);
		return glm::vec4(normal, distance);
	}
	
	// Проверить, находится ли точка на положительной стороне плоскости
	// (на стороне, куда направлена нормаль)
	bool IsPointOnPositiveSide(const glm::vec3& point) const {
		glm::vec4 plane = GetPlane();
		glm::vec3 normal(plane.x, plane.y, plane.z);
		float distance = plane.w;
		return glm::dot(normal, point) + distance >= 0.0f;
	}
	
	// Проверить, находится ли точка на отрицательной стороне плоскости
	bool IsPointOnNegativeSide(const glm::vec3& point) const {
		return !IsPointOnPositiveSide(point);
	}
	
	// Получить расстояние от точки до плоскости
	float GetDistanceToPlane(const glm::vec3& point) const {
		glm::vec4 plane = GetPlane();
		glm::vec3 normal(plane.x, plane.y, plane.z);
		float distance = plane.w;
		return glm::dot(normal, point) + distance;
	}
	
	// Проверить, активна ли плоскость
	bool IsActive() const {
		return liveSettings.hasPlane();
	}
	
	// Установить плоскость как неактивную
	void Disable() {
		liveSettings.plane = DisabledPlaneVec;
	}
	
	// Установить плоскость из позиции и направления
	void SetPlane(const glm::vec3& pos, const glm::vec3& fwd) {
		position = pos;
		forward = glm::normalize(fwd);
		glm::vec4 plane = GetPlane();
		liveSettings.plane = plane;
	}
};

#endif /* VOXELS_WATERCLIPPINGPLANE_H_ */

