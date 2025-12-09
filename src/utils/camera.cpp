#include "camera.h"
#include <glm/glm.hpp>
#include <cmath>

#include <stdexcept>


Camera::Camera()
{
    rebuildView();
}

void Camera::setViewMatrix(const glm::vec3 &pos,
                           const glm::vec3 &look,
                           const glm::vec3 &up)
{
    m_pos  = pos;
    m_look = glm::normalize(look);
    m_up   = glm::normalize(up);
    rebuildView();
}


void Camera::rebuildView()
{
    // Build camera basis from pose
    glm::vec3 w = -glm::normalize(m_look);          // forward
    glm::vec3 v = glm::normalize(m_up);             // up
    glm::vec3 u = glm::normalize(glm::cross(v, w)); // right
    v = glm::cross(w, u);                           // re-orthogonalize up

    // Rotation part: rows [u; v; w] ⇒ columns are (u.x,u.y,u.z,0), etc.
    glm::mat4 rot(1.f);
    rot[0] = glm::vec4(u.x, v.x, w.x, 0.f);
    rot[1] = glm::vec4(u.y, v.y, w.y, 0.f);
    rot[2] = glm::vec4(u.z, v.z, w.z, 0.f);

    // Translation
    glm::mat4 trans(1.f);
    trans[3] = glm::vec4(-m_pos, 1.f);

    // View matrix = rotation * translation
    m_view = rot * trans;
}


void Camera::setProjectionMatrix(float aspect,
                                 float nearPlane,
                                 float farPlane,
                                 float heightAngle)
{
    m_aspect = aspect;
    m_near   = nearPlane;
    m_far    = farPlane;
    m_fovy   = heightAngle;

    // Avoid degenerate clip planes
    if (m_far <= m_near + 1e-4f) {
        m_far = m_near + 1e-4f;
    }

    float t = std::tan(m_fovy * 0.5f);
    float A = 1.f / (m_aspect * t);
    float B = 1.f / t;
    float C = -(m_far + m_near) / (m_far - m_near);
    float D = -(2.f * m_far * m_near) / (m_far - m_near);

    glm::mat4 P(0.f);
    P[0][0] = A;
    P[1][1] = B;
    P[2][2] = C;
    P[2][3] = -1.f;
    P[3][2] = D;

    m_proj = P;
}

void Camera::translate(const glm::vec3 &delta)
{
    m_pos += delta;
    rebuildView();
}

void Camera::rotateAroundUp(float angle)
{
    // yaw around world up (0,1,0)
    glm::vec3 k(0.f, 1.f, 0.f);
    glm::vec3 d = m_look;

    float c = std::cos(angle);
    float s = std::sin(angle);

    glm::vec3 r = d * c + glm::cross(k, d) * s + k * glm::dot(k, d) * (1.f - c);
    m_look = glm::normalize(r);

    rebuildView();
}

void Camera::rotateAroundRight(float angle)
{
    glm::vec3 w = -glm::normalize(m_look);
    glm::vec3 u = glm::normalize(glm::cross(m_up, w)); // right axis
    glm::vec3 d = m_look;

    float c = std::cos(angle);
    float s = std::sin(angle);

    glm::vec3 r = d * c + glm::cross(u, d) * s + u * glm::dot(u, d) * (1.f - c);
    m_look = glm::normalize(r);

    // also rotate up so camera stays orthogonal
    glm::vec3 up = m_up;
    glm::vec3 upR = up * c + glm::cross(u, up) * s + u * glm::dot(u, up) * (1.f - c);
    m_up = glm::normalize(upR);

    rebuildView();
}

