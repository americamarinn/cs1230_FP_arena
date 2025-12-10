#pragma once

#include <vector>
#include <glm/glm.hpp>

// ✅ This is where RenderShapeData + SceneLightData live
#include "utils/scenedata.h"
#include "utils/sceneparser.h"

// utils/snakegame.h

struct SnakeSegment {
    glm::vec3 pos;
};

struct SnakeState {
    glm::vec3 pos; // head position (center of cube)
    glm::vec3 vel; // velocity
};

class SnakeGame {
public:
    void init();
    void update(float dt);

    void submitGeometry(std::vector<RenderShapeData>& shapes);
    void submitLights(std::vector<SceneLightData>& lights);

private:
    std::vector<SnakeSegment> m_body;
    glm::vec3 m_direction = glm::vec3(1, 0, 0); // still used as input direction
    float m_speed = 3.0f;                        // no longer used directly

    // === NEW: physics-based motion ===
    SnakeState m_snake;

    float m_mass       = 1.0f;
    float m_forceMag   = 70.0f;   // how strong WASD/arrow input feels
    float m_friction   = 8.0f;    // damping
    float m_maxSpeed   = 8.0f;    // clamp speed
};
