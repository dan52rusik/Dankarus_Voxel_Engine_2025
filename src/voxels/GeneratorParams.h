#pragma once

// Единая структура параметров генерации мира
// Используется везде для согласованности параметров
struct GeneratorParams {
	float baseFreq = 1.0f / 256.0f;
	int octaves = 5;
	float lacunarity = 2.0f;
	float gain = 0.5f;
	float baseHeight = 40.0f;
	float heightVariation = 240.0f;  // 220-260 для выразительных долин/хребтов
	float waterLevel = 38.0f; // baseHeight - 2.0f
	int64_t seed = 1337;
};

