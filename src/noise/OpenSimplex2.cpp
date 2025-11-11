#include "OpenSimplex2.h"
#include <cmath>
#include <algorithm>
#include <mutex>

namespace OpenSimplex2 {
	// Константы
	static constexpr int64_t PRIME_X = 5910200641878280303LL;
	static constexpr int64_t PRIME_Y = 6452764530575939509LL;
	static constexpr int64_t PRIME_Z = 6614699811220273867LL;
	static constexpr int64_t PRIME_W = 6254464313819354443LL;
	static constexpr int64_t HASH_MULTIPLIER = 6026932503003350773LL;
	static constexpr int64_t SEED_FLIP_3D = -5968755714895566377LL;
	static constexpr int64_t SEED_OFFSET_4D = 1045921697555224141LL;
	
	static constexpr double ROOT2OVER2 = 0.70710678118654757;
	static constexpr double SKEW_2D = 0.366025403784439;
	static constexpr double UNSKEW_2D = -0.21132486540518713;
	static constexpr double ROOT3OVER3 = 0.577350269189626;
	static constexpr double FALLBACK_ROTATE_3D = 0.66666666666666663;
	static constexpr double ROTATE_3D_ORTHOGONALIZER = -0.21132486540518713;
	
	static constexpr float SKEW_4D = -0.1381966f;
	static constexpr float UNSKEW_4D = 0.309017f;
	static constexpr float LATTICE_STEP_4D = 0.2f;
	
	static constexpr int N_GRADS_2D_EXPONENT = 7;
	static constexpr int N_GRADS_3D_EXPONENT = 8;
	static constexpr int N_GRADS_4D_EXPONENT = 9;
	static constexpr int N_GRADS_2D = 128;
	static constexpr int N_GRADS_3D = 256;
	static constexpr int N_GRADS_4D = 512;
	
	static constexpr double NORMALIZER_2D = 0.01001634121365712;
	static constexpr double NORMALIZER_3D = 0.079698376689353312;
	static constexpr double NORMALIZER_4D = 0.0220065933241897;
	
	static constexpr float RSQUARED_2D = 0.5f;
	static constexpr float RSQUARED_3D = 0.6f;
	static constexpr float RSQUARED_4D = 0.6f;
	
	// Массивы градиентов
	static float GRADIENTS_2D[N_GRADS_2D * 2];
	static float GRADIENTS_3D[N_GRADS_3D * 3];
	static float GRADIENTS_4D[N_GRADS_4D * 4];
	
	// Флаг инициализации
	static bool initialized = false;
	static std::mutex initMutex;
	
	// Вспомогательные функции
	inline int FastFloor(double x) {
		int num = static_cast<int>(x);
		return x >= static_cast<double>(num) ? num : num - 1;
	}
	
	inline int FastRound(double x) {
		return x >= 0.0 ? static_cast<int>(x + 0.5) : static_cast<int>(x - 0.5);
	}
	
	inline float Grad(int64_t seed, int64_t xsvp, int64_t ysvp, float dx, float dy) {
		int64_t num1 = (seed ^ xsvp ^ ysvp) * HASH_MULTIPLIER;
		int num2 = static_cast<int>((num1 ^ (num1 >> 58)) & 254);
		return static_cast<float>(static_cast<double>(GRADIENTS_2D[num2 | 0]) * static_cast<double>(dx) + 
		                          static_cast<double>(GRADIENTS_2D[num2 | 1]) * static_cast<double>(dy));
	}
	
	inline float Grad(int64_t seed, int64_t xrvp, int64_t yrvp, int64_t zrvp, float dx, float dy, float dz) {
		int64_t num1 = (seed ^ xrvp ^ yrvp ^ zrvp) * HASH_MULTIPLIER;
		int num2 = static_cast<int>((num1 ^ (num1 >> 58)) & 1020);
		return static_cast<float>(static_cast<double>(GRADIENTS_3D[num2 | 0]) * static_cast<double>(dx) + 
		                          static_cast<double>(GRADIENTS_3D[num2 | 1]) * static_cast<double>(dy) + 
		                          static_cast<double>(GRADIENTS_3D[num2 | 2]) * static_cast<double>(dz));
	}
	
	inline float Grad(int64_t seed, int64_t xsvp, int64_t ysvp, int64_t zsvp, int64_t wsvp, float dx, float dy, float dz, float dw) {
		int64_t num1 = (seed ^ xsvp ^ ysvp ^ zsvp ^ wsvp) * HASH_MULTIPLIER;
		int num2 = static_cast<int>((num1 ^ (num1 >> 57)) & 2044);
		return static_cast<float>(static_cast<double>(GRADIENTS_4D[num2 | 0]) * static_cast<double>(dx) + 
		                          static_cast<double>(GRADIENTS_4D[num2 | 1]) * static_cast<double>(dy) + 
		                          (static_cast<double>(GRADIENTS_4D[num2 | 2]) * static_cast<double>(dz) + 
		                           static_cast<double>(GRADIENTS_4D[num2 | 3]) * static_cast<double>(dw)));
	}
	
