#include "DecoObject.h"
#include <iostream>
#include <cstring>

DecoObject::DecoObject()
	: pos(0), realYPos(0.0f), blockId(0), rotation(0), state(DecoState::GeneratedActive) {
}

DecoObject::DecoObject(const glm::ivec3& _pos, float _realYPos, uint8_t _blockId, uint8_t _rotation, DecoState _state)
	: pos(_pos), realYPos(_realYPos), blockId(_blockId), rotation(_rotation), state(_state) {
}

void DecoObject::init(const glm::ivec3& _pos, float _realYPos, uint8_t _blockId, uint8_t _rotation, DecoState _state) {
	pos = _pos;
	realYPos = _realYPos;
	blockId = _blockId;
	rotation = _rotation;
	state = _state;
}

void DecoObject::read(std::istream& stream) {
	stream.read(reinterpret_cast<char*>(&pos.x), sizeof(int));
	stream.read(reinterpret_cast<char*>(&pos.y), sizeof(int));
	stream.read(reinterpret_cast<char*>(&pos.z), sizeof(int));
	stream.read(reinterpret_cast<char*>(&realYPos), sizeof(float));
	stream.read(reinterpret_cast<char*>(&blockId), sizeof(uint8_t));
	stream.read(reinterpret_cast<char*>(&rotation), sizeof(uint8_t));
	uint8_t stateByte;
	stream.read(reinterpret_cast<char*>(&stateByte), sizeof(uint8_t));
	state = static_cast<DecoState>(stateByte);
}

void DecoObject::write(std::ostream& stream) const {
	stream.write(reinterpret_cast<const char*>(&pos.x), sizeof(int));
	stream.write(reinterpret_cast<const char*>(&pos.y), sizeof(int));
	stream.write(reinterpret_cast<const char*>(&pos.z), sizeof(int));
	stream.write(reinterpret_cast<const char*>(&realYPos), sizeof(float));
	stream.write(reinterpret_cast<const char*>(&blockId), sizeof(uint8_t));
	stream.write(reinterpret_cast<const char*>(&rotation), sizeof(uint8_t));
	uint8_t stateByte = static_cast<uint8_t>(state);
	stream.write(reinterpret_cast<const char*>(&stateByte), sizeof(uint8_t));
}

