#include "Chunk.h"
#include "voxel.h"
#include <math.h>

Chunk::Chunk(){
	voxels = new voxel[CHUNK_VOL];
	for (int y = 0; y < CHUNK_H; y++){
		for (int z = 0; z < CHUNK_D; z++){
			for (int x = 0; x < CHUNK_W; x++){
				float hx = sinf(x * 0.30f);
				float hz = cosf(z * 0.30f);
				float height = (hx * 0.5f + hz * 0.5f) * 5.0f + 8.0f;
				voxels[(y * CHUNK_D + z) * CHUNK_W + x].density = (float)y - height;
			}
		}
	}
}

Chunk::~Chunk(){
	delete[] voxels;
}
