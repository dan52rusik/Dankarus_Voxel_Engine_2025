#ifndef VOXELS_DECOCHUNK_H_
#define VOXELS_DECOCHUNK_H_

#include "DecoObject.h"
#include <unordered_map>
#include <vector>
#include <list>
#include <cstdint>
#include <glm/glm.hpp>

// Чанк декораций (128x128 блоков)
// Адаптировано из 7 Days To Die DecoChunk
class DecoChunk {
public:
	static const int CHUNK_SIZE = 128; // Размер чанка декораций (128x128)
	
	int decoChunkX;           // Координата чанка X
	int decoChunkZ;           // Координата чанка Z
	bool isDecorated;         // Декорирован ли чанк
	bool isGameObjectUpdated; // Обновлены ли игровые объекты
	bool isModelsUpdated;     // Обновлены ли модели
	
	// Декорации, сгруппированные по маленьким чанкам (16x16)
	// Ключ: chunkKey (x, z) -> список декораций
	std::unordered_map<int64_t, std::list<DecoObject>> decosPerSmallChunks;
	
	DecoChunk(int _decoChunkX, int _decoChunkZ);
	~DecoChunk();
	
	// Преобразовать мировые координаты в координаты чанка декораций
	static int toDecoChunkPos(int worldPos);
	
	// Создать ключ из координат чанка (16-bit)
	static int makeKey16(int x, int z);
	
	// Добавить декорацию
	void addDecoObject(const DecoObject& deco, bool updateImmediately = false);
	
	// Удалить декорацию по позиции
	bool removeDecoObject(const glm::ivec3& pos);
	
	// Удалить декорацию по объекту
	bool removeDecoObject(const DecoObject& deco);
	
	// Получить декорацию по позиции
	DecoObject* getDecoObjectAt(const glm::ivec3& pos);
	
	// Установить видимость
	void setVisible(bool visible);
	
	// Уничтожить чанк (очистить все декорации)
	void destroy();
	
	// Восстановить сгенерированные декорации (для reset)
	void restoreGeneratedDecos(int64_t worldChunkKey);
	
private:
	bool isVisible;
	
	// Получить ключ маленького чанка из мировых координат
	static int64_t makeSmallChunkKey(int worldX, int worldZ);
};

#endif /* VOXELS_DECOCHUNK_H_ */

