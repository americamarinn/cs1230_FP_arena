#include "cylinder.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

// push interleaved (pos, normal)
static inline void pushPosNorm(std::vector<float>& dst,
                               const glm::vec3& p, const glm::vec3& n) {
    dst.push_back(p.x); dst.push_back(p.y); dst.push_back(p.z);
    dst.push_back(n.x); dst.push_back(n.y); dst.push_back(n.z);
}

static inline glm::vec3 cyl(float r, float theta, float y) {
    return glm::vec3(r * std::cos(theta), y, r * std::sin(theta));
}

static inline glm::vec3 radialNormal(const glm::vec3& p) {
    glm::vec3 n(p.x, 0.f, p.z);
    float len = std::sqrt(std::max(1e-12f, n.x*n.x + n.z*n.z));
    return glm::vec3(n.x/len, 0.f, n.z/len);
}

void Cylinder::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Cylinder::makeTopCapSlice(float theta0, float theta1) {
    const float y = 0.5f;
    const int n = m_param1;
    const glm::vec3 nTop(0.f, 1.f, 0.f);

    for (int i = 0; i < n; ++i) {
        float r0 = (i    / float(n)) * 0.5f;
        float r1 = ((i+1)/ float(n)) * 0.5f;

        glm::vec3 I0 = cyl(r0, theta0, y);
        glm::vec3 I1 = cyl(r0, theta1, y);
        glm::vec3 O0 = cyl(r1, theta0, y);
        glm::vec3 O1 = cyl(r1, theta1, y);

        // CCW from above for +Y normal
        // A: I0 -> O1 -> O0
        pushPosNorm(m_vertexData, I0, nTop);
        pushPosNorm(m_vertexData, O1, nTop);
        pushPosNorm(m_vertexData, O0, nTop);
        // B: I0 -> I1 -> O1
        pushPosNorm(m_vertexData, I0, nTop);
        pushPosNorm(m_vertexData, I1, nTop);
        pushPosNorm(m_vertexData, O1, nTop);
    }
}

// BOTTOM cap (y = -0.5), outward normal -Y.
// Winding chosen so it’s front-facing when viewed from ABOVE as well
// (flip if your stencil expects the bottom invisible from above).
void Cylinder::makeBottomCapSlice(float theta0, float theta1) {
    const float y = -0.5f;
    const int n = m_param1;
    const glm::vec3 nBot(0.f, -1.f, 0.f);

    for (int i = 0; i < n; ++i) {
        float r0 = (i    / float(n)) * 0.5f;
        float r1 = ((i+1)/ float(n)) * 0.5f;

        glm::vec3 I0 = cyl(r0, theta0, y);
        glm::vec3 I1 = cyl(r0, theta1, y);
        glm::vec3 O0 = cyl(r1, theta0, y);
        glm::vec3 O1 = cyl(r1, theta1, y);

        // Front-facing from above with -Y normals
        // A: I0 -> O0 -> O1
        pushPosNorm(m_vertexData, I0, nBot);
        pushPosNorm(m_vertexData, O0, nBot);
        pushPosNorm(m_vertexData, O1, nBot);
        // B: I0 -> O1 -> I1
        pushPosNorm(m_vertexData, I0, nBot);
        pushPosNorm(m_vertexData, O1, nBot);
        pushPosNorm(m_vertexData, I1, nBot);
    }
}

void Cylinder::makeSideSlice(float theta0, float theta1) {
    const int n = m_param1;                 // vertical bands
    const float y0 = -0.5f, y1 = 0.5f;
    const float dy = (y1 - y0) / n;
    const float r = 0.5f;

    for (int i = 0; i < n; ++i) {
        float ya = y0 + i * dy;
        float yb = y0 + (i + 1) * dy;

        glm::vec3 TL = cyl(r, theta0, yb);
        glm::vec3 TR = cyl(r, theta1, yb);
        glm::vec3 BL = cyl(r, theta0, ya);
        glm::vec3 BR = cyl(r, theta1, ya);

        glm::vec3 nTL = radialNormal(TL);
        glm::vec3 nTR = radialNormal(TR);
        glm::vec3 nBL = radialNormal(BL);
        glm::vec3 nBR = radialNormal(BR);

        // CCW from the outside
        // A: TL -> BL -> BR
        pushPosNorm(m_vertexData, TL, nTL);
        pushPosNorm(m_vertexData, BR, nBR);
        pushPosNorm(m_vertexData, BL, nBL);

        pushPosNorm(m_vertexData, TL, nTL);
        pushPosNorm(m_vertexData, TR, nTR);
        pushPosNorm(m_vertexData, BR, nBR);
    }
}

void Cylinder::makeWedge(float theta0, float theta1) {
    makeTopCapSlice(theta0, theta1);
    makeBottomCapSlice(theta0, theta1);
    makeSideSlice(theta0, theta1);
}

void Cylinder::setVertexData() {
    // TODO for Project 5: Lights, Camera
    m_vertexData.clear();

    const int   wedges = std::max(3, m_param2);
    const float dTheta = glm::two_pi<float>() / wedges;

    for (int k = 0; k < wedges; ++k) {
        float a = k * dTheta;
        float b = (k + 1) * dTheta;
        makeWedge(a, b);
    }
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cylinder::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
