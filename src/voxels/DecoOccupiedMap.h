#ifndef VOXELS_DECOCCUPIEDMAP_H_
#define VOXELS_DECOCCUPIEDMAP_H_

#include <vector>
#include <cstdint>

// Тип занятости декорацией
enum class EnumDecoOccupied : uint8_t {
	Free = 0,              // Свободно
	Deco = 1,              // Декорация
	POI = 2,               // POI (Point of Interest)
	Stop_BigDeco = 3,      // Остановка больших декораций
	Perimeter = 4,         // Периметр (вокруг больших декораций)
	NoneAllowed = 255      // Ничего не разрешено
};

// Карта занятости для декораций
// Адаптировано из 7 Days To Die DecoOccupiedMap
class DecoOccupiedMap {
public:
	int worldWidth;
	int worldHeight;
	std::vector<EnumDecoOccupied> data;
	
	DecoOccupiedMap(int _worldWidth, int _worldHeight);
	
	// Получить значение в точке
	EnumDecoOccupied get(int x, int z) const;
	
	// Установить значение в точке
	void set(int x, int z, EnumDecoOccupied value);
	
	// Установить область
	void setArea(int x, int z, EnumDecoOccupied value, int sizeX, int sizeZ);
	
	// Проверить область
	bool checkArea(int x, int z, EnumDecoOccupied value, int sizeX, int sizeZ) const;
	
	// Получить данные (для копирования)
	const std::vector<EnumDecoOccupied>& getData() const { return data; }
	
private:
	// Преобразовать координаты в индекс
	int toIndex(int x, int z) const;
	
	// Проверить границы
	bool isValid(int x, int z) const;
};

#endif /* VOXELS_DECOCCUPIEDMAP_H_ */

