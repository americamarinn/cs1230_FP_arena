#include "terraingenerator.h"
#include <algorithm>

std::vector<float> TerrainGenerator::generateFlatGrid(int resolution, float size) {
    std::vector<float> data;

    int N = std::max(1, resolution);
    float half = size * 0.5f;
    glm::vec3 n(0.f, 1.f, 0.f); // up normal

    auto pushVertex = [&](float x, float z) {
        data.push_back(x);      // pos.x
        data.push_back(0.f);    // pos.y (flat plane)
        data.push_back(z);      // pos.z
        data.push_back(n.x);    // normal.x
        data.push_back(n.y);    // normal.y
        data.push_back(n.z);    // normal.z
    };

    // Build N x N quads, each as two triangles
    for (int zi = 0; zi < N; ++zi) {
        float z0 = -half + size *  zi      / float(N);
        float z1 = -half + size * (zi + 1) / float(N);

        for (int xi = 0; xi < N; ++xi) {
            float x0 = -half + size *  xi      / float(N);
            float x1 = -half + size * (xi + 1) / float(N);

            // Triangle 1
            pushVertex(x0, z0);
            pushVertex(x1, z0);
            pushVertex(x1, z1);

            // Triangle 2
            pushVertex(x0, z0);
            pushVertex(x1, z1);
            pushVertex(x0, z1);
        }
    }

    return data;
}

