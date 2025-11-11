#ifndef VOXELS_WATERCONSTANTS_H_
#define VOXELS_WATERCONSTANTS_H_

// Константы для симуляции воды
// Адаптировано из 7 Days To Die WaterConstants
namespace WaterConstants {
	// Константы масс (адаптированы под наш формат: 0-255 вместо 0-19500)
	// В 7DTD: MIN_MASS=195, MAX_MASS=19500, OVERFULL_MAX=58500
	// У нас: MIN_MASS=195, MAX_MASS=255, OVERFULL_MAX=255 (упрощено)
	static constexpr int MIN_MASS = 195;           // Минимальная масса для активной воды
	static constexpr int MAX_MASS = 255;            // Максимальная масса воды (полный воксель)
	static constexpr int OVERFULL_MAX = 255;        // Максимальная масса при переполнении
	static constexpr int MIN_FLOW = 1;              // Минимальный поток
	static constexpr float FLOW_SPEED = 0.5f;       // Скорость потока (множитель)
	static constexpr int MIN_MASS_SIDE_SPREAD = 25; // Минимальная масса для бокового распространения
	
	// Вычислить стабильную массу ниже (для потока вниз)
	inline int GetStableMassBelow(int mass, int massBelow) {
		// В 7DTD: min(mass + massBelow, MAX_MASS)
		// У нас упрощено: просто ограничиваем максимумом
		return (mass + massBelow > MAX_MASS) ? MAX_MASS : (mass + massBelow);
	}
}

#endif /* VOXELS_WATERCONSTANTS_H_ */

