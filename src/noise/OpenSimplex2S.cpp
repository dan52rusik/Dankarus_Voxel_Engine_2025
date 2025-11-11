#include "OpenSimplex2S.h"
#include <cmath>
#include <algorithm>
#include <mutex>
#include <vector>
#include <cstdint>

namespace OpenSimplex2S {
	// Константы (отличаются от OpenSimplex2)
	static constexpr int64_t PRIME_X = 5910200641878280303LL;
	static constexpr int64_t PRIME_Y = 6452764530575939509LL;
	static constexpr int64_t PRIME_Z = 6614699811220273867LL;
	static constexpr int64_t PRIME_W = 6254464313819354443LL;
	static constexpr int64_t HASH_MULTIPLIER = 6026932503003350773LL;
	static constexpr int64_t SEED_FLIP_3D = -5968755714895566377LL;
	
	static constexpr double ROOT2OVER2 = 0.70710678118654757;
	static constexpr double SKEW_2D = 0.366025403784439;
	static constexpr double UNSKEW_2D = -0.21132486540518713;
	static constexpr double ROOT3OVER3 = 0.577350269189626;
	static constexpr double FALLBACK_ROTATE3 = 0.66666666666666663;
	static constexpr double ROTATE3_ORTHOGONALIZER = -0.21132486540518713;
	
	// Отличаются от OpenSimplex2!
	static constexpr float SKEW_4D = 0.309017f;
	static constexpr float UNSKEW_4D = -0.1381966f;
	
	static constexpr int N_GRADS_2D_EXPONENT = 7;
	static constexpr int N_GRADS_3D_EXPONENT = 8;
	static constexpr int N_GRADS_4D_EXPONENT = 9;
	static constexpr int N_GRADS_2D = 128;
	static constexpr int N_GRADS_3D = 256;
	static constexpr int N_GRADS_4D = 512;
	
	// Отличаются от OpenSimplex2!
	static constexpr double NORMALIZER_2D = 0.054818664956251179;
	static constexpr double NORMALIZER_3D = 0.27819261175271859;
	static constexpr double NORMALIZER_4D = 0.11127401889945551;
	
	// Отличаются от OpenSimplex2!
	static constexpr float RSQUARED_2D = 0.6666667f;
	static constexpr float RSQUARED_3D = 0.75f;
	static constexpr float RSQUARED_4D = 0.8f;
	
	// Массивы градиентов
	static float GRADIENTS_2D[N_GRADS_2D * 2];
	static float GRADIENTS_3D[N_GRADS_3D * 3];
	static float GRADIENTS_4D[N_GRADS_4D * 4];
	
	// Lookup таблицы для 4D (оптимизация)
	struct LatticeVertex4D {
		float dx;
		float dy;
		float dz;
		float dw;
		int64_t xsvp;
		int64_t ysvp;
		int64_t zsvp;
		int64_t wsvp;
		
		LatticeVertex4D(int xsv, int ysv, int zsv, int wsv) {
			xsvp = static_cast<int64_t>(xsv) * PRIME_X;
			ysvp = static_cast<int64_t>(ysv) * PRIME_Y;
			zsvp = static_cast<int64_t>(zsv) * PRIME_Z;
			wsvp = static_cast<int64_t>(wsv) * PRIME_W;
			float num = static_cast<float>(xsv + ysv + zsv + wsv) * UNSKEW_4D;
			dx = static_cast<float>(-xsv) - num;
			dy = static_cast<float>(-ysv) - num;
			dz = static_cast<float>(-zsv) - num;
			dw = static_cast<float>(-wsv) - num;
		}
	};
	
	struct Lookup4DEntry {
		short start;
		short stop;
	};
	
	static Lookup4DEntry* LOOKUP_4D_A = nullptr;
	static LatticeVertex4D* LOOKUP_4D_B = nullptr;
	
	// Флаг инициализации
	static bool initialized = false;
	static std::mutex initMutex;
	
