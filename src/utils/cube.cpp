#include "cube.h"

static inline void pushPosNorm(std::vector<float> &dst,
                               const glm::vec3 &p, const glm::vec3 &n) {
    dst.push_back(p.x); dst.push_back(p.y); dst.push_back(p.z);
    dst.push_back(n.x); dst.push_back(n.y); dst.push_back(n.z);
}

void Cube::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Cube::makeTile(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 2: create a tile (i.e. 2 triangles) based on 4 given points.
    // Flat normal from Triangle A (CCW)
    glm::vec3 e1 = bottomLeft  - topLeft;
    glm::vec3 e2 = bottomRight - topLeft;
    glm::vec3 n  = glm::normalize(glm::cross(e1, e2)); // unit normal

    // Triangle A
    pushPosNorm(m_vertexData, topLeft,     n);
    pushPosNorm(m_vertexData, bottomLeft,  n);
    pushPosNorm(m_vertexData, bottomRight, n);

    // Triangle B
    pushPosNorm(m_vertexData, topLeft,     n);
    pushPosNorm(m_vertexData, bottomRight, n);
    pushPosNorm(m_vertexData, topRight,    n);
}

void Cube::makeFace(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 3: create a single side of the cube out of the 4
    //         given points and makeTile()
    // Note: think about how param 1 affects the number of triangles on
    //       the face of the cube

    // Number of tiles per side (clamp to at least 1)

    int n = std::max(1, m_param1);
    float du = 1.0f / n;
    float dv = 1.0f / n;

    auto lerp = [](const glm::vec3 &a, const glm::vec3 &b, float t) {
        return a * (1.0f - t) + b * t;
    };
    auto pointAt = [&](float u, float v) -> glm::vec3 {
        glm::vec3 L = lerp(topLeft,  bottomLeft,  v);
        glm::vec3 R = lerp(topRight, bottomRight, v);
        return lerp(L, R, u);
    };

    for (int j = 0; j < n; ++j) {
        float v0 = j * dv,   v1 = (j + 1) * dv;
        for (int i = 0; i < n; ++i) {
            float u0 = i * du, u1 = (i + 1) * du;

            glm::vec3 TL = pointAt(u0, v0);
            glm::vec3 TR = pointAt(u1, v0);
            glm::vec3 BL = pointAt(u0, v1);
            glm::vec3 BR = pointAt(u1, v1);

            // CCW from the face’s front
            makeTile(TL, TR, BL, BR);
        }
    }


}

void Cube::setVertexData() {
    // Uncomment these lines for Task 2, then comment them out for Task 3:

    // makeTile(glm::vec3(-0.5f,  0.5f, 0.5f),
    // glm::vec3( 0.5f,  0.5f, 0.5f),
    // glm::vec3(-0.5f, -0.5f, 0.5f),
    // glm::vec3( 0.5f, -0.5f, 0.5f));


    // Uncomment these lines for Task 3:

    m_vertexData.clear();
    const float h = 0.5f;

    // +Z (front)
    makeFace(glm::vec3(-h,  h,  h),
             glm::vec3( h,  h,  h),
             glm::vec3(-h, -h,  h),
             glm::vec3( h, -h,  h));

    // -Z (back) — mirror X to keep CCW when seen from outside
    makeFace(glm::vec3( h,  h, -h),
             glm::vec3(-h,  h, -h),
             glm::vec3( h, -h, -h),
             glm::vec3(-h, -h, -h));

    // -X (left)
    makeFace(glm::vec3(-h,  h, -h),
             glm::vec3(-h,  h,  h),
             glm::vec3(-h, -h, -h),
             glm::vec3(-h, -h,  h));

    // +X (right)
    makeFace(glm::vec3( h,  h,  h),
             glm::vec3( h,  h, -h),
             glm::vec3( h, -h,  h),
             glm::vec3( h, -h, -h));

    // +Y (top)  FIXED ORDER (so normal points +Y)
    makeFace(glm::vec3(-h,  h, -h),  // TL
             glm::vec3( h,  h, -h),  // TR
             glm::vec3(-h,  h,  h),  // BL
             glm::vec3( h,  h,  h)); // BR

    // -Y (bottom) FIXED ORDER (so normal points -Y)
    makeFace(glm::vec3(-h, -h,  h),  // TL
             glm::vec3( h, -h,  h),  // TR
             glm::vec3(-h, -h, -h),  // BL
             glm::vec3( h, -h, -h)); // BR

    // Task 4: Use the makeFace() function to make all 6 sides of the cube
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cube::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
