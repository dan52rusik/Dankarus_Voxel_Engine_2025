#include "SimplexNoise.h"

namespace SimplexNoise {
	// Константы
	static constexpr float onethird = 0.333333343f;
	static constexpr float onesixth = 0.166666672f;
	
	// Таблица T из C# кода
	static constexpr int T[8] = {
		21, 56, 50, 44, 13, 19, 7, 42
	};
	
	// Статические переменные для промежуточных вычислений
	// (в C++ они должны быть thread-local для потокобезопасности, но для простоты оставим статическими)
	static thread_local int i;
	static thread_local int j;
	static thread_local int k;
	static thread_local int A[3];
	static thread_local float u;
	static thread_local float v;
	static thread_local float w;
	static thread_local float s;
	
	// Вспомогательные функции
	static int fastfloor(float n) {
		return static_cast<double>(n) <= 0.0 ? static_cast<int>(n) - 1 : static_cast<int>(n);
	}
	
	static int b(int N, int B) {
		return (N >> B) & 1;
	}
	
	static int b(int i, int j, int k, int B) {
		return T[(b(i, B) << 2) | (b(j, B) << 1) | b(k, B)];
	}
	
	static int shuffle(int i, int j, int k) {
		return b(i, j, k, 0) + b(j, k, i, 1) + b(k, i, j, 2) + 
		       b(i, j, k, 3) + b(j, k, i, 4) + b(k, i, j, 5) + 
		       b(i, j, k, 6) + b(j, k, i, 7);
	}
	
	static float K(int a) {
		s = static_cast<float>(A[0] + A[1] + A[2]) * onesixth;
		float num1 = u - static_cast<float>(A[0]) + s;
		float num2 = v - static_cast<float>(A[1]) + s;
		float num3 = w - static_cast<float>(A[2]) + s;
		float num4 = 0.60000002384185791f - num1 * num1 - num2 * num2 - num3 * num3;
		
		int num5 = shuffle(i + A[0], j + A[1], k + A[2]);
		++A[a];
		
		if (static_cast<double>(num4) < 0.0) {
			return 0.0f;
		}
		
		int num6 = (num5 >> 5) & 1;
		int num7 = (num5 >> 4) & 1;
		int num8 = (num5 >> 3) & 1;
		int num9 = (num5 >> 2) & 1;
		int num10 = num5 & 3;
		
		float num11;
		float num12;
		float num13;
		float num14;
		float num15;
		float num16;
		
		switch (num10) {
			case 1:
				num11 = num1;
				num12 = num1;
				num13 = num2;
				num14 = num2;
				num15 = num3;
				num16 = num3;
				break;
			case 2:
				num11 = num2;
				num12 = num2;
				num13 = num3;
				num14 = num3;
				num15 = num1;
				num16 = num1;
				break;
			default:
				num11 = num3;
				num12 = num3;
				num13 = num1;
				num14 = num1;
				num15 = num2;
				num16 = num2;
				break;
		}
		
		float num17 = num6 == num8 ? -num12 : num12;
		float num18 = num6 == num7 ? -num14 : num14;
		float num19 = num6 != (num7 ^ num8) ? -num16 : num16;
		
		float num20 = num4 * num4;
		
		if (num10 == 0) {
			return static_cast<float>(8.0 * static_cast<double>(num20) * static_cast<double>(num20) * 
			                          (static_cast<double>(num17) + static_cast<double>(num18) + static_cast<double>(num19)));
		} else if (num9 == 0) {
			return static_cast<float>(8.0 * static_cast<double>(num20) * static_cast<double>(num20) * 
			                          (static_cast<double>(num17) + static_cast<double>(num18)));
		} else {
			return static_cast<float>(8.0 * static_cast<double>(num20) * static_cast<double>(num20) * 
			                          (static_cast<double>(num17) + static_cast<double>(num19)));
		}
	}
	
	float noise(float x, float y, float z) {
		s = static_cast<float>((static_cast<double>(x) + static_cast<double>(y) + static_cast<double>(z)) * 0.3333333432674408);
		i = fastfloor(x + s);
		j = fastfloor(y + s);
		k = fastfloor(z + s);
		s = static_cast<float>(i + j + k) * onesixth;
		u = x - static_cast<float>(i) + s;
		v = y - static_cast<float>(j) + s;
		w = z - static_cast<float>(k) + s;
		A[0] = A[1] = A[2] = 0;
		
		int a1;
		if (static_cast<double>(u) >= static_cast<double>(w)) {
			a1 = static_cast<double>(u) >= static_cast<double>(v) ? 0 : 1;
		} else {
			a1 = static_cast<double>(v) >= static_cast<double>(w) ? 1 : 2;
		}
		
		int a2;
		if (static_cast<double>(u) < static_cast<double>(w)) {
			a2 = static_cast<double>(u) < static_cast<double>(v) ? 0 : 1;
		} else {
			a2 = static_cast<double>(v) < static_cast<double>(w) ? 1 : 2;
		}
		
		return K(a1) + K(3 - a1 - a2) + K(a2) + K(0);
	}
}

