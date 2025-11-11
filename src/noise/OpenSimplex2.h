#ifndef NOISE_OPENSIMPLEX2_H_
#define NOISE_OPENSIMPLEX2_H_

#include <cstdint>

// OpenSimplex2 - улучшенная версия алгоритма шума OpenSimplex
// Адаптировано из 7 Days To Die OpenSimplex2
namespace OpenSimplex2 {
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

#endif /* NOISE_OPENSIMPLEX2_H_ */