	// Приватные методы
	static float Noise2_UnskewedBase(int64_t seed, double xs, double ys);
	static float Noise3_UnrotatedBase(int64_t seed, double xr, double yr, double zr);
	static float Noise4_UnskewedBase(int64_t seed, double xs, double ys, double zs, double ws);
	
	// Инициализация массивов градиентов
	void Initialize() {
		std::lock_guard<std::mutex> lock(initMutex);
		if (initialized) {
			return;
		}
		
		// Инициализация GRADIENTS_2D
		float gradients2DBase[] = {
			0.382683426f, 0.9238795f, 0.9238795f, 0.382683426f, 0.9238795f, -0.382683426f,
			0.382683426f, -0.9238795f, -0.382683426f, -0.9238795f, -0.9238795f, -0.382683426f,
			-0.9238795f, 0.382683426f, -0.382683426f, 0.9238795f, 0.130526185f, 0.9914449f,
			0.6087614f, 0.7933533f, 0.7933533f, 0.6087614f, 0.9914449f, 0.130526185f,
			0.9914449f, -0.130526185f, 0.7933533f, -0.6087614f, 0.6087614f, -0.7933533f,
			0.130526185f, -0.9914449f, -0.130526185f, -0.9914449f, -0.6087614f, -0.7933533f,
			-0.7933533f, -0.6087614f, -0.9914449f, -0.130526185f, -0.9914449f, 0.130526185f,
			-0.7933533f, 0.6087614f, -0.6087614f, 0.7933533f, -0.130526185f, 0.9914449f
		};
		
		for (size_t i = 0; i < sizeof(gradients2DBase) / sizeof(float); ++i) {
			gradients2DBase[i] = gradients2DBase[i] / 0.0100163408f;
		}
		
		int index1 = 0;
		int index2 = 0;
		while (index1 < N_GRADS_2D * 2) {
			if (index2 == sizeof(gradients2DBase) / sizeof(float)) {
				index2 = 0;
			}
			GRADIENTS_2D[index1] = gradients2DBase[index2];
			++index1;
			++index2;
		}
		
		// Инициализация GRADIENTS_3D
		float gradients3DBase[] = {
			2.2247448f, 2.2247448f, -1.0f, 0.0f, 2.2247448f, 2.2247448f, 1.0f, 0.0f,
			3.08626652f, 1.17215133f, 0.0f, 0.0f, 1.17215133f, 3.08626652f, 0.0f, 0.0f,
			-2.2247448f, 2.2247448f, -1.0f, 0.0f, -2.2247448f, 2.2247448f, 1.0f, 0.0f,
			-1.17215133f, 3.08626652f, 0.0f, 0.0f, -3.08626652f, 1.17215133f, 0.0f, 0.0f,
			-1.0f, -2.2247448f, -2.2247448f, 0.0f, 1.0f, -2.2247448f, -2.2247448f, 0.0f,
			0.0f, -3.08626652f, -1.17215133f, 0.0f, 0.0f, -1.17215133f, -3.08626652f, 0.0f,
			-1.0f, -2.2247448f, 2.2247448f, 0.0f, 1.0f, -2.2247448f, 2.2247448f, 0.0f,
			0.0f, -1.17215133f, 3.08626652f, 0.0f, 0.0f, -3.08626652f, 1.17215133f, 0.0f,
			-2.2247448f, -2.2247448f, -1.0f, 0.0f, -2.2247448f, -2.2247448f, 1.0f, 0.0f,
			-3.08626652f, -1.17215133f, 0.0f, 0.0f, -1.17215133f, -3.08626652f, 0.0f, 0.0f,
			-2.2247448f, -1.0f, -2.2247448f, 0.0f, -2.2247448f, 1.0f, -2.2247448f, 0.0f,
			-1.17215133f, 0.0f, -3.08626652f, 0.0f, -3.08626652f, 0.0f, -1.17215133f, 0.0f,
			-2.2247448f, -1.0f, 2.2247448f, 0.0f, -2.2247448f, 1.0f, 2.2247448f, 0.0f,
			-3.08626652f, 0.0f, 1.17215133f, 0.0f, -1.17215133f, 0.0f, 3.08626652f, 0.0f,
			-1.0f, 2.2247448f, -2.2247448f, 0.0f, 1.0f, 2.2247448f, -2.2247448f, 0.0f,
			0.0f, 1.17215133f, -3.08626652f, 0.0f, 0.0f, 3.08626652f, -1.17215133f, 0.0f,
			-1.0f, 2.2247448f, 2.2247448f, 0.0f, 1.0f, 2.2247448f, 2.2247448f, 0.0f,
			0.0f, 3.08626652f, 1.17215133f, 0.0f, 0.0f, 1.17215133f, 3.08626652f, 0.0f,
			2.2247448f, -2.2247448f, -1.0f, 0.0f, 2.2247448f, -2.2247448f, 1.0f, 0.0f,
			1.17215133f, -3.08626652f, 0.0f, 0.0f, 3.08626652f, -1.17215133f, 0.0f, 0.0f,
			2.2247448f, -1.0f, -2.2247448f, 0.0f, 2.2247448f, 1.0f, -2.2247448f, 0.0f,
			3.08626652f, 0.0f, -1.17215133f, 0.0f, 1.17215133f, 0.0f, -3.08626652f, 0.0f,
			2.2247448f, -1.0f, 2.2247448f, 0.0f, 2.2247448f, 1.0f, 2.2247448f, 0.0f,
			1.17215133f, 0.0f, 3.08626652f, 0.0f, 3.08626652f, 0.0f, 1.17215133f, 0.0f
		};
		
		for (size_t i = 0; i < sizeof(gradients3DBase) / sizeof(float); ++i) {
			gradients3DBase[i] = gradients3DBase[i] / 0.07969838f;
		}
		
		int index3 = 0;
		int index4 = 0;
		while (index3 < N_GRADS_3D * 3) {
			if (index4 == sizeof(gradients3DBase) / sizeof(float)) {
				index4 = 0;
			}
			GRADIENTS_3D[index3] = gradients3DBase[index4];
			++index3;
			++index4;
		}
		
		// Инициализация GRADIENTS_4D (полный массив из 7DTD)
		float gradients4DBase[] = {
			-0.6740059f, -0.323984772f, -0.323984772f, 0.5794685f,
			-0.7504884f, -0.400467217f, 0.15296486f, 0.502986f,
			-0.7504884f, 0.15296486f, -0.400467217f, 0.502986f,
			-0.8828162f, 0.08164729f, 0.08164729f, 0.4553054f,
			-0.4553054f, -0.08164729f, -0.08164729f, 0.8828162f,
			-0.502986f, -0.15296486f, 0.400467217f, 0.7504884f,
			-0.502986f, 0.400467217f, -0.15296486f, 0.7504884f,
			-0.5794685f, 0.323984772f, 0.323984772f, 0.6740059f,
			-0.6740059f, -0.323984772f, 0.5794685f, -0.323984772f,
			-0.7504884f, -0.400467217f, 0.502986f, 0.15296486f,
			-0.7504884f, 0.15296486f, 0.502986f, -0.400467217f,
			-0.8828162f, 0.08164729f, 0.4553054f, 0.08164729f,
			-0.4553054f, -0.08164729f, 0.8828162f, -0.08164729f,
			-0.502986f, -0.15296486f, 0.7504884f, 0.400467217f,
			-0.502986f, 0.400467217f, 0.7504884f, -0.15296486f,
			-0.5794685f, 0.323984772f, 0.6740059f, 0.323984772f,
			-0.6740059f, 0.5794685f, -0.323984772f, -0.323984772f,
			-0.7504884f, 0.502986f, -0.400467217f, 0.15296486f,
			-0.7504884f, 0.502986f, 0.15296486f, -0.400467217f,
			-0.8828162f, 0.4553054f, 0.08164729f, 0.08164729f,
			-0.4553054f, 0.8828162f, -0.08164729f, -0.08164729f,
			-0.502986f, 0.7504884f, -0.15296486f, 0.400467217f,
			-0.502986f, 0.7504884f, 0.400467217f, -0.15296486f,
			-0.5794685f, 0.6740059f, 0.323984772f, 0.323984772f,
			0.5794685f, -0.6740059f, -0.323984772f, -0.323984772f,
			0.502986f, -0.7504884f, -0.400467217f, 0.15296486f,
			0.502986f, -0.7504884f, 0.15296486f, -0.400467217f,
			0.4553054f, -0.8828162f, 0.08164729f, 0.08164729f,
			0.8828162f, -0.4553054f, -0.08164729f, -0.08164729f,
			0.7504884f, -0.502986f, -0.15296486f, 0.400467217f,
			0.7504884f, -0.502986f, 0.400467217f, -0.15296486f,
			0.6740059f, -0.5794685f, 0.323984772f, 0.323984772f,
			-0.753341f, -0.3796829f, -0.3796829f, -0.3796829f,
			-0.782168448f, -0.432147264f, -0.432147264f, 0.121284805f,
			-0.782168448f, -0.432147264f, 0.121284805f, -0.432147264f,
			-0.782168448f, 0.121284805f, -0.432147264f, -0.432147264f,
			-0.858650863f, -0.5086297f, 0.04480237f, 0.04480237f,
			-0.858650863f, 0.04480237f, -0.5086297f, 0.04480237f,
			-0.858650863f, 0.04480237f, 0.04480237f, -0.5086297f,
			-0.9982829f, -0.0338194147f, -0.0338194147f, -0.0338194147f,
			-0.3796829f, -0.753341f, -0.3796829f, -0.3796829f,
			-0.432147264f, -0.782168448f, -0.432147264f, 0.121284805f,
			-0.432147264f, -0.782168448f, 0.121284805f, -0.432147264f,
			0.121284805f, -0.782168448f, -0.432147264f, -0.432147264f,
			-0.5086297f, -0.858650863f, 0.04480237f, 0.04480237f,
			0.04480237f, -0.858650863f, -0.5086297f, 0.04480237f,
			0.04480237f, -0.858650863f, 0.04480237f, -0.5086297f,
			-0.0338194147f, -0.9982829f, -0.0338194147f, -0.0338194147f,
			-0.3796829f, -0.3796829f, -0.753341f, -0.3796829f,
			-0.432147264f, -0.432147264f, -0.782168448f, 0.121284805f,
			-0.432147264f, 0.121284805f, -0.782168448f, -0.432147264f,
			0.121284805f, -0.432147264f, -0.782168448f, -0.432147264f,
			-0.5086297f, 0.04480237f, -0.858650863f, 0.04480237f,
			0.04480237f, -0.5086297f, -0.858650863f, 0.04480237f,
			0.04480237f, 0.04480237f, -0.858650863f, -0.5086297f,
			-0.0338194147f, -0.0338194147f, -0.9982829f, -0.0338194147f,
			-0.3796829f, -0.3796829f, -0.3796829f, -0.753341f,
			-0.432147264f, -0.432147264f, 0.121284805f, -0.782168448f,
			-0.432147264f, 0.121284805f, -0.432147264f, -0.782168448f,
			0.121284805f, -0.432147264f, -0.432147264f, -0.782168448f,
			-0.5086297f, 0.04480237f, 0.04480237f, -0.858650863f,
			0.04480237f, -0.5086297f, 0.04480237f, -0.858650863f,
			0.04480237f, 0.04480237f, -0.5086297f, -0.858650863f,
			-0.0338194147f, -0.0338194147f, -0.0338194147f, -0.9982829f,
			-0.323984772f, -0.6740059f, -0.323984772f, 0.5794685f,
			-0.400467217f, -0.7504884f, 0.15296486f, 0.502986f,
			0.15296486f, -0.7504884f, -0.400467217f, 0.502986f,
			0.08164729f, -0.8828162f, 0.08164729f, 0.4553054f,
			-0.08164729f, -0.4553054f, -0.08164729f, 0.8828162f,
			-0.15296486f, -0.502986f, 0.400467217f, 0.7504884f,
			0.400467217f, -0.502986f, -0.15296486f, 0.7504884f,
			0.323984772f, -0.5794685f, 0.323984772f, 0.6740059f,
			-0.323984772f, -0.323984772f, -0.6740059f, 0.5794685f,
			-0.400467217f, 0.15296486f, -0.7504884f, 0.502986f,
			0.15296486f, -0.400467217f, -0.7504884f, 0.502986f,
			0.08164729f, 0.08164729f, -0.8828162f, 0.4553054f,
			-0.08164729f, -0.08164729f, -0.4553054f, 0.8828162f,
			-0.15296486f, 0.400467217f, -0.502986f, 0.7504884f,
			0.400467217f, -0.15296486f, -0.502986f, 0.7504884f,
			0.323984772f, 0.323984772f, -0.5794685f, 0.6740059f,
			-0.323984772f, -0.6740059f, 0.5794685f, -0.323984772f,
			-0.400467217f, -0.7504884f, 0.502986f, 0.15296486f,
			0.15296486f, -0.7504884f, 0.502986f, -0.400467217f,
			0.08164729f, -0.8828162f, 0.4553054f, 0.08164729f,
			-0.08164729f, -0.4553054f, 0.8828162f, -0.08164729f,
			-0.15296486f, -0.502986f, 0.7504884f, 0.400467217f,
			0.400467217f, -0.502986f, 0.7504884f, -0.15296486f,
			0.323984772f, -0.5794685f, 0.6740059f, 0.323984772f,
			-0.323984772f, -0.323984772f, 0.5794685f, -0.6740059f,
			-0.400467217f, 0.15296486f, 0.502986f, -0.7504884f,
			0.15296486f, -0.400467217f, 0.502986f, -0.7504884f,
			0.08164729f, 0.08164729f, 0.4553054f, -0.8828162f,
			-0.08164729f, -0.08164729f, 0.8828162f, -0.4553054f,
			-0.15296486f, 0.400467217f, 0.7504884f, -0.502986f,
			0.400467217f, -0.15296486f, 0.7504884f, -0.502986f,
			0.323984772f, 0.323984772f, 0.6740059f, -0.5794685f,
			-0.323984772f, 0.5794685f, -0.6740059f, -0.323984772f,
			-0.400467217f, 0.502986f, -0.7504884f, 0.15296486f,
			0.15296486f, 0.502986f, -0.7504884f, -0.400467217f,
			0.08164729f, 0.4553054f, -0.8828162f, 0.08164729f,
			-0.08164729f, 0.8828162f, -0.4553054f, -0.08164729f,
			-0.15296486f, 0.7504884f, -0.502986f, 0.400467217f,
			0.400467217f, 0.7504884f, -0.502986f, -0.15296486f,
			0.323984772f, 0.6740059f, -0.5794685f, 0.323984772f,
			-0.323984772f, 0.5794685f, -0.323984772f, -0.6740059f,
			-0.400467217f, 0.502986f, 0.15296486f, -0.7504884f,
			0.15296486f, 0.502986f, -0.400467217f, -0.7504884f,
			0.08164729f, 0.4553054f, 0.08164729f, -0.8828162f,
			-0.08164729f, 0.8828162f, -0.08164729f, -0.4553054f,
			-0.15296486f, 0.7504884f, 0.400467217f, -0.502986f,
			0.400467217f, 0.7504884f, -0.15296486f, -0.502986f,
			0.323984772f, 0.6740059f, 0.323984772f, -0.5794685f,
			0.5794685f, -0.323984772f, -0.6740059f, -0.323984772f,
			0.502986f, -0.400467217f, -0.7504884f, 0.15296486f,
			0.502986f, 0.15296486f, -0.7504884f, -0.400467217f,
			0.4553054f, 0.08164729f, -0.8828162f, 0.08164729f,
			0.8828162f, -0.08164729f, -0.4553054f, -0.08164729f,
			0.7504884f, -0.15296486f, -0.502986f, 0.400467217f,
			0.7504884f, 0.400467217f, -0.502986f, -0.15296486f,
			0.6740059f, 0.323984772f, -0.5794685f, 0.323984772f,
			0.5794685f, -0.323984772f, -0.323984772f, -0.6740059f,
			0.502986f, -0.400467217f, 0.15296486f, -0.7504884f,
			0.502986f, 0.15296486f, -0.400467217f, -0.7504884f,
			0.4553054f, 0.08164729f, 0.08164729f, -0.8828162f,
			0.8828162f, -0.08164729f, -0.08164729f, -0.4553054f,
			0.7504884f, -0.15296486f, 0.400467217f, -0.502986f,
			0.7504884f, 0.400467217f, -0.15296486f, -0.502986f,
			0.6740059f, 0.323984772f, 0.323984772f, -0.5794685f,
			0.0338194147f, 0.0338194147f, 0.0338194147f, 0.9982829f,
			-0.04480237f, -0.04480237f, 0.5086297f, 0.858650863f,
			-0.04480237f, 0.5086297f, -0.04480237f, 0.858650863f,
			-0.121284805f, 0.432147264f, 0.432147264f, 0.782168448f,
			0.5086297f, -0.04480237f, -0.04480237f, 0.858650863f,
			0.432147264f, -0.121284805f, 0.432147264f, 0.782168448f,
			0.432147264f, 0.432147264f, -0.121284805f, 0.782168448f,
			0.3796829f, 0.3796829f, 0.3796829f, 0.753341f,
			0.0338194147f, 0.0338194147f, 0.9982829f, 0.0338194147f,
			-0.04480237f, 0.04480237f, 0.858650863f, 0.5086297f,
			-0.04480237f, 0.5086297f, 0.858650863f, -0.04480237f,
			-0.121284805f, 0.432147264f, 0.782168448f, 0.432147264f,
			0.5086297f, -0.04480237f, 0.858650863f, -0.04480237f,
			0.432147264f, -0.121284805f, 0.782168448f, 0.432147264f,
			0.432147264f, 0.432147264f, 0.782168448f, -0.121284805f,
			0.3796829f, 0.3796829f, 0.753341f, 0.3796829f,
			0.0338194147f, 0.9982829f, 0.0338194147f, 0.0338194147f,
			-0.04480237f, 0.858650863f, -0.04480237f, 0.5086297f,
			-0.04480237f, 0.858650863f, 0.5086297f, -0.04480237f,
			-0.121284805f, 0.782168448f, 0.432147264f, 0.432147264f,
			0.5086297f, 0.858650863f, -0.04480237f, -0.04480237f,
			0.432147264f, 0.782168448f, -0.121284805f, 0.432147264f,
			0.432147264f, 0.782168448f, 0.432147264f, -0.121284805f,
			0.3796829f, 0.753341f, 0.3796829f, 0.3796829f,
			0.9982829f, 0.0338194147f, 0.0338194147f, 0.0338194147f,
			0.858650863f, -0.04480237f, -0.04480237f, 0.5086297f,
			0.858650863f, -0.04480237f, 0.5086297f, -0.04480237f,
			0.782168448f, -0.121284805f, 0.432147264f, 0.432147264f,
			0.858650863f, 0.5086297f, -0.04480237f, -0.04480237f,
			0.782168448f, 0.432147264f, -0.121284805f, 0.432147264f,
			0.782168448f, 0.432147264f, 0.432147264f, -0.121284805f,
			0.753341f, 0.3796829f, 0.3796829f, 0.3796829f
		};
		
		for (size_t i = 0; i < sizeof(gradients4DBase) / sizeof(float); ++i) {
			gradients4DBase[i] = gradients4DBase[i] / 0.0220065936f;
		}
		
		int index5 = 0;
		int index6 = 0;
		while (index5 < N_GRADS_4D * 4) {
			if (index6 == sizeof(gradients4DBase) / sizeof(float)) {
				index6 = 0;
			}
			GRADIENTS_4D[index5] = gradients4DBase[index6];
			++index5;
			++index6;
		}
		
		initialized = true;
	}
	