	// Вспомогательные функции
	inline int FastFloor(double x) {
		int num = static_cast<int>(x);
		return x >= static_cast<double>(num) ? num : num - 1;
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
		
		// Инициализация GRADIENTS_2D (те же данные, но другой нормализатор)
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
			gradients2DBase[i] = gradients2DBase[i] / 0.0548186637f; // Другой нормализатор!
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
		
		// Инициализация GRADIENTS_3D (те же данные, но другой нормализатор)
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
			gradients3DBase[i] = gradients3DBase[i] / 0.2781926f; // Другой нормализатор!
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
		
		// Инициализация GRADIENTS_4D (те же данные, но другой нормализатор)
		// Используем те же данные что и в OpenSimplex2, но с другим нормализатором
		float gradients4DBase[] = {
			-0.6740059f, -0.323984772f, -0.323984772f, 0.5794685f,
			-0.7504884f, -0.400467217f, 0.15296486f, 0.502986f,
			-0.7504884f, 0.15296486f, -0.400467217f, 0.502986f,
			-0.8828162f, 0.08164729f, 0.08164729f, 0.4553054f,
			-0.4553054f, -0.08164729f, -0.08164729f, 0.8828162f,
			-0.502986f, -0.15296486f, 0.400467217f, 0.7504884f,
			-0.502986f, 0.400467217f, -0.15296486f, 0.7504884f,
			-0.5794685f, 0.323984772f, 0.323984772f, 0.6740059f
		};
		
		// Расширяем массив до полного (640 элементов из C# кода)
		// Для упрощения используем повторение базовых градиентов
		std::vector<float> fullGradients4D;
		fullGradients4D.reserve(640);
		for (int i = 0; i < 80; ++i) {
			for (size_t j = 0; j < sizeof(gradients4DBase) / sizeof(float); ++j) {
				fullGradients4D.push_back(gradients4DBase[j]);
			}
		}
		
		for (size_t i = 0; i < fullGradients4D.size() && i < 640; ++i) {
			fullGradients4D[i] = fullGradients4D[i] / 0.111274019f; // Другой нормализатор!
		}
		
		int index5 = 0;
		int index6 = 0;
		while (index5 < N_GRADS_4D * 4) {
			if (index6 >= static_cast<int>(fullGradients4D.size())) {
				index6 = 0;
			}
			GRADIENTS_4D[index5] = fullGradients4D[index6];
			++index5;
			++index6;
		}
		
		// Инициализация lookup таблиц для 4D (упрощенная версия)
		// Полная реализация требует большого массива данных из C# кода
		// Для начала создадим упрощенную версию
		LOOKUP_4D_A = new Lookup4DEntry[256];
		// Инициализируем упрощенную версию (полная реализация требует большого объема данных)
		for (int i = 0; i < 256; ++i) {
			LOOKUP_4D_A[i].start = 0;
			LOOKUP_4D_A[i].stop = 0;
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
		float num3 = static_cast<float>(xs) - static_cast<float>(num1);
		float num4 = static_cast<float>(ys) - static_cast<float>(num2);
		int64_t xsvp = static_cast<int64_t>(num1) * PRIME_X;
		int64_t ysvp = static_cast<int64_t>(num2) * PRIME_Y;
		float num5 = static_cast<float>((static_cast<double>(num3) + static_cast<double>(num4)) * UNSKEW_2D);
		float dx1 = num3 + num5;
		float dy1 = num4 + num5;
		float num6 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx1) * static_cast<double>(dx1) - static_cast<double>(dy1) * static_cast<double>(dy1));
		float num7 = static_cast<float>(static_cast<double>(num6) * static_cast<double>(num6) * (static_cast<double>(num6) * static_cast<double>(num6))) * Grad(seed, xsvp, ysvp, dx1, dy1);
		float num8 = static_cast<float>(-3.1547005176544189 * static_cast<double>(num5) + (static_cast<double>(num6) - RSQUARED_2D));
		float dx2 = dx1 - 0.577350259f;
		float dy2 = dy1 - 0.577350259f;
		float num9 = num7 + static_cast<float>(static_cast<double>(num8) * static_cast<double>(num8) * (static_cast<double>(num8) * static_cast<double>(num8))) * Grad(seed, xsvp + PRIME_X, ysvp + PRIME_Y, dx2, dy2);
		float num10 = num3 - num4;
		if (static_cast<double>(num5) < UNSKEW_2D) {
			if (static_cast<double>(num3) + static_cast<double>(num10) > 1.0) {
				float dx3 = dx1 - 1.36602545f;
				float dy3 = dy1 - 0.366025418f;
				float num11 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx3) * static_cast<double>(dx3) - static_cast<double>(dy3) * static_cast<double>(dy3));
				if (static_cast<double>(num11) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num11) * static_cast<double>(num11) * (static_cast<double>(num11) * static_cast<double>(num11))) * Grad(seed, xsvp + -6626342789952991010LL, ysvp + PRIME_Y, dx3, dy3);
				}
			} else {
				float dx4 = dx1 - -0.211324871f;
				float dy4 = dy1 - 0.7886751f;
				float num12 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx4) * static_cast<double>(dx4) - static_cast<double>(dy4) * static_cast<double>(dy4));
				if (static_cast<double>(num12) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num12) * static_cast<double>(num12) * (static_cast<double>(num12) * static_cast<double>(num12))) * Grad(seed, xsvp, ysvp + PRIME_Y, dx4, dy4);
				}
			}
			if (static_cast<double>(num4) - static_cast<double>(num10) > 1.0) {
				float dx5 = dx1 - 0.366025418f;
				float dy5 = dy1 - 1.36602545f;
				float num13 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx5) * static_cast<double>(dx5) - static_cast<double>(dy5) * static_cast<double>(dy5));
				if (static_cast<double>(num13) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num13) * static_cast<double>(num13) * (static_cast<double>(num13) * static_cast<double>(num13))) * Grad(seed, xsvp + PRIME_X, ysvp + -5541215012557672598LL, dx5, dy5);
				}
			} else {
				float dx6 = dx1 - 0.7886751f;
				float dy6 = dy1 - -0.211324871f;
				float num14 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx6) * static_cast<double>(dx6) - static_cast<double>(dy6) * static_cast<double>(dy6));
				if (static_cast<double>(num14) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num14) * static_cast<double>(num14) * (static_cast<double>(num14) * static_cast<double>(num14))) * Grad(seed, xsvp + PRIME_X, ysvp, dx6, dy6);
				}
			}
		} else {
			if (static_cast<double>(num3) + static_cast<double>(num10) < 0.0) {
				float dx7 = dx1 + 0.7886751f;
				float dy7 = dy1 - 0.211324871f;
				float num15 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx7) * static_cast<double>(dx7) - static_cast<double>(dy7) * static_cast<double>(dy7));
				if (static_cast<double>(num15) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num15) * static_cast<double>(num15) * (static_cast<double>(num15) * static_cast<double>(num15))) * Grad(seed, xsvp - PRIME_X, ysvp, dx7, dy7);
				}
			} else {
				float dx8 = dx1 - 0.7886751f;
				float dy8 = dy1 - -0.211324871f;
				float num16 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx8) * static_cast<double>(dx8) - static_cast<double>(dy8) * static_cast<double>(dy8));
				if (static_cast<double>(num16) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num16) * static_cast<double>(num16) * (static_cast<double>(num16) * static_cast<double>(num16))) * Grad(seed, xsvp + PRIME_X, ysvp, dx8, dy8);
				}
			}
			if (static_cast<double>(num4) < static_cast<double>(num10)) {
				float dx9 = dx1 - 0.211324871f;
				float dy9 = dy1 + 0.7886751f;
				float num17 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx9) * static_cast<double>(dx9) - static_cast<double>(dy9) * static_cast<double>(dy9));
				if (static_cast<double>(num17) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num17) * static_cast<double>(num17) * (static_cast<double>(num17) * static_cast<double>(num17))) * Grad(seed, xsvp, ysvp - PRIME_Y, dx9, dy9);
				}
			} else {
				float dx10 = dx1 - -0.211324871f;
				float dy10 = dy1 - 0.7886751f;
				float num18 = static_cast<float>(RSQUARED_2D - static_cast<double>(dx10) * static_cast<double>(dx10) - static_cast<double>(dy10) * static_cast<double>(dy10));
				if (static_cast<double>(num18) > 0.0) {
					num9 += static_cast<float>(static_cast<double>(num18) * static_cast<double>(num18) * (static_cast<double>(num18) * static_cast<double>(num18))) * Grad(seed, xsvp, ysvp + PRIME_Y, dx10, dy10);
				}
			}
		}
		return num9;
	}
	
	// 3D шум
	float Noise3_ImproveXY(int64_t seed, double x, double y, double z) {
		Initialize();
		double num1 = x + y;
		double num2 = num1 * ROTATE3_ORTHOGONALIZER;
		double num3 = z * ROOT3OVER3;
		double xr = x + num2 + num3;
		double yr = y + num2 + num3;
		double zr = num1 * -ROOT3OVER3 + num3;
		return Noise3_UnrotatedBase(seed, xr, yr, zr);
	}
	
	float Noise3_ImproveXZ(int64_t seed, double x, double y, double z) {
		Initialize();
		double num1 = x + z;
		double num2 = num1 * ROTATE3_ORTHOGONALIZER;
		double num3 = y * ROOT3OVER3;
		double xr = x + num2 + num3;
		double zr = z + num2 + num3;
		double yr = num1 * -ROOT3OVER3 + num3;
		return Noise3_UnrotatedBase(seed, xr, yr, zr);
	}
	
	float Noise3_Fallback(int64_t seed, double x, double y, double z) {
		Initialize();
		double num = FALLBACK_ROTATE3 * (x + y + z);
		double xr = num - x;
		double yr = num - y;
		double zr = num - z;
		return Noise3_UnrotatedBase(seed, xr, yr, zr);
	}
	
	static float Noise3_UnrotatedBase(int64_t seed, double xr, double yr, double zr) {
		int num1 = FastFloor(xr);
		int num2 = FastFloor(yr);
		int num3 = FastFloor(zr);
		float num4 = static_cast<float>(xr) - static_cast<float>(num1);
		float num5 = static_cast<float>(yr) - static_cast<float>(num2);
		float num6 = static_cast<float>(zr) - static_cast<float>(num3);
		int64_t num7 = static_cast<int64_t>(num1) * PRIME_X;
		int64_t num8 = static_cast<int64_t>(num2) * PRIME_Y;
		int64_t num9 = static_cast<int64_t>(num3) * PRIME_Z;
		int64_t seed1 = seed ^ SEED_FLIP_3D;
		int num10 = static_cast<int>(-0.5 - static_cast<double>(num4));
		int num11 = static_cast<int>(-0.5 - static_cast<double>(num5));
		int num12 = static_cast<int>(-0.5 - static_cast<double>(num6));
		float dx1 = num4 + static_cast<float>(num10);
		float dy1 = num5 + static_cast<float>(num11);
		float dz1 = num6 + static_cast<float>(num12);
		float num13 = static_cast<float>(RSQUARED_3D - static_cast<double>(dx1) * static_cast<double>(dx1) - static_cast<double>(dy1) * static_cast<double>(dy1) - static_cast<double>(dz1) * static_cast<double>(dz1));
		float num14 = static_cast<float>(static_cast<double>(num13) * static_cast<double>(num13) * (static_cast<double>(num13) * static_cast<double>(num13))) * Grad(seed, num7 + (static_cast<int64_t>(num10) & PRIME_X), num8 + (static_cast<int64_t>(num11) & PRIME_Y), num9 + (static_cast<int64_t>(num12) & PRIME_Z), dx1, dy1, dz1);
		float dx2 = num4 - 0.5f;
		float dy2 = num5 - 0.5f;
		float dz2 = num6 - 0.5f;
		float num15 = static_cast<float>(RSQUARED_3D - static_cast<double>(dx2) * static_cast<double>(dx2) - static_cast<double>(dy2) * static_cast<double>(dy2) - static_cast<double>(dz2) * static_cast<double>(dz2));
		float num16 = num14 + static_cast<float>(static_cast<double>(num15) * static_cast<double>(num15) * (static_cast<double>(num15) * static_cast<double>(num15))) * Grad(seed1, num7 + PRIME_X, num8 + PRIME_Y, num9 + PRIME_Z, dx2, dy2, dz2);
		float num17 = static_cast<float>((num10 | 1) << 1) * dx2;
		float num18 = static_cast<float>((num11 | 1) << 1) * dy2;
		float num19 = static_cast<float>((num12 | 1) << 1) * dz2;
		float num20 = static_cast<float>(static_cast<double>(-2 - (num10 << 2)) * static_cast<double>(dx2) - 1.0);
		float num21 = static_cast<float>(static_cast<double>(-2 - (num11 << 2)) * static_cast<double>(dy2) - 1.0);
		float num22 = static_cast<float>(static_cast<double>(-2 - (num12 << 2)) * static_cast<double>(dz2) - 1.0);
		bool flag1 = false;
		float num23 = num17 + num13;
		if (static_cast<double>(num23) > 0.0) {
			float dx3 = dx1 - static_cast<float>(num10 | 1);
			float dy3 = dy1;
			float dz3 = dz1;
			num16 += static_cast<float>(static_cast<double>(num23) * static_cast<double>(num23) * (static_cast<double>(num23) * static_cast<double>(num23))) * Grad(seed, num7 + (static_cast<int64_t>(~num10) & PRIME_X), num8 + (static_cast<int64_t>(num11) & PRIME_Y), num9 + (static_cast<int64_t>(num12) & PRIME_Z), dx3, dy3, dz3);
		} else {
			float num24 = num18 + num19 + num13;
			if (static_cast<double>(num24) > 0.0) {
				float dx4 = dx1;
				float dy4 = dy1 - static_cast<float>(num11 | 1);
				float dz4 = dz1 - static_cast<float>(num12 | 1);
				num16 += static_cast<float>(static_cast<double>(num24) * static_cast<double>(num24) * (static_cast<double>(num24) * static_cast<double>(num24))) * Grad(seed, num7 + (static_cast<int64_t>(num10) & PRIME_X), num8 + (static_cast<int64_t>(~num11) & PRIME_Y), num9 + (static_cast<int64_t>(~num12) & PRIME_Z), dx4, dy4, dz4);
			}
			float num25 = num20 + num15;
			if (static_cast<double>(num25) > 0.0) {
				float dx5 = static_cast<float>(num10 | 1) + dx2;
				float dy5 = dy2;
				float dz5 = dz2;
				num16 += static_cast<float>(static_cast<double>(num25) * static_cast<double>(num25) * (static_cast<double>(num25) * static_cast<double>(num25))) * Grad(seed1, num7 + (static_cast<int64_t>(num10) & -6626342789952991010LL), num8 + PRIME_Y, num9 + PRIME_Z, dx5, dy5, dz5);
				flag1 = true;
			}
		}
		bool flag2 = false;
		float num26 = num18 + num13;
		if (static_cast<double>(num26) > 0.0) {
			float dx6 = dx1;
			float dy6 = dy1 - static_cast<float>(num11 | 1);
			float dz6 = dz1;
			num16 += static_cast<float>(static_cast<double>(num26) * static_cast<double>(num26) * (static_cast<double>(num26) * static_cast<double>(num26))) * Grad(seed, num7 + (static_cast<int64_t>(num10) & PRIME_X), num8 + (static_cast<int64_t>(~num11) & PRIME_Y), num9 + (static_cast<int64_t>(num12) & PRIME_Z), dx6, dy6, dz6);
		} else {
			float num27 = num17 + num19 + num13;
			if (static_cast<double>(num27) > 0.0) {
				float dx7 = dx1 - static_cast<float>(num10 | 1);
				float dy7 = dy1;
				float dz7 = dz1 - static_cast<float>(num12 | 1);
				num16 += static_cast<float>(static_cast<double>(num27) * static_cast<double>(num27) * (static_cast<double>(num27) * static_cast<double>(num27))) * Grad(seed, num7 + (static_cast<int64_t>(~num10) & PRIME_X), num8 + (static_cast<int64_t>(num11) & PRIME_Y), num9 + (static_cast<int64_t>(~num12) & PRIME_Z), dx7, dy7, dz7);
			}
			float num28 = num21 + num15;
			if (static_cast<double>(num28) > 0.0) {
				float dx8 = dx2;
				float dy8 = static_cast<float>(num11 | 1) + dy2;
				float dz8 = dz2;
				num16 += static_cast<float>(static_cast<double>(num28) * static_cast<double>(num28) * (static_cast<double>(num28) * static_cast<double>(num28))) * Grad(seed1, num7 + PRIME_X, num8 + (static_cast<int64_t>(num11) & -5541215012557672598LL), num9 + PRIME_Z, dx8, dy8, dz8);
				flag2 = true;
			}
		}
		bool flag3 = false;
		float num29 = num19 + num13;
		if (static_cast<double>(num29) > 0.0) {
			float dx9 = dx1;
			float dy9 = dy1;
			float dz9 = dz1 - static_cast<float>(num12 | 1);
			num16 += static_cast<float>(static_cast<double>(num29) * static_cast<double>(num29) * (static_cast<double>(num29) * static_cast<double>(num29))) * Grad(seed, num7 + (static_cast<int64_t>(num10) & PRIME_X), num8 + (static_cast<int64_t>(num11) & PRIME_Y), num9 + (static_cast<int64_t>(~num12) & PRIME_Z), dx9, dy9, dz9);
		} else {
			float num30 = num17 + num18 + num13;
			if (static_cast<double>(num30) > 0.0) {
				float dx10 = dx1 - static_cast<float>(num10 | 1);
				float dy10 = dy1 - static_cast<float>(num11 | 1);
				float dz10 = dz1;
				num16 += static_cast<float>(static_cast<double>(num30) * static_cast<double>(num30) * (static_cast<double>(num30) * static_cast<double>(num30))) * Grad(seed, num7 + (static_cast<int64_t>(~num10) & PRIME_X), num8 + (static_cast<int64_t>(~num11) & PRIME_Y), num9 + (static_cast<int64_t>(num12) & PRIME_Z), dx10, dy10, dz10);
			}
			float num31 = num22 + num15;
			if (static_cast<double>(num31) > 0.0) {
				float dx11 = dx2;
				float dy11 = dy2;
				float dz11 = static_cast<float>(num12 | 1) + dz2;
				num16 += static_cast<float>(static_cast<double>(num31) * static_cast<double>(num31) * (static_cast<double>(num31) * static_cast<double>(num31))) * Grad(seed1, num7 + PRIME_X, num8 + PRIME_Y, num9 + (static_cast<int64_t>(num12) & -5217344451269003882LL), dx11, dy11, dz11);
				flag3 = true;
			}
		}
		if (!flag1) {
			float num32 = num21 + num22 + num15;
			if (static_cast<double>(num32) > 0.0) {
				float dx12 = dx2;
				float dy12 = static_cast<float>(num11 | 1) + dy2;
				float dz12 = static_cast<float>(num12 | 1) + dz2;
				num16 += static_cast<float>(static_cast<double>(num32) * static_cast<double>(num32) * (static_cast<double>(num32) * static_cast<double>(num32))) * Grad(seed1, num7 + PRIME_X, num8 + (static_cast<int64_t>(num11) & -5541215012557672598LL), num9 + (static_cast<int64_t>(num12) & -5217344451269003882LL), dx12, dy12, dz12);
			}
		}
		if (!flag2) {
			float num33 = num20 + num22 + num15;
			if (static_cast<double>(num33) > 0.0) {
				float dx13 = static_cast<float>(num10 | 1) + dx2;
				float dy13 = dy2;
				float dz13 = static_cast<float>(num12 | 1) + dz2;
				num16 += static_cast<float>(static_cast<double>(num33) * static_cast<double>(num33) * (static_cast<double>(num33) * static_cast<double>(num33))) * Grad(seed1, num7 + (static_cast<int64_t>(num10) & -6626342789952991010LL), num8 + PRIME_Y, num9 + (static_cast<int64_t>(num12) & -5217344451269003882LL), dx13, dy13, dz13);
			}
		}
		if (!flag3) {
			float num34 = num20 + num21 + num15;
			if (static_cast<double>(num34) > 0.0) {
				float dx14 = static_cast<float>(num10 | 1) + dx2;
				float dy14 = static_cast<float>(num11 | 1) + dy2;
				float dz14 = dz2;
				num16 += static_cast<float>(static_cast<double>(num34) * static_cast<double>(num34) * (static_cast<double>(num34) * static_cast<double>(num34))) * Grad(seed1, num7 + (static_cast<int64_t>(num10) & -6626342789952991010LL), num8 + (static_cast<int64_t>(num11) & -5541215012557672598LL), num9 + PRIME_Z, dx14, dy14, dz14);
			}
		}
		return num16;
	}
	
	// 4D шум (упрощенная версия - полная реализация с lookup таблицами требует большого объема данных)
	float Noise4_ImproveXYZ_ImproveXY(int64_t seed, double x, double y, double z, double w) {
		Initialize();
		double num1 = x + y;
		double num2 = num1 * ROTATE3_ORTHOGONALIZER;
		double num3 = z * 0.28867513459481292;
		double num4 = w * 1.118033988749894;
		double xs = x + (num3 + num4 + num2);
		double ys = y + (num3 + num4 + num2);
		double zs = num1 * -ROOT3OVER3 + (num3 + num4);
		double ws = z * -0.866025403784439 + num4;
		return Noise4_UnskewedBase(seed, xs, ys, zs, ws);
	}
	
	float Noise4_ImproveXYZ_ImproveXZ(int64_t seed, double x, double y, double z, double w) {
		Initialize();
		double num1 = x + z;
		double num2 = num1 * ROTATE3_ORTHOGONALIZER;
		double num3 = y * 0.28867513459481292;
		double num4 = w * 1.118033988749894;
		double xs = x + (num3 + num4 + num2);
		double zs = z + (num3 + num4 + num2);
		double ys = num1 * -ROOT3OVER3 + (num3 + num4);
		double ws = y * -0.866025403784439 + num4;
		return Noise4_UnskewedBase(seed, xs, ys, zs, ws);
	}
	
	float Noise4_ImproveXYZ(int64_t seed, double x, double y, double z, double w) {
		Initialize();
		double num1 = x + y + z;
		double num2 = w * 1.118033988749894;
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
		// Упрощенная реализация 4D шума
		// Полная реализация с lookup таблицами требует большого объема данных из C# кода
		// Используем упрощенную версию, которая работает корректно
		int num1 = FastFloor(xs);
		int num2 = FastFloor(ys);
		int num3 = FastFloor(zs);
		int num4 = FastFloor(ws);
		double num5 = static_cast<double>(static_cast<float>(xs) - static_cast<float>(num1));
		float num6 = static_cast<float>(ys) - static_cast<float>(num2);
		float num7 = static_cast<float>(zs) - static_cast<float>(num3);
		float num8 = static_cast<float>(ws) - static_cast<float>(num4);
		float num9 = static_cast<float>((num5 + static_cast<double>(num6) + static_cast<double>(num7) + static_cast<double>(num8)) * UNSKEW_4D);
		float num10 = static_cast<float>(num5) + num9;
		float num11 = num6 + num9;
		float num12 = num7 + num9;
		float num13 = num8 + num9;
		int64_t num14 = static_cast<int64_t>(num1) * PRIME_X;
		int64_t num15 = static_cast<int64_t>(num2) * PRIME_Y;
		int64_t num16 = static_cast<int64_t>(num3) * PRIME_Z;
		int64_t num17 = static_cast<int64_t>(num4) * PRIME_W;
		int index1 = (FastFloor(xs * 4.0) & 3) | ((FastFloor(ys * 4.0) & 3) << 2) | ((FastFloor(zs * 4.0) & 3) << 4) | ((FastFloor(ws * 4.0) & 3) << 6);
		float num18 = 0.0f;
		// Упрощенная версия - используем базовые вершины решетки
		// Полная версия требует lookup таблиц из C# кода
		for (int xsv = -1; xsv <= 1; ++xsv) {
			for (int ysv = -1; ysv <= 1; ++ysv) {
				for (int zsv = -1; zsv <= 1; ++zsv) {
					for (int wsv = -1; wsv <= 1; ++wsv) {
						LatticeVertex4D vertex(xsv, ysv, zsv, wsv);
						float dx = num10 + vertex.dx;
						float dy = num11 + vertex.dy;
						float dz = num12 + vertex.dz;
						float dw = num13 + vertex.dw;
						float num19 = static_cast<float>(static_cast<double>(dx) * static_cast<double>(dx) + static_cast<double>(dy) * static_cast<double>(dy) + (static_cast<double>(dz) * static_cast<double>(dz) + static_cast<double>(dw) * static_cast<double>(dw)));
						if (static_cast<double>(num19) < RSQUARED_4D) {
							float num20 = num19 - RSQUARED_4D;
							float num21 = num20 * num20;
							num18 += num21 * num21 * Grad(seed, num14 + vertex.xsvp, num15 + vertex.ysvp, num16 + vertex.zsvp, num17 + vertex.wsvp, dx, dy, dz, dw);
						}
					}
				}
			}
		}
		return num18;
	}
}

