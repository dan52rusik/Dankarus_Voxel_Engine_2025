#ifndef NOISE_VP_FRACTALNOISE_H_
#define NOISE_VP_FRACTALNOISE_H_

#include "vp_Perlin.h"
#include <vector>

// vp_FractalNoise - фрактальный шум с разными типами
// Адаптировано из 7 Days To Die vp_FractalNoise
class vp_FractalNoise {
private:
	vp_Perlin* m_Noise;
	bool m_OwnsNoise; // Флаг, указывающий, владеем ли мы указателем
	std::vector<float> m_Exponent;
	int m_IntOctaves;
	float m_Octaves;
	float m_Lacunarity;
	
public:
	// Конструкторы
	vp_FractalNoise(float inH, float inLacunarity, float inOctaves);
	vp_FractalNoise(float inH, float inLacunarity, float inOctaves, vp_Perlin* noise);
	~vp_FractalNoise();
	
	// Hybrid Multifractal - гибридный мультифрактал
	float HybridMultifractal(float x, float y, float offset);
	
	// Ridged Multifractal - гребневой мультифрактал (для острых хребтов)
	float RidgedMultifractal(float x, float y, float offset, float gain);
	
	// Brownian Motion - броуновское движение (классический FBM)
	float BrownianMotion(float x, float y);
};

#endif /* NOISE_VP_FRACTALNOISE_H_ */

