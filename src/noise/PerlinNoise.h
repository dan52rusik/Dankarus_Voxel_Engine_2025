#ifndef NOISE_PERLINNOISE_H_
#define NOISE_PERLINNOISE_H_

#include <cstdint>

// PerlinNoise - классический алгоритм шума Perlin
// Адаптировано из 7 Days To Die PerlinNoise
class PerlinNoise {
private:
	static constexpr int GradientSizeTable = 256;
	static constexpr int cGradMask = 255;
	
	double _gradients[768]; // 256 * 3
	uint8_t _perm[256]; // Таблица перестановок
	
	// Приватные методы
	double Lattice(int ix, int iy, double fx, double fy);
	double Lattice(int ix, int iy, int iz, double fx, double fy, double fz);
	double Lerp(double t, double value0, double value1);
	double Smooth(double x);
	
public:
	explicit PerlinNoise(int64_t seed);
	
	// 2D шум (возвращает значение в диапазоне [-1, 1])
	double Noise(double x, double y);
	
	// 2D шум (возвращает значение в диапазоне [0, 1])
	double Noise01(double x, double y);
	
	// 3D шум (возвращает значение в диапазоне [-1, 1])
	double Noise(double x, double y, double z);
	
	// Fractal Brownian Motion (2D)
	double FBM(double x, double y, double freq);
};

#endif /* NOISE_PERLINNOISE_H_ */

