#ifndef NOISE_VP_PERLIN_H_
#define NOISE_VP_PERLIN_H_

#include <random>

// vp_Perlin - классический алгоритм Perlin шума
// Адаптировано из 7 Days To Die vp_Perlin
// Отличается от PerlinNoise - это другая реализация
class vp_Perlin {
private:
	static constexpr int B = 256;
	static constexpr int BM = 255;
	static constexpr int N = 4096;
	
	int p[514];
	float g3[514][3];
	float g2[514][2];
	float g1[514];
	
	float s_curve(float t);
	float lerp(float t, float a, float b);
	void setup(float value, int& b0, int& b1, float& r0, float& r1);
	float at2(float rx, float ry, float x, float y);
	float at3(float rx, float ry, float rz, float x, float y, float z);
	void normalize2(float& x, float& y);
	void normalize3(float& x, float& y, float& z);
	void init(std::mt19937& rng);
	
public:
	vp_Perlin();
	explicit vp_Perlin(int seed); // Конструктор с seed
	
	// 1D шум
	float Noise(float arg);
	
	// 2D шум
	float Noise(float x, float y);
	
	// 3D шум
	float Noise(float x, float y, float z);
};

#endif /* NOISE_VP_PERLIN_H_ */

