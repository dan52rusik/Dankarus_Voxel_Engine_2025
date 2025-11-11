#ifndef VOXELS_DECOOBJECT_H_
#define VOXELS_DECOOBJECT_H_

#include <glm/glm.hpp>
#include <cstdint>
#include <istream>
#include <ostream>

// Состояние декорации
enum class DecoState : uint8_t {
	GeneratedActive = 0,    // Сгенерированная активная (видимая)
	GeneratedInactive = 1,  // Сгенерированная неактивная (скрытая)
	Dynamic = 2             // Динамическая (добавлена игроком)
};

// Объект декорации
// Адаптировано из 7 Days To Die DecoObject
class DecoObject {
public:
	glm::ivec3 pos;          // Позиция в мире (блоки)
	float realYPos;          // Реальная Y позиция (с плавающей точкой)
	uint8_t blockId;         // ID блока декорации
	uint8_t rotation;        // Поворот блока (0-3)
	DecoState state;         // Состояние декорации
	
	DecoObject();
	DecoObject(const glm::ivec3& _pos, float _realYPos, uint8_t _blockId, uint8_t _rotation, DecoState _state);
	
	// Инициализация
	void init(const glm::ivec3& _pos, float _realYPos, uint8_t _blockId, uint8_t _rotation, DecoState _state);
	
	// Сериализация
	void read(std::istream& stream);
	void write(std::ostream& stream) const;
	
	// Сравнение для HashSet
	bool operator==(const DecoObject& other) const {
		return pos == other.pos && blockId == other.blockId;
	}
};

// Хеш-функция для DecoObject (для использования в std::unordered_set)
struct DecoObjectHash {
	std::size_t operator()(const DecoObject& obj) const {
		std::size_t h1 = std::hash<int>()(obj.pos.x);
		std::size_t h2 = std::hash<int>()(obj.pos.y);
		std::size_t h3 = std::hash<int>()(obj.pos.z);
		std::size_t h4 = std::hash<uint8_t>()(obj.blockId);
		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
	}
};

#endif /* VOXELS_DECOOBJECT_H_ */

