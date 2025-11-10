#ifndef GRAPHICS_VOXELRENDERER_H_
#define GRAPHICS_VOXELRENDERER_H_

#include <stdlib.h>

class Mesh;
class MCChunk;

namespace lighting {
    class LightingSystem;
}

class VoxelRenderer {
	float* buffer;
	size_t capacity;
public:
	VoxelRenderer(size_t capacity);
	~VoxelRenderer();

	Mesh* render(MCChunk* chunk, MCChunk** nearbyChunks, lighting::LightingSystem* lightingSystem = nullptr);
};

#endif /* GRAPHICS_VOXELRENDERER_H_ */