	// 2D шум
	float Noise2(int64_t seed, double x, double y) {
		Initialize();
		double num = SKEW_2D * (x + y);
		double xs = x + num;
		double ys = y + num;
		return Noise2_UnskewedBase(seed, xs, ys);
	}
	
	float Noise2_ImproveX(int64_t seed, double x, double y) {
		Initialize();
		double num1 = x * ROOT2OVER2;
		double num2 = y * 1.2247448713915896;
		return Noise2_UnskewedBase(seed, num2 + num1, num2 - num1);
	}
	
	static float Noise2_UnskewedBase(int64_t seed, double xs, double ys) {
		int num1 = FastFloor(xs);
		int num2 = FastFloor(ys);
		double num3 = static_cast<double>(static_cast<float>(xs) - static_cast<float>(num1));
		float num4 = static_cast<float>(ys) - static_cast<float>(num2);
		int64_t xsvp = static_cast<int64_t>(num1) * PRIME_X;
		int64_t ysvp = static_cast<int64_t>(num2) * PRIME_Y;
		float num5 = static_cast<float>((num3 + static_cast<double>(num4)) * UNSKEW_2D);
		float dx1 = static_cast<float>(num3) + num5;
		float dy1 = num4 + num5;
		float num6 = 0.0f;
		float num7 = static_cast<float>(0.5 - static_cast<double>(dx1) * static_cast<double>(dx1) - static_cast<double>(dy1) * static_cast<double>(dy1));
		if (static_cast<double>(num7) > 0.0) {
			num6 = static_cast<float>(static_cast<double>(num7) * static_cast<double>(num7) * (static_cast<double>(num7) * static_cast<double>(num7))) * Grad(seed, xsvp, ysvp, dx1, dy1);
		}
		float num8 = static_cast<float>(-3.1547005176544189 * static_cast<double>(num5) + (static_cast<double>(num7) - 0.66666668653488159));
		if (static_cast<double>(num8) > 0.0) {
			float dx2 = dx1 - 0.577350259f;
			float dy2 = dy1 - 0.577350259f;
			num6 += static_cast<float>(static_cast<double>(num8) * static_cast<double>(num8) * (static_cast<double>(num8) * static_cast<double>(num8))) * Grad(seed, xsvp + PRIME_X, ysvp + PRIME_Y, dx2, dy2);
		}
		if (static_cast<double>(dy1) > static_cast<double>(dx1)) {
			float dx3 = dx1 - -0.211324871f;
			float dy3 = dy1 - 0.7886751f;
			float num9 = static_cast<float>(0.5 - static_cast<double>(dx3) * static_cast<double>(dx3) - static_cast<double>(dy3) * static_cast<double>(dy3));
			if (static_cast<double>(num9) > 0.0) {
				num6 += static_cast<float>(static_cast<double>(num9) * static_cast<double>(num9) * (static_cast<double>(num9) * static_cast<double>(num9))) * Grad(seed, xsvp, ysvp + PRIME_Y, dx3, dy3);
			}
		} else {
			float dx4 = dx1 - 0.7886751f;
			float dy4 = dy1 - -0.211324871f;
			float num10 = static_cast<float>(0.5 - static_cast<double>(dx4) * static_cast<double>(dx4) - static_cast<double>(dy4) * static_cast<double>(dy4));
			if (static_cast<double>(num10) > 0.0) {
				num6 += static_cast<float>(static_cast<double>(num10) * static_cast<double>(num10) * (static_cast<double>(num10) * static_cast<double>(num10))) * Grad(seed, xsvp + PRIME_X, ysvp, dx4, dy4);
			}
		}
		return num6;
	}
	
