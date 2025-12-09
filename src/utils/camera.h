#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera();

    // Set pose from position, look, up (all in world space)
    void setViewMatrix(const glm::vec3 &pos,
                       const glm::vec3 &look,
                       const glm::vec3 &up);


    void setProjectionMatrix(float aspectRatio,
                             float nearPlane,
                             float farPlane,
                             float heightAngle);

    // Getters
    const glm::mat4 &getViewMatrix()  const { return m_view; }
    const glm::mat4 &getProjMatrix()  const { return m_proj; }
    glm::vec3        getPosition()    const { return m_pos;  }
    glm::vec3        getLook()        const { return m_look; }


    // Movement hooks
    void translate(const glm::vec3 &delta);
    void rotateAroundUp(float angle);    // yaw
    void rotateAroundRight(float angle); // pitch

    glm::mat4 getProjectionMatrix() const { return m_projMatrix; }





private:
    // Camera pose
    glm::vec3 m_pos{0.f, 0.f, 3.f};
    glm::vec3 m_look{0.f, 0.f, -1.f};
    glm::vec3 m_up{0.f, 1.f, 0.f};

    // Matrices
    glm::mat4 m_view{1.f};
    glm::mat4 m_proj{1.f};

    glm::mat4 m_projMatrix {1.0f};
    float m_nearPlane = 0.1f;
    float m_farPlane  = 10.f;

    // Stored projection params (for updates)
    float m_aspect   = 1.f;
    float m_near     = 0.1f;
    float m_far      = 10.f;
    float m_fovy     = 0.785398f; //45° in radians

    float getHeightAngle() const {
        return m_fovy;
    }

    void rebuildView();
};
