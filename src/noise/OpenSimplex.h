#ifndef NOISE_OPENSIMPLEX_H_
#define NOISE_OPENSIMPLEX_H_

#include <cstdint>

// OpenSimplex3D - обертка над OpenSimplex2S для совместимости
// Использует улучшенный алгоритм OpenSimplex2S из 7 Days To Die
class OpenSimplex3D {
	int64_t seed;
public:
	explicit OpenSimplex3D(int64_t seed);
	// Single octave (использует OpenSimplex2S::Noise3_ImproveXY)
	float noise(float x, float y, float z) const;
	// Fractal Brownian Motion
	float fbm(float x, float y, float z, int octaves, float lacunarity, float gain) const;
};

#endif /* NOISE_OPENSIMPLEX_H_ */

