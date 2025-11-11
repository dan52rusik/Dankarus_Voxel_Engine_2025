#include "vp_FractalNoise.h"
#include <cmath>
#include <algorithm>

// Helper для clamp (если нет C++17)
static inline float clamp(float value, float min, float max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

vp_FractalNoise::vp_FractalNoise(float inH, float inLacunarity, float inOctaves)
	: vp_FractalNoise(inH, inLacunarity, inOctaves, nullptr) {
}

vp_FractalNoise::vp_FractalNoise(float inH, float inLacunarity, float inOctaves, vp_Perlin* noise) {
	m_Lacunarity = inLacunarity;
	m_Octaves = inOctaves;
	m_IntOctaves = static_cast<int>(inOctaves);
	m_Exponent.resize(m_IntOctaves + 1);
	
	float num = 1.0f;
	for (int index = 0; index < m_IntOctaves + 1; ++index) {
		m_Exponent[index] = static_cast<float>(std::pow(static_cast<double>(m_Lacunarity), -static_cast<double>(inH)));
		num *= m_Lacunarity;
	}
	
	if (noise == nullptr) {
		m_Noise = new vp_Perlin();
		m_OwnsNoise = true;
	} else {
		m_Noise = noise;
		m_OwnsNoise = false;
	}
}

vp_FractalNoise::~vp_FractalNoise() {
	// Удаляем только если мы создали его сами
	if (m_OwnsNoise && m_Noise != nullptr) {
		delete m_Noise;
		m_Noise = nullptr;
	}
}

float vp_FractalNoise::HybridMultifractal(float x, float y, float offset) {
	float num1 = (m_Noise->Noise(x, y) + offset) * m_Exponent[0];
	float num2 = num1;
	x *= m_Lacunarity;
	y *= m_Lacunarity;
	
	int index;
	for (index = 1; index < m_IntOctaves; ++index) {
		if (num2 > 1.0f) {
			num2 = 1.0f;
		}
		float num3 = (m_Noise->Noise(x, y) + offset) * m_Exponent[index];
		num1 += num2 * num3;
		num2 *= num3;
		x *= m_Lacunarity;
		y *= m_Lacunarity;
	}
	
	float num4 = m_Octaves - static_cast<float>(m_IntOctaves);
	return num1 + num4 * m_Noise->Noise(x, y) * m_Exponent[index];
}

float vp_FractalNoise::RidgedMultifractal(float x, float y, float offset, float gain) {
	float num1 = std::abs(m_Noise->Noise(x, y));
	float num2 = offset - num1;
	float num3 = num2 * num2;
	float num4 = num3;
	
	for (int index = 1; index < m_IntOctaves; ++index) {
		x *= m_Lacunarity;
		y *= m_Lacunarity;
		float num5 = clamp(num3 * gain, 0.0f, 1.0f);
		float num6 = std::abs(m_Noise->Noise(x, y));
		float num7 = offset - num6;
		num3 = num7 * num7 * num5;
		num4 += num3 * m_Exponent[index];
	}
	
	return num4;
}

float vp_FractalNoise::BrownianMotion(float x, float y) {
	float num1 = 0.0f;
	int index;
	for (index = 0; index < m_IntOctaves; ++index) {
		num1 += m_Noise->Noise(x, y) * m_Exponent[index];
		x *= m_Lacunarity;
		y *= m_Lacunarity;
	}
	
	float num2 = m_Octaves - static_cast<float>(m_IntOctaves);
	return num1 + num2 * m_Noise->Noise(x, y) * m_Exponent[index];
}

