#ifndef NOISE_OPENSIMPLEX2S_H_
#define NOISE_OPENSIMPLEX2S_H_

#include <cstdint>

// OpenSimplex2S - улучшенная и оптимизированная версия алгоритма шума OpenSimplex
// Адаптировано из 7 Days To Die OpenSimplex2S
namespace OpenSimplex2S {
	// 2D шум
	float Noise2(int64_t seed, double x, double y);
	float Noise2_ImproveX(int64_t seed, double x, double y);
	
	// 3D шум
	float Noise3_ImproveXY(int64_t seed, double x, double y, double z);
	float Noise3_ImproveXZ(int64_t seed, double x, double y, double z);
	float Noise3_Fallback(int64_t seed, double x, double y, double z);
	
	// 4D шум
	float Noise4_ImproveXYZ_ImproveXY(int64_t seed, double x, double y, double z, double w);
	float Noise4_ImproveXYZ_ImproveXZ(int64_t seed, double x, double y, double z, double w);
	float Noise4_ImproveXYZ(int64_t seed, double x, double y, double z, double w);
	float Noise4_Fallback(int64_t seed, double x, double y, double z, double w);
	
	// Инициализация (вызывается автоматически при первом использовании)
	void Initialize();
}

#endif /* NOISE_OPENSIMPLEX2S_H_ */

