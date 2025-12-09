#pragma once

#include <vector>
#include <glm/glm.hpp>

// Simple helper that generates a flat grid of triangles.
// Layout:  [px, py, pz, nx, ny, nz] per vertex
class TerrainGenerator {
public:
    // resolution = number of quads per side (N x N grid)
    // size       = world-space width / depth of the terrain
    std::vector<float> generateFlatGrid(int resolution, float size);
};