	// 3D шум
	float Noise3_ImproveXY(int64_t seed, double x, double y, double z) {
		Initialize();
		double num1 = x + y;
		double num2 = num1 * ROTATE_3D_ORTHOGONALIZER;
		double num3 = z * ROOT3OVER3;
		double xr = x + num2 + num3;
		double yr = y + num2 + num3;
		double zr = num1 * -ROOT3OVER3 + num3;
		return Noise3_UnrotatedBase(seed, xr, yr, zr);
	}
	
	float Noise3_ImproveXZ(int64_t seed, double x, double y, double z) {
		Initialize();
		double num1 = x + z;
		double num2 = num1 * ROTATE_3D_ORTHOGONALIZER;
		double num3 = y * ROOT3OVER3;
		double xr = x + num2 + num3;
		double zr = z + num2 + num3;
		double yr = num1 * -ROOT3OVER3 + num3;
		return Noise3_UnrotatedBase(seed, xr, yr, zr);
	}
	
	float Noise3_Fallback(int64_t seed, double x, double y, double z) {
		Initialize();
		double num = FALLBACK_ROTATE_3D * (x + y + z);
		double xr = num - x;
		double yr = num - y;
		double zr = num - z;
		return Noise3_UnrotatedBase(seed, xr, yr, zr);
	}
	
