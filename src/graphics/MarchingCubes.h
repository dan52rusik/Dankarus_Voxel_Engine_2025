#ifndef GRAPHICS_MARCHINGCUBES_H_
#define GRAPHICS_MARCHINGCUBES_H_

#include <cstddef>

class Mesh;

// density: scalar field sized (nx+1)*(ny+1)*(nz+1) in x-fastest order
// isoLevel: threshold for isosurface extraction
Mesh* buildIsoSurface(const float* density, int nx, int ny, int nz, float isoLevel);

#endif /* GRAPHICS_MARCHINGCUBES_H_ */

