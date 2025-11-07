#ifndef NOISE_OPENSIMPLEX_H_
#define NOISE_OPENSIMPLEX_H_

#include <cstdint>

class OpenSimplex3D {
	int64_t seed;
public:
	explicit OpenSimplex3D(int64_t seed);
	// Single octave
	float noise(float x, float y, float z) const;
	// Fractal Brownian Motion
	float fbm(float x, float y, float z, int octaves, float lacunarity, float gain) const;
};

#endif /* NOISE_OPENSIMPLEX_H_ */