	static float Noise3_UnrotatedBase(int64_t seed, double xr, double yr, double zr) {
		int num1 = FastRound(xr);
		int num2 = FastRound(yr);
		int num3 = FastRound(zr);
		float dx = static_cast<float>(xr) - static_cast<float>(num1);
		float dy = static_cast<float>(yr) - static_cast<float>(num2);
		float dz = static_cast<float>(zr) - static_cast<float>(num3);
		int num4 = static_cast<int>(-1.0 - static_cast<double>(dx)) | 1;
		int num5 = static_cast<int>(-1.0 - static_cast<double>(dy)) | 1;
		int num6 = static_cast<int>(-1.0 - static_cast<double>(dz)) | 1;
		float num7 = static_cast<float>(num4) * -dx;
		float num8 = static_cast<float>(num5) * -dy;
		float num9 = static_cast<float>(num6) * -dz;
		int64_t xrvp = static_cast<int64_t>(num1) * PRIME_X;
		int64_t yrvp = static_cast<int64_t>(num2) * PRIME_Y;
		int64_t zrvp = static_cast<int64_t>(num3) * PRIME_Z;
		float num10 = 0.0f;
		float num11 = static_cast<float>(0.60000002384185791 - static_cast<double>(dx) * static_cast<double>(dx) - (static_cast<double>(dy) * static_cast<double>(dy) + static_cast<double>(dz) * static_cast<double>(dz)));
		int num12 = 0;
		while (true) {
			if (static_cast<double>(num11) > 0.0) {
				num10 += static_cast<float>(static_cast<double>(num11) * static_cast<double>(num11) * (static_cast<double>(num11) * static_cast<double>(num11))) * Grad(seed, xrvp, yrvp, zrvp, dx, dy, dz);
			}
			if (static_cast<double>(num7) >= static_cast<double>(num8) && static_cast<double>(num7) >= static_cast<double>(num9)) {
				float num13 = num11 + num7 + num7;
				if (static_cast<double>(num13) > 1.0) {
					float num14 = num13 - 1.0f;
					num10 += static_cast<float>(static_cast<double>(num14) * static_cast<double>(num14) * (static_cast<double>(num14) * static_cast<double>(num14))) * Grad(seed, xrvp - static_cast<int64_t>(num4) * PRIME_X, yrvp, zrvp, dx + static_cast<float>(num4), dy, dz);
				}
			} else if (static_cast<double>(num8) > static_cast<double>(num7) && static_cast<double>(num8) >= static_cast<double>(num9)) {
				float num15 = num11 + num8 + num8;
				if (static_cast<double>(num15) > 1.0) {
					float num16 = num15 - 1.0f;
					num10 += static_cast<float>(static_cast<double>(num16) * static_cast<double>(num16) * (static_cast<double>(num16) * static_cast<double>(num16))) * Grad(seed, xrvp, yrvp - static_cast<int64_t>(num5) * PRIME_Y, zrvp, dx, dy + static_cast<float>(num5), dz);
				}
			} else {
				float num17 = num11 + num9 + num9;
				if (static_cast<double>(num17) > 1.0) {
					float num18 = num17 - 1.0f;
					num10 += static_cast<float>(static_cast<double>(num18) * static_cast<double>(num18) * (static_cast<double>(num18) * static_cast<double>(num18))) * Grad(seed, xrvp, yrvp, zrvp - static_cast<int64_t>(num6) * PRIME_Z, dx, dy, dz + static_cast<float>(num6));
				}
			}
			if (num12 != 1) {
				num7 = 0.5f - num7;
				num8 = 0.5f - num8;
				num9 = 0.5f - num9;
				dx = static_cast<float>(num4) * num7;
				dy = static_cast<float>(num5) * num8;
				dz = static_cast<float>(num6) * num9;
				num11 += static_cast<float>(0.75 - static_cast<double>(num7) - (static_cast<double>(num8) + static_cast<double>(num9)));
				xrvp += (static_cast<int64_t>(num4 >> 1) & PRIME_X);
				yrvp += (static_cast<int64_t>(num5 >> 1) & PRIME_Y);
				zrvp += (static_cast<int64_t>(num6 >> 1) & PRIME_Z);
				num4 = -num4;
				num5 = -num5;
				num6 = -num6;
				seed ^= SEED_FLIP_3D;
				++num12;
			} else {
				break;
			}
		}
		return num10;
	}
	
