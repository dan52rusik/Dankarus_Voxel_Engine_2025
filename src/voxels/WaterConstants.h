#ifndef VOXELS_WATERCONSTANTS_H_
#define VOXELS_WATERCONSTANTS_H_

#include <algorithm> // для std::min

// Константы для симуляции воды
// Адаптировано из 7 Days To Die WaterConstants
namespace WaterConstants {
	// Константы масс (адаптированы под наш формат: 0-255 вместо 0-19500)
	// В 7DTD: MIN_MASS=195, MAX_MASS=19500, OVERFULL_MAX=58500
	// У нас: понижены пороги для более живой симуляции
	static constexpr int MIN_MASS = 10;              // Минимальная масса для активной воды (было 195)
	static constexpr int MAX_MASS = 255;            // Максимальная масса воды (полный воксель)
	static constexpr int OVERFULL_MAX = 255;        // Максимальная масса при переполнении
	static constexpr int MIN_FLOW = 1;              // Минимальный поток
	static constexpr float FLOW_SPEED = 0.4f;       // Скорость потока (множитель) - слегка снижена для плавности
	static constexpr int MIN_MASS_SIDE_SPREAD = 5;  // Минимальная масса для бокового распространения (было 25)
	
	// Вычислить стабильную массу ниже (для потока вниз)
	// Улучшенная версия с небольшим переливом для более естественного поведения
	inline int GetStableMassBelow(int mass, int massBelow) {
		int totalMass = mass + massBelow;
		
		// Если суммарная масса не превышает максимум - всё стекает вниз
		if (totalMass <= MAX_MASS) {
			return totalMass;
		}
		
		// Если снизу уже полный блок, но сверху есть вода - разрешаем небольшой перелив
		// Это позволяет воде продолжать течь даже когда снизу уже MAX_MASS
		// Перелив ограничен небольшим порогом для предотвращения бесконечного накопления
		if (massBelow >= MAX_MASS) {
			// Разрешаем небольшой перелив (до 5 единиц сверх максимума)
			// Это позволяет верхнему блоку потихоньку стекать
			int overflow = std::min(mass, 5);
			return MAX_MASS + overflow;
		}
		
		// Стандартный случай: ограничиваем максимумом
		return MAX_MASS;
	}
}

#endif /* VOXELS_WATERCONSTANTS_H_ */

