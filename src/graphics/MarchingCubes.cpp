#include "MarchingCubes.h"
#include "Mesh.h"
#include "../voxels/MarchingTables.h"
#include <vector>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

using namespace glm;

namespace {
	// Helper: get index in 3D array (x-fastest order)
	inline int idx3(int x, int y, int z, int sx, int sy) {
		return (y * sy + z) * sx + x;
	}

	// Sample density with bounds checking
	inline float sampleDensity(const float* density, int x, int y, int z, int sx, int sy, int sz) {
		x = std::max(0, std::min(x, sx - 1));
		y = std::max(0, std::min(y, sy - 1));
		z = std::max(0, std::min(z, sz - 1));
		return density[idx3(x, y, z, sx, sy)];
	}

	// Calculate normal via gradient (central differences)
	vec3 calculateNormal(const float* density, int x, int y, int z, int sx, int sy, int sz) {
		float dx = sampleDensity(density, x + 1, y, z, sx, sy, sz) - sampleDensity(density, x - 1, y, z, sx, sy, sz);
		float dy = sampleDensity(density, x, y + 1, z, sx, sy, sz) - sampleDensity(density, x, y - 1, z, sx, sy, sz);
		float dz = sampleDensity(density, x, y, z + 1, sx, sy, sz) - sampleDensity(density, x, y, z - 1, sx, sy, sz);
		vec3 grad(dx, dy, dz);
		return normalize(grad);
	}

	// Create vertex by interpolating between two corner points
	struct Vertex {
		vec3 position;
		vec3 normal;
	};

	Vertex createVertex(const float* density, int x0, int y0, int z0, int x1, int y1, int z1,
	                    int sx, int sy, int sz, float isoLevel) {
		vec3 posA((float)x0, (float)y0, (float)z0);
		vec3 posB((float)x1, (float)y1, (float)z1);
		
		float densityA = sampleDensity(density, x0, y0, z0, sx, sy, sz);
		float densityB = sampleDensity(density, x1, y1, z1, sx, sy, sz);

		// Interpolate position
		float t = (isoLevel - densityA) / ((densityB - densityA) != 0.0f ? (densityB - densityA) : 1e-6f);
		vec3 position = posA + t * (posB - posA);

		// Interpolate normal
		vec3 normalA = calculateNormal(density, x0, y0, z0, sx, sy, sz);
		vec3 normalB = calculateNormal(density, x1, y1, z1, sx, sy, sz);
		vec3 normal = normalize(normalA + t * (normalB - normalA));

		Vertex v;
		v.position = position;
		v.normal = normal;
		return v;
	}
}

Mesh* buildIsoSurface(const float* density, int nx, int ny, int nz, float isoLevel) {
	// Grid nodes: (nx+1) x (ny+1) x (nz+1)
	const int sx = nx + 1;
	const int sy = ny + 1;
	const int sz = nz + 1;

	std::vector<float> buffer;
	buffer.reserve(nx * ny * nz * 15 * 6); // rough estimate: 6 floats per vertex (pos + normal)

	int attrs[] = {3, 3, 0}; // position(3), normal(3), terminator(0)

	// Process each cube
	for (int y = 0; y < ny; y++) {
		for (int z = 0; z < nz; z++) {
			for (int x = 0; x < nx; x++) {
				// Calculate coordinates of each corner of the current cube
				// Order matches Unity: (0,0,0), (1,0,0), (1,0,1), (0,0,1), (0,1,0), (1,1,0), (1,1,1), (0,1,1)
				int cornerCoords[8][3] = {
					{x,     y,     z    },
					{x + 1, y,     z    },
					{x + 1, y,     z + 1},
					{x,     y,     z + 1},
					{x,     y + 1, z    },
					{x + 1, y + 1, z    },
					{x + 1, y + 1, z + 1},
					{x,     y + 1, z + 1}
				};

				// Calculate cube configuration
				int cubeConfiguration = 0;
				for (int i = 0; i < 8; i++) {
					float d = sampleDensity(density, cornerCoords[i][0], cornerCoords[i][1], cornerCoords[i][2], sx, sy, sz);
					if (d < isoLevel) {
						cubeConfiguration |= (1 << i);
					}
				}

				// Get triangulation for this configuration
				const int* edgeIndices = TRIANGULATION_TABLE[cubeConfiguration];

				// Create triangles
				for (int i = 0; i < 16; i += 3) {
					if (edgeIndices[i] == -1) break;

					// Get edge indices
					int edgeIndexA = edgeIndices[i];
					int edgeIndexB = edgeIndices[i + 1];
					int edgeIndexC = edgeIndices[i + 2];

					// Get corner indices for each edge
					int a0 = CORNER_INDEX_A_FROM_EDGE[edgeIndexA];
					int a1 = CORNER_INDEX_B_FROM_EDGE[edgeIndexA];
					int b0 = CORNER_INDEX_A_FROM_EDGE[edgeIndexB];
					int b1 = CORNER_INDEX_B_FROM_EDGE[edgeIndexB];
					int c0 = CORNER_INDEX_A_FROM_EDGE[edgeIndexC];
					int c1 = CORNER_INDEX_B_FROM_EDGE[edgeIndexC];

					// Create vertices (order: C, B, A - reverse order like Unity!)
					Vertex vertexC = createVertex(density,
						cornerCoords[c0][0], cornerCoords[c0][1], cornerCoords[c0][2],
						cornerCoords[c1][0], cornerCoords[c1][1], cornerCoords[c1][2],
						sx, sy, sz, isoLevel);
					Vertex vertexB = createVertex(density,
						cornerCoords[b0][0], cornerCoords[b0][1], cornerCoords[b0][2],
						cornerCoords[b1][0], cornerCoords[b1][1], cornerCoords[b1][2],
						sx, sy, sz, isoLevel);
					Vertex vertexA = createVertex(density,
						cornerCoords[a0][0], cornerCoords[a0][1], cornerCoords[a0][2],
						cornerCoords[a1][0], cornerCoords[a1][1], cornerCoords[a1][2],
						sx, sy, sz, isoLevel);

					// Emit triangle vertices (C, B, A order - reverse winding!)
					// Vertex C
					buffer.push_back(vertexC.position.x);
					buffer.push_back(vertexC.position.y);
					buffer.push_back(vertexC.position.z);
					buffer.push_back(vertexC.normal.x);
					buffer.push_back(vertexC.normal.y);
					buffer.push_back(vertexC.normal.z);

					// Vertex B
					buffer.push_back(vertexB.position.x);
					buffer.push_back(vertexB.position.y);
					buffer.push_back(vertexB.position.z);
					buffer.push_back(vertexB.normal.x);
					buffer.push_back(vertexB.normal.y);
					buffer.push_back(vertexB.normal.z);

					// Vertex A
					buffer.push_back(vertexA.position.x);
					buffer.push_back(vertexA.position.y);
					buffer.push_back(vertexA.position.z);
					buffer.push_back(vertexA.normal.x);
					buffer.push_back(vertexA.normal.y);
					buffer.push_back(vertexA.normal.z);
				}
			}
		}
	}

	return new Mesh(buffer.data(), buffer.size() / 6, attrs);
}

