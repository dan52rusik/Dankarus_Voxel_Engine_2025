#ifndef NOISE_VP_SMOOTHRANDOM_H_
#define NOISE_VP_SMOOTHRANDOM_H_

#include <glm/glm.hpp>

// Forward declaration (vp_FractalNoise находится в глобальном namespace)
class vp_FractalNoise;

// vp_SmoothRandom - генератор плавных случайных значений
// Адаптировано из 7 Days To Die vp_SmoothRandom
// Используется для эффектов (камера, ветер, анимации)
namespace vp_SmoothRandom {
	// Получить Vector3 для плавного случайного движения
	glm::vec3 GetVector3(float speed);
	
	// Получить Vector3 с центрированием (для плавных переходов)
	glm::vec3 GetVector3Centered(float speed);
	
	// Получить Vector3 с центрированием и указанным временем
	glm::vec3 GetVector3Centered(float time, float speed);
	
	// Получить одно значение
	float Get(float speed);
	
	// Получить экземпляр vp_FractalNoise (для прямого использования)
	vp_FractalNoise* Get();
}

#endif /* NOISE_VP_SMOOTHRANDOM_H_ */

