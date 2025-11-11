#include "vp_Perlin.h"
#include <cmath>
#include <random>

vp_Perlin::vp_Perlin() {
	// Используем случайный seed
	std::random_device rd;
	std::mt19937 rng(rd());
	init(rng);
}

vp_Perlin::vp_Perlin(int seed) {
	std::mt19937 rng(static_cast<unsigned int>(seed));
	init(rng);
}

void vp_Perlin::init(std::mt19937& rng) {
	// Инициализируем таблицу перестановок
	for (int i = 0; i < 256; ++i) {
		p[i] = i;
		
		// 1D градиенты
		g1[i] = static_cast<float>(rng() % 512 - 256) / 256.0f;
		
		// 2D градиенты
		for (int j = 0; j < 2; ++j) {
			g2[i][j] = static_cast<float>(rng() % 512 - 256) / 256.0f;
		}
		normalize2(g2[i][0], g2[i][1]);
		
		// 3D градиенты
		for (int j = 0; j < 3; ++j) {
			g3[i][j] = static_cast<float>(rng() % 512 - 256) / 256.0f;
		}
		normalize3(g3[i][0], g3[i][1], g3[i][2]);
	}
	
	// Перемешиваем таблицу перестановок
	for (int i = 255; i > 0; --i) {
		int j = rng() % 256;
		int temp = p[i];
		p[i] = p[j];
		p[j] = temp;
	}
	
	// Дублируем для обертки
	for (int i = 0; i < 258; ++i) {
		p[256 + i] = p[i];
		g1[256 + i] = g1[i];
		for (int j = 0; j < 2; ++j) {
			g2[256 + i][j] = g2[i][j];
		}
		for (int j = 0; j < 3; ++j) {
			g3[256 + i][j] = g3[i][j];
		}
	}
}

float vp_Perlin::s_curve(float t) {
	return t * t * (3.0f - 2.0f * t);
}

float vp_Perlin::lerp(float t, float a, float b) {
	return a + t * (b - a);
}

void vp_Perlin::setup(float value, int& b0, int& b1, float& r0, float& r1) {
	float num = value + 4096.0f;
	b0 = static_cast<int>(num) & 255;
	b1 = (b0 + 1) & 255;
	r0 = num - static_cast<float>(static_cast<int>(num));
	r1 = r0 - 1.0f;
}

float vp_Perlin::at2(float rx, float ry, float x, float y) {
	return rx * x + ry * y;
}

float vp_Perlin::at3(float rx, float ry, float rz, float x, float y, float z) {
	return rx * x + ry * y + rz * z;
}

void vp_Perlin::normalize2(float& x, float& y) {
	float num = std::sqrt(x * x + y * y);
	if (num > 0.0f) {
		x /= num;
		y /= num;
	}
}

void vp_Perlin::normalize3(float& x, float& y, float& z) {
	float num = std::sqrt(x * x + y * y + z * z);
	if (num > 0.0f) {
		x /= num;
		y /= num;
		z /= num;
	}
}

float vp_Perlin::Noise(float arg) {
	int b0, b1;
	float r0, r1;
	setup(arg, b0, b1, r0, r1);
	return lerp(s_curve(r0), r0 * g1[p[b0]], r1 * g1[p[b1]]);
}

float vp_Perlin::Noise(float x, float y) {
	int b0_1, b1_1;
	float r0_1, r1_1;
	setup(x, b0_1, b1_1, r0_1, r1_1);
	
	int b0_2, b1_2;
	float r0_2, r1_2;
	setup(y, b0_2, b1_2, r0_2, r1_2);
	
	int num1 = p[b0_1];
	int num2 = p[b1_1];
	int index1 = p[num1 + b0_2];
	int index2 = p[num2 + b0_2];
	int index3 = p[num1 + b1_2];
	int index4 = p[num2 + b1_2];
	
	float t1 = s_curve(r0_1);
	float t2 = s_curve(r0_2);
	
	float a1 = at2(r0_1, r0_2, g2[index1][0], g2[index1][1]);
	float b1 = at2(r1_1, r0_2, g2[index2][0], g2[index2][1]);
	float a2 = lerp(t1, a1, b1);
	
	float a3 = at2(r0_1, r1_2, g2[index3][0], g2[index3][1]);
	float b2 = at2(r1_1, r1_2, g2[index4][0], g2[index4][1]);
	float b3 = lerp(t1, a3, b2);
	
	return lerp(t2, a2, b3);
}

float vp_Perlin::Noise(float x, float y, float z) {
	int b0_1, b1_1;
	float r0_1, r1_1;
	setup(x, b0_1, b1_1, r0_1, r1_1);
	
	int b0_2, b1_2;
	float r0_2, r1_2;
	setup(y, b0_2, b1_2, r0_2, r1_2);
	
	int b0_3, b1_3;
	float r0_3, r1_3;
	setup(z, b0_3, b1_3, r0_3, r1_3);
	
	int num1 = p[b0_1];
	int num2 = p[b1_1];
	int num3 = p[num1 + b0_2];
	int num4 = p[num2 + b0_2];
	int num5 = p[num1 + b1_2];
	int num6 = p[num2 + b1_2];
	
	float t1 = s_curve(r0_1);
	float t2 = s_curve(r0_2);
	float t3 = s_curve(r0_3);
	
	float a1 = at3(r0_1, r0_2, r0_3, g3[num3 + b0_3][0], g3[num3 + b0_3][1], g3[num3 + b0_3][2]);
	float b1 = at3(r1_1, r0_2, r0_3, g3[num4 + b0_3][0], g3[num4 + b0_3][1], g3[num4 + b0_3][2]);
	float a2 = lerp(t1, a1, b1);
	
	float a3 = at3(r0_1, r1_2, r0_3, g3[num5 + b0_3][0], g3[num5 + b0_3][1], g3[num5 + b0_3][2]);
	float b2 = at3(r1_1, r1_2, r0_3, g3[num6 + b0_3][0], g3[num6 + b0_3][1], g3[num6 + b0_3][2]);
	float b3 = lerp(t1, a3, b2);
	
	float a4 = lerp(t2, a2, b3);
	
	float a5 = at3(r0_1, r0_2, r1_3, g3[num3 + b1_3][0], g3[num3 + b1_3][1], g3[num3 + b1_3][2]);
	float b4 = at3(r1_1, r0_2, r1_3, g3[num4 + b1_3][0], g3[num4 + b1_3][1], g3[num4 + b1_3][2]);
	float a6 = lerp(t1, a5, b4);
	
	float a7 = at3(r0_1, r1_2, r1_3, g3[num5 + b1_3][0], g3[num5 + b1_3][1], g3[num5 + b1_3][2]);
	float b5 = at3(r1_1, r1_2, r1_3, g3[num6 + b1_3][0], g3[num6 + b1_3][1], g3[num6 + b1_3][2]);
	float b6 = lerp(t1, a7, b5);
	
	float b7 = lerp(t2, a6, b6);
	
	return lerp(t3, a4, b7);
}

