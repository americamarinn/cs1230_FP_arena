#include "cone.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>   // glm::pi, glm::two_pi
#include <algorithm>
#include <cmath>


static inline void pushPosNorm(std::vector<float>& dst,
                               const glm::vec3& p, const glm::vec3& n) {
    dst.push_back(p.x); dst.push_back(p.y); dst.push_back(p.z);
    dst.push_back(n.x); dst.push_back(n.y); dst.push_back(n.z);
}

static inline glm::vec3 calcNorm(const glm::vec3& pt) {
    float xNorm = 2.f * pt.x;
    float yNorm = -(1.f/4.f) * (2.f * pt.y - 1.f);
    float zNorm = 2.f * pt.z;
    return glm::normalize(glm::vec3{xNorm, yNorm, zNorm});
}

static inline glm::vec3 cyl(float r, float theta, float y) {
    return glm::vec3(r * std::cos(theta), y, r * std::sin(theta));
}

// Radius of the cone at height y (linear from 0.5 at y=-0.5 to 0 at y=+0.5)
static inline float radiusAtY(float y) {
    // Map y ∈ [-0.5, 0.5] to t ∈ [0,1], r = 0.5 * (1 - t)
    float t = (y + 0.5f);               // 0 at bottom, 1 at top
    return 0.5f * (1.f - t);
}

void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Cone::makeCapSlice(float theta0, float theta1) {
    const float y = -0.5f;
    const int   n = std::max(1, m_param1);
    const glm::vec3 nCap(0.f, -1.f, 0.f);

    for (int i = 0; i < n; ++i) {
        float r0 = (i    / float(n)) * 0.5f;
        float r1 = ((i+1)/ float(n)) * 0.5f;

        glm::vec3 I0 = cyl(r0, theta0, y);
        glm::vec3 I1 = cyl(r0, theta1, y);
        glm::vec3 O0 = cyl(r1, theta0, y);
        glm::vec3 O1 = cyl(r1, theta1, y);

        // Triangle A: I0 -> O0 -> O1
        pushPosNorm(m_vertexData, I0, nCap);
        pushPosNorm(m_vertexData, O0, nCap);
        pushPosNorm(m_vertexData, O1, nCap);
        // Triangle B: I0 -> O1 -> I1
        pushPosNorm(m_vertexData, I0, nCap);
        pushPosNorm(m_vertexData, O1, nCap);
        pushPosNorm(m_vertexData, I1, nCap);
    }
}

void Cone::makeSlopeSlice(float theta0, float theta1) {
    const int n = std::max(1, m_param1);      // vertical tiles along height
    const float yBottom = -0.5f, yTop = 0.5f;
    const float dy = (yTop - yBottom) / n;

    // Mid-theta for tip normal fallback
    //THESE
    const float thetaMid = 0.5f * (theta0 + theta1);
    glm::vec3 tipDir = glm::normalize(glm::vec3(std::cos(thetaMid), 0.f, std::sin(thetaMid)));

    for (int i = 0; i < n; ++i) {
        float y0 = yBottom + i * dy;
        float y1 = yBottom + (i+1) * dy;
        float r0 = radiusAtY(y0);
        float r1 = radiusAtY(y1);

        glm::vec3 TL = cyl(r0, theta0, y0);
        glm::vec3 TR = cyl(r0, theta1, y0);
        glm::vec3 BL = cyl(r1, theta0, y1);
        glm::vec3 BR = cyl(r1, theta1, y1);

        // Per-vertex normals using implicit-cone gradient
        //THESE IF RADIUS 0 THEN YOU USE SPECIAL CASE
        auto nTL = (r0 == 0.f) ? glm::normalize(glm::vec3(tipDir.x, 1.f, tipDir.z)) : calcNorm(TL);
        auto nTR = (r0 == 0.f) ? glm::normalize(glm::vec3(tipDir.x, 1.f, tipDir.z)) : calcNorm(TR);
        auto nBL = (r1 == 0.f) ? glm::normalize(glm::vec3(tipDir.x, 1.f, tipDir.z)) : calcNorm(BL);
        auto nBR = (r1 == 0.f) ? glm::normalize(glm::vec3(tipDir.x, 1.f, tipDir.z)) : calcNorm(BR);

        // Keep CCW as seen from *outside* the cone
        // Triangle A: TL -> BL -> BR
        pushPosNorm(m_vertexData, TL, nTL);
        pushPosNorm(m_vertexData, BL, nBL);
        pushPosNorm(m_vertexData, BR, nBR);
        // Triangle B: TL -> BR -> TR
        pushPosNorm(m_vertexData, TL, nTL);
        pushPosNorm(m_vertexData, BR, nBR);
        pushPosNorm(m_vertexData, TR, nTR);
    }
}

void Cone::makeWedge(float theta0, float theta1) {
    makeCapSlice(theta0, theta1);
    makeSlopeSlice(theta0, theta1);
}


void Cone::setVertexData() {
    // TODO for Project 5: Lights, Camera
    m_vertexData.clear();

    const int   wedges = std::max(3, m_param2);
    const float dTheta = glm::two_pi<float>() / wedges;

    for (int k = 0; k < wedges; ++k) {
        float theta0 = k * dTheta;
        float theta1 = (k + 1) * dTheta;
        makeWedge(theta0, theta1);
    }
}


// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
