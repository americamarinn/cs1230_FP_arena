#include "sphere.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>   // glm::pi
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

inline void pushPosNorm(std::vector<float> &dst, const glm::vec3 &p) {
    glm::vec3 n = glm::normalize(p);
    dst.push_back(p.x); dst.push_back(p.y); dst.push_back(p.z);
    dst.push_back(n.x); dst.push_back(n.y); dst.push_back(n.z);
}

static inline glm::vec3 sph(float r, float phi, float theta) {
    float s = sinf(phi), c = cosf(phi);
    float ct = cosf(theta), st = sinf(theta);
    return glm::vec3(r * s * ct,
                     r * c,
                     -r * s * st);  // note the minus on z
}

void Sphere::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    m_param2 = param2;
    setVertexData();
}

void Sphere::makeTile(glm::vec3 topLeft,
                      glm::vec3 topRight,
                      glm::vec3 bottomLeft,
                      glm::vec3 bottomRight) {
    // Task 5: Implement the makeTile() function for a Sphere
    // Note: this function is very similar to the makeTile() function for Cube,
    //       but the normals are calculated in a different way!

    // Triangle A: TL -> BL -> BR
    pushPosNorm(m_vertexData, topLeft);
    pushPosNorm(m_vertexData, bottomLeft);
    pushPosNorm(m_vertexData, bottomRight);

    // Triangle B: TL -> BR -> TR
    pushPosNorm(m_vertexData, topLeft);
    pushPosNorm(m_vertexData, bottomRight);
    pushPosNorm(m_vertexData, topRight);
}

void Sphere::makeWedge(float currentTheta, float nextTheta) {
    // Task 6: create a single wedge of the sphere using the
    //         makeTile() function you implemented in Task 5
    // Note: think about how param 1 comes into play here!

    m_vertexData.reserve(m_vertexData.size() + 6 * m_param1); // small reserve

    const float r = 0.5f;
    int rows = std::max(2, m_param1);                  // at least 2 phi rings
    float dphi = glm::pi<float>() / rows;              // [0, pi]

    for (int i = 0; i < rows; ++i) {
        float phi0 = i * dphi;         // top of the band
        float phi1 = (i + 1) * dphi;   // bottom of the band

        // Four band corners for this wedge slice
        glm::vec3 TL = sph(r, phi0, currentTheta);
        glm::vec3 TR = sph(r, phi0, nextTheta);
        glm::vec3 BL = sph(r, phi1, currentTheta);
        glm::vec3 BR = sph(r, phi1, nextTheta);

        // Keep CCW when viewed from outside
        makeTile(TL, TR, BL, BR);
    }
}

void Sphere::makeSphere() {
    // Task 7: create a full sphere using the makeWedge() function you
    //         implemented in Task 6
    // Note: think about how param 2 comes into play here!
    const int cols = std::max(3, m_param2);            // at least 3 wedges
    const float dtheta = glm::two_pi<float>() / cols;

    for (int k = 0; k < cols; ++k) {
        float theta0 = k * dtheta;
        float theta1 = (k + 1) * dtheta;
        makeWedge(theta0, theta1);
    }
}

void Sphere::setVertexData() {
    // Uncomment these lines to make a wedge for Task 6, then comment them out for Task 7:

    // float thetaStep = glm::radians(360.f / m_param2);
    // float currentTheta = 0 * thetaStep;
    // float nextTheta = 1 * thetaStep;
    // makeWedge(currentTheta, nextTheta);

    // Uncomment these lines to make sphere for Task 7:
    m_vertexData.clear();
    makeSphere();
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Sphere::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
