#include "vp_SmoothRandom.h"
#include "vp_FractalNoise.h"
#include <glm/glm.hpp>
#include <mutex>

namespace vp_SmoothRandom {
	static vp_FractalNoise* s_Noise = nullptr;
	static std::mutex s_Mutex;
	
	// Получить экземпляр vp_FractalNoise (thread-safe)
	vp_FractalNoise* Get() {
		std::lock_guard<std::mutex> lock(s_Mutex);
		if (s_Noise == nullptr) {
			// Параметры из оригинала: H=1.27f, Lacunarity=2.04f, Octaves=8.36f
			s_Noise = new vp_FractalNoise(1.27f, 2.04f, 8.36f);
		}
		return s_Noise;
	}
	
	glm::vec3 GetVector3(float speed) {
		// В оригинале используется Time.time, здесь нужно передавать время извне
		// Для совместимости используем статическую переменную
		static float currentTime = 0.0f;
		currentTime += 0.016f; // Примерно 60 FPS
		
		float x = currentTime * 0.01f * speed;
		vp_FractalNoise* noise = Get();
		return glm::vec3(
			noise->HybridMultifractal(x, 15.73f, 0.58f),
			noise->HybridMultifractal(x, 63.94f, 0.58f),
			noise->HybridMultifractal(x, 0.2f, 0.58f)
		);
	}
	
	glm::vec3 GetVector3Centered(float speed) {
		static float currentTime = 0.0f;
		currentTime += 0.016f;
		
		float x1 = currentTime * 0.01f * speed;
		float x2 = (currentTime - 1.0f) * 0.01f * speed;
		vp_FractalNoise* noise = Get();
		
		glm::vec3 v1(
			noise->HybridMultifractal(x1, 15.73f, 0.58f),
			noise->HybridMultifractal(x1, 63.94f, 0.58f),
			noise->HybridMultifractal(x1, 0.2f, 0.58f)
		);
		
		glm::vec3 v2(
			noise->HybridMultifractal(x2, 15.73f, 0.58f),
			noise->HybridMultifractal(x2, 63.94f, 0.58f),
			noise->HybridMultifractal(x2, 0.2f, 0.58f)
		);
		
		return v1 - v2;
	}
	
	glm::vec3 GetVector3Centered(float time, float speed) {
		float x1 = time * 0.01f * speed;
		float x2 = (time - 1.0f) * 0.01f * speed;
		vp_FractalNoise* noise = Get();
		
		glm::vec3 v1(
			noise->HybridMultifractal(x1, 15.73f, 0.58f),
			noise->HybridMultifractal(x1, 63.94f, 0.58f),
			noise->HybridMultifractal(x1, 0.2f, 0.58f)
		);
		
		glm::vec3 v2(
			noise->HybridMultifractal(x2, 15.73f, 0.58f),
			noise->HybridMultifractal(x2, 63.94f, 0.58f),
			noise->HybridMultifractal(x2, 0.2f, 0.58f)
		);
		
		return v1 - v2;
	}
	
	float Get(float speed) {
		static float currentTime = 0.0f;
		currentTime += 0.016f;
		
		float num = currentTime * 0.01f * speed;
		vp_FractalNoise* noise = Get();
		return noise->HybridMultifractal(num * 0.01f, 15.7f, 0.65f);
	}
}

