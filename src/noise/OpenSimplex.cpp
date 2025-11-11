#include "OpenSimplex.h"
#include "OpenSimplex2S.h"
#include <cmath>

// OpenSimplex3D теперь использует улучшенный OpenSimplex2S из 7 Days To Die
OpenSimplex3D::OpenSimplex3D(int64_t s) : seed(s) {}

float OpenSimplex3D::noise(float x, float y, float z) const {
	// Используем улучшенный алгоритм OpenSimplex2S::Noise3_ImproveXY
	// Это обеспечивает лучшую производительность и качество шума
	return OpenSimplex2S::Noise3_ImproveXY(seed, static_cast<double>(x), static_cast<double>(y), static_cast<double>(z));
}

float OpenSimplex3D::fbm(float x, float y, float z, int octaves, float lacunarity, float gain) const {
	float sum = 0.0f;
	float amp = 0.5f;
	float freq = 1.0f;
	for (int i = 0; i < octaves; i++) {
		sum += amp * noise(x * freq, y * freq, z * freq);
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}
