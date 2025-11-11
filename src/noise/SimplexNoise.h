#ifndef NOISE_SIMPLEXNOISE_H_
#define NOISE_SIMPLEXNOISE_H_

// SimplexNoise - классический алгоритм шума Simplex (3D)
// Адаптировано из 7 Days To Die SimplexNoise
namespace SimplexNoise {
	// 3D шум (возвращает значение в диапазоне примерно [-1, 1])
	float noise(float x, float y, float z);
}

#endif /* NOISE_SIMPLEXNOISE_H_ */