	float Noise4_ImproveXYZ_ImproveXY(int64_t seed, double x, double y, double z, double w) {
		Initialize();
		double num1 = x + y;
		double num2 = num1 * ROTATE_3D_ORTHOGONALIZER;
		double num3 = z * 0.28867513459481292;
		double num4 = w * 0.2236067977499788;
		double xs = x + (num3 + num4 + num2);
		double ys = y + (num3 + num4 + num2);
		double zs = num1 * -ROOT3OVER3 + (num3 + num4);
		double ws = z * -0.866025403784439 + num4;
		return Noise4_UnskewedBase(seed, xs, ys, zs, ws);
	}
	
	float Noise4_ImproveXYZ_ImproveXZ(int64_t seed, double x, double y, double z, double w) {
		Initialize();
		double num1 = x + z;
		double num2 = num1 * ROTATE_3D_ORTHOGONALIZER;
		double num3 = y * 0.28867513459481292;
		double num4 = w * 0.2236067977499788;
		double xs = x + (num3 + num4 + num2);
		double zs = z + (num3 + num4 + num2);
		double ys = num1 * -ROOT3OVER3 + (num3 + num4);
		double ws = y * -0.866025403784439 + num4;
		return Noise4_UnskewedBase(seed, xs, ys, zs, ws);
	}
	
