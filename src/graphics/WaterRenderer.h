#ifndef GRAPHICS_WATERRENDERER_H_
#define GRAPHICS_WATERRENDERER_H_

#include <stdlib.h>
#include <vector>

class Mesh;
class MCChunk;
class WaterData;

// Рендерер для воды
// Генерирует меш поверхности воды на основе активных вокселей воды
class WaterRenderer {
	float* buffer;
	size_t capacity;
	
public:
	WaterRenderer(size_t capacity);
	~WaterRenderer();
	
	// Генерировать меш воды для чанка
	// Генерирует верхние грани для всех активных вокселей воды
	Mesh* render(MCChunk* chunk);
	
private:
	// Добавить верхнюю грань вокселя воды в буфер
	void addWaterTopFace(float*& bufferPtr, float x, float y, float z, float size);
	
	// Проверить, нужно ли рендерить грань (если соседний воксель не содержит воду)
	bool shouldRenderFace(const WaterData* waterData, int x, int y, int z, int dx, int dy, int dz) const;
};

#endif /* GRAPHICS_WATERRENDERER_H_ */

