#include "PerlinNoise.h"
#include <cmath>
#include <random>
#include <algorithm>

PerlinNoise::PerlinNoise(int64_t seed) {
	// Инициализируем таблицу перестановок (из C# кода)
	uint8_t permBase[] = {
		225, 155, 210, 108, 175, 199, 221, 144, 203, 116, 70, 213, 69, 158, 33, 252,
		5, 82, 173, 133, 222, 139, 174, 27, 9, 71, 90, 246, 75, 130, 91, 191,
		169, 138, 2, 151, 194, 235, 81, 7, 25, 113, 228, 159, 205, 253, 134, 142,
		248, 65, 224, 217, 22, 121, 229, 63, 89, 103, 96, 104, 156, 17, 201, 129,
		36, 8, 165, 110, 237, 117, 231, 56, 132, 211, 152, 20, 181, 111, 239, 218,
		170, 163, 51, 172, 157, 47, 80, 212, 176, 250, 87, 49, 99, 242, 136, 189,
		162, 115, 44, 43, 124, 94, 150, 16, 141, 247, 32, 10, 198, 223, 255, 72,
		53, 131, 84, 57, 220, 197, 58, 50, 208, 11, 241, 28, 3, 192, 62, 202,
		18, 215, 153, 24, 76, 41, 15, 179, 39, 46, 55, 6, 128, 167, 23, 188,
		106, 34, 187, 140, 164, 73, 112, 182, 244, 195, 227, 13, 35, 77, 196, 185,
		26, 200, 226, 119, 31, 123, 168, 125, 249, 68, 183, 230, 177, 135, 160, 180,
		12, 1, 243, 148, 102, 166, 38, 238, 251, 37, 240, 126, 64, 74, 161, 40,
		184, 149, 171, 178, 101, 66, 29, 59, 146, 61, 254, 107, 42, 86, 154, 4,
		236, 232, 120, 21, 233, 209, 45, 98, 193, 114, 78, 19, 206, 14, 118, 127,
		48, 79, 147, 85, 30, 207, 219, 54, 88, 234, 190, 122, 95, 67, 143, 109,
		137, 214, 145, 93, 92, 100, 245, 0, 216, 186, 60, 83, 105, 97, 204, 52
	};
	
	std::copy(permBase, permBase + 256, _perm);
	
	// Генерируем градиенты на основе seed
	// Используем std::mt19937 вместо GameRandom
	std::mt19937 rng(static_cast<unsigned int>(seed));
	std::uniform_real_distribution<double> dist(0.0, 1.0);
	
	for (int index = 0; index < 256; ++index) {
		double num1 = 1.0 - 2.0 * dist(rng);
		double num2 = std::sqrt(1.0 - num1 * num1);
		double num3 = 2.0 * M_PI * dist(rng);
		_gradients[index * 3] = num2 * std::cos(num3);
		_gradients[index * 3 + 1] = num2 * std::sin(num3);
		_gradients[index * 3 + 2] = num1;
	}
}

double PerlinNoise::Noise01(double x, double y) {
	return (Noise(x, y) + 1.0) * 0.5;
}

double PerlinNoise::Noise(double x, double y) {
	int ix = static_cast<int>(std::floor(x));
	double num1 = x - static_cast<double>(ix);
	double fx = num1 - 1.0;
	double t1 = Smooth(num1);
	
	int iy = static_cast<int>(std::floor(y));
	double num2 = y - static_cast<double>(iy);
	double fy = num2 - 1.0;
	double t2 = Smooth(num2);
	
	double num3 = Lattice(ix, iy, num1, num2);
	double num4 = Lattice(ix + 1, iy, fx, num2);
	double num5 = Lerp(t1, num3, num4);
	
	double num6 = Lattice(ix, iy + 1, num1, fy);
	double num7 = Lattice(ix + 1, iy + 1, fx, fy);
	double num8 = Lerp(t1, num6, num7);
	
	double num9 = Lerp(t2, num5, num8) * (20.0 / 11.0);
	
	if (num9 <= -1.0) {
		return -1.0;
	}
	return num9 >= 1.0 ? 1.0 : num9;
}

double PerlinNoise::Noise(double x, double y, double z) {
	int ix = static_cast<int>(std::floor(x));
	double num1 = x - static_cast<double>(ix);
	double fx = num1 - 1.0;
	double t1 = Smooth(num1);
	
	int iy = static_cast<int>(std::floor(y));
	double num2 = y - static_cast<double>(iy);
	double fy = num2 - 1.0;
	double t2 = Smooth(num2);
	
	int iz = static_cast<int>(std::floor(z));
	double num3 = z - static_cast<double>(iz);
	double fz = num3 - 1.0;
	double t3 = Smooth(num3);
	
	double num4 = Lattice(ix, iy, iz, num1, num2, num3);
	double num5 = Lattice(ix + 1, iy, iz, fx, num2, num3);
	double num6 = Lerp(t1, num4, num5);
	
	double num7 = Lattice(ix, iy + 1, iz, num1, fy, num3);
	double num8 = Lattice(ix + 1, iy + 1, iz, fx, fy, num3);
	double num9 = Lerp(t1, num7, num8);
	
	double num10 = Lerp(t2, num6, num9);
	
	double num11 = Lattice(ix, iy, iz + 1, num1, num2, fz);
	double num12 = Lattice(ix + 1, iy, iz + 1, fx, num2, fz);
	double num13 = Lerp(t1, num11, num12);
	
	double num14 = Lattice(ix, iy + 1, iz + 1, num1, fy, fz);
	double num15 = Lattice(ix + 1, iy + 1, iz + 1, fx, fy, fz);
	double num16 = Lerp(t1, num14, num15);
	
	double num17 = Lerp(t2, num13, num16);
	
	double num18 = Lerp(t3, num10, num17) * (20.0 / 11.0);
	
	if (num18 <= -1.0) {
		return -1.0;
	}
	return num18 >= 1.0 ? 1.0 : num18;
}

double PerlinNoise::FBM(double x, double y, double freq) {
	double num1 = 0.0;
	double num2 = 1.0;
	double num3 = 0.3; // gain
	double num4 = 2.1; // lacunarity
	double num5 = freq;
	
	for (int index = 0; index < 2; ++index) {
		double num6 = Noise(x * num5, y * num5);
		num1 += num6 * num2;
		num2 *= num3;
		num5 *= num4;
	}
	
	return num1;
}

double PerlinNoise::Lattice(int ix, int iy, double fx, double fy) {
	int index = static_cast<int>(_perm[ix + static_cast<int>(_perm[iy + 225 & cGradMask]) & cGradMask]) * 3;
	return _gradients[index] * fx + _gradients[index + 1] * fy;
}

double PerlinNoise::Lattice(int ix, int iy, int iz, double fx, double fy, double fz) {
	int index = static_cast<int>(_perm[ix + static_cast<int>(_perm[iy + static_cast<int>(_perm[iz & cGradMask]) & cGradMask]) & cGradMask]) * 3;
	return _gradients[index] * fx + _gradients[index + 1] * fy + _gradients[index + 2] * fz;
}

double PerlinNoise::Lerp(double t, double value0, double value1) {
	return value0 + t * (value1 - value0);
}

double PerlinNoise::Smooth(double x) {
	return x * x * (3.0 - 2.0 * x);
}

