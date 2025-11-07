#include "OpenSimplex.h"
#include <cmath>

namespace {
	inline int fastFloor(float x) { return x >= 0 ? (int)x : (int)x - 1; }
	inline float lerp(float a, float b, float t) { return a + t * (b - a); }
	
	inline float grad(int hash, float x, float y, float z) {
		int h = hash & 15;
		float u = h < 8 ? x : y;
		float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
		return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
	}
	
	inline uint32_t mix32(uint32_t x) {
		x ^= x >> 16; x *= 0x7feb352dU;
		x ^= x >> 15; x *= 0x846ca68bU;
		x ^= x >> 16; return x;
	}
}

OpenSimplex3D::OpenSimplex3D(int64_t s) : seed(s) {}

float OpenSimplex3D::noise(float x, float y, float z) const {
	int X = fastFloor(x);
	int Y = fastFloor(y);
	int Z = fastFloor(z);
	x -= X; y -= Y; z -= Z;
	X &= 255; Y &= 255; Z &= 255;

	uint32_t s = (uint32_t)seed;
	uint32_t A = mix32(s + X), AA = mix32(A + Y), AB = mix32(A + Y + 1);
	uint32_t B = mix32(s + X + 1), BA = mix32(B + Y), BB = mix32(B + Y + 1);

	float u = x * x * x * (x * (x * 6 - 15) + 10);
	float v = y * y * y * (y * (y * 6 - 15) + 10);
	float w = z * z * z * (z * (z * 6 - 15) + 10);

	float n000 = grad(mix32(AA + Z), x, y, z);
	float n001 = grad(mix32(AA + Z + 1), x, y, z - 1.0f);
	float n010 = grad(mix32(AB + Z), x, y - 1.0f, z);
	float n011 = grad(mix32(AB + Z + 1), x, y - 1.0f, z - 1.0f);
	float n100 = grad(mix32(BA + Z), x - 1.0f, y, z);
	float n101 = grad(mix32(BA + Z + 1), x - 1.0f, y, z - 1.0f);
	float n110 = grad(mix32(BB + Z), x - 1.0f, y - 1.0f, z);
	float n111 = grad(mix32(BB + Z + 1), x - 1.0f, y - 1.0f, z - 1.0f);

	float nx00 = lerp(n000, n100, u);
	float nx01 = lerp(n001, n101, u);
	float nx10 = lerp(n010, n110, u);
	float nx11 = lerp(n011, n111, u);
	float nxy0 = lerp(nx00, nx10, v);
	float nxy1 = lerp(nx01, nx11, v);
	float nxyz = lerp(nxy0, nxy1, w);
	return nxyz;
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