	float Noise4_ImproveXYZ(int64_t seed, double x, double y, double z, double w) {
		Initialize();
		double num1 = x + y + z;
		double num2 = w * 0.2236067977499788;
		double num3 = num1 * (-1.0 / 6.0) + num2;
		double xs = x + num3;
		double ys = y + num3;
		double zs = z + num3;
		double ws = -0.5 * num1 + num2;
		return Noise4_UnskewedBase(seed, xs, ys, zs, ws);
	}
	
	float Noise4_Fallback(int64_t seed, double x, double y, double z, double w) {
		Initialize();
		double num = SKEW_4D * (x + y + z + w);
		double xs = x + num;
		double ys = y + num;
		double zs = z + num;
		double ws = w + num;
		return Noise4_UnskewedBase(seed, xs, ys, zs, ws);
	}
	
	static float Noise4_UnskewedBase(int64_t seed, double xs, double ys, double zs, double ws) {
		int num1 = FastFloor(xs);
		int num2 = FastFloor(ys);
		int num3 = FastFloor(zs);
		int num4 = FastFloor(ws);
		float num5 = static_cast<float>(xs) - static_cast<float>(num1);
		float num6 = static_cast<float>(ys) - static_cast<float>(num2);
		float num7 = static_cast<float>(zs) - static_cast<float>(num3);
		float num8 = static_cast<float>(ws) - static_cast<float>(num4);
		double num9;
		int num10 = static_cast<int>((num9 = static_cast<double>(num5) + static_cast<double>(num6) + (static_cast<double>(num7) + static_cast<double>(num8))) * 1.25);
		seed += static_cast<int64_t>(num10) * SEED_OFFSET_4D;
		float num11 = static_cast<float>(num10) * -LATTICE_STEP_4D;
		float num12 = num5 + num11;
		float num13 = num6 + num11;
		float num14 = num7 + num11;
		float num15 = num8 + num11;
		double num16 = static_cast<double>(num11) * 4.0;
		float num17 = static_cast<float>((num9 + num16) * UNSKEW_4D);
		int64_t xsvp = static_cast<int64_t>(num1) * PRIME_X;
		int64_t ysvp = static_cast<int64_t>(num2) * PRIME_Y;
		int64_t zsvp = static_cast<int64_t>(num3) * PRIME_Z;
		int64_t wsvp = static_cast<int64_t>(num4) * PRIME_W;
		float num18 = 0.0f;
		int num19 = 0;
		while (true) {
			double num20 = 1.0 + static_cast<double>(num17) * -3.2360678915486614;
			if (static_cast<double>(num12) >= static_cast<double>(num13) && static_cast<double>(num12) >= static_cast<double>(num14) && static_cast<double>(num12) >= static_cast<double>(num15) && static_cast<double>(num12) >= num20) {
				xsvp += PRIME_X;
				--num12;
				num17 -= UNSKEW_4D;
			} else if (static_cast<double>(num13) > static_cast<double>(num12) && static_cast<double>(num13) >= static_cast<double>(num14) && static_cast<double>(num13) >= static_cast<double>(num15) && static_cast<double>(num13) >= num20) {
				ysvp += PRIME_Y;
				--num13;
				num17 -= UNSKEW_4D;
			} else if (static_cast<double>(num14) > static_cast<double>(num12) && static_cast<double>(num14) > static_cast<double>(num13) && static_cast<double>(num14) >= static_cast<double>(num15) && static_cast<double>(num14) >= num20) {
				zsvp += PRIME_Z;
				--num14;
				num17 -= UNSKEW_4D;
			} else if (static_cast<double>(num15) > static_cast<double>(num12) && static_cast<double>(num15) > static_cast<double>(num13) && static_cast<double>(num15) > static_cast<double>(num14) && static_cast<double>(num15) >= num20) {
				wsvp += PRIME_W;
				--num15;
				num17 -= UNSKEW_4D;
			}
			float dx = num12 + num17;
			float dy = num13 + num17;
			float dz = num14 + num17;
			float dw = num15 + num17;
			float num21 = static_cast<float>(static_cast<double>(dx) * static_cast<double>(dx) + static_cast<double>(dy) * static_cast<double>(dy) + (static_cast<double>(dz) * static_cast<double>(dz) + static_cast<double>(dw) * static_cast<double>(dw)));
			if (static_cast<double>(num21) < 0.60000002384185791) {
				float num22 = num21 - RSQUARED_4D;
				float num23 = num22 * num22;
				num18 += num23 * num23 * Grad(seed, xsvp, ysvp, zsvp, wsvp, dx, dy, dz, dw);
			}
			if (num19 != 4) {
				num12 += LATTICE_STEP_4D;
				num13 += LATTICE_STEP_4D;
				num14 += LATTICE_STEP_4D;
				num15 += LATTICE_STEP_4D;
				num17 += 0.2472136f;
				seed -= SEED_OFFSET_4D;
				if (num19 == num10) {
					xsvp -= PRIME_X;
					ysvp -= PRIME_Y;
					zsvp -= PRIME_Z;
					wsvp -= PRIME_W;
					seed += 5229608487776120705LL;
				}
				++num19;
			} else {
				break;
			}
		}
		return num18;
	}
}

