#include "utils/snakegame.h"
#include <glm/gtx/transform.hpp>

void SnakeGame::init() {
    m_body.clear();

    // Start with 4 segments like before
    for (int i = 0; i < 4; i++) {
        SnakeSegment seg;
        seg.pos = glm::vec3(-i, 0.5f, 0.0f); // sit slightly above ground
        m_body.push_back(seg);
    }

    // Physics head state
    m_snake.pos = m_body[0].pos;
    m_snake.vel = glm::vec3(0.0f);

    // Direction the Realtime code will keep writing into
    m_direction = glm::vec3(1, 0, 0);

    // Physics params (feel free to tweak later)
    m_mass     = 1.0f;
    m_forceMag = 70.0f;
    m_friction = 8.0f;
    m_maxSpeed = 8.0f;
}

void SnakeGame::update(float dt) {
    // --- 1) Integrate physics for the head ---

    // Treat m_direction as desired movement direction (set from Realtime)
    glm::vec3 inputDir = m_direction;
    if (glm::length(inputDir) > 0.0f) {
        inputDir = glm::normalize(inputDir);
    }

    // Forces
    glm::vec3 F_input = inputDir * m_forceMag;
    glm::vec3 F_fric  = -m_snake.vel * m_friction;
    glm::vec3 F_total = F_input + F_fric;

    // Acceleration and velocity
    glm::vec3 a = F_total / m_mass;
    m_snake.vel += a * dt;

    // Clamp max speed
    float speed = glm::length(m_snake.vel);
    if (speed > m_maxSpeed && speed > 0.0f) {
        m_snake.vel = (m_snake.vel / speed) * m_maxSpeed;
    }

    // Integrate position
    m_snake.pos += m_snake.vel * dt;

    // Keep snake on the ground plane (y constant)
    m_snake.pos.y = 0.5f;

    // --- 2) Update body segments like before ---

    if (!m_body.empty()) {
        // shift body from back to front
        for (int i = static_cast<int>(m_body.size()) - 1; i > 0; --i) {
            m_body[i].pos = m_body[i - 1].pos;
        }

        // head segment follows physics head
        m_body[0].pos = m_snake.pos;
    }
}

void SnakeGame::submitGeometry(std::vector<RenderShapeData>& shapes) {
    for (int i = 0; i < (int)m_body.size(); i++) {
        RenderShapeData s{};
        s.ctm = glm::translate(m_body[i].pos);
        s.primitive.type = PrimitiveType::PRIMITIVE_CUBE;

        // Dark base color so glow stands out
        s.primitive.material.cDiffuse  = glm::vec4(0.05f, 0.05f, 0.05f, 1);

        // Head brighter than body
        if (i == 0)
            s.primitive.material.cEmissive = glm::vec4(1.0f, 1.0f, 0.2f, 1);
        else
            s.primitive.material.cEmissive = glm::vec4(0.0f, 1.0f, 1.0f, 1);

        shapes.push_back(s);
    }
}

void SnakeGame::submitLights(std::vector<SceneLightData>& lights) {
    for (int i = 0; i < (int)m_body.size(); i++) {
        SceneLightData L{};
        L.type = LightType::LIGHT_POINT;
        L.pos  = glm::vec4(m_body[i].pos, 1);

        L.color = (i == 0)
                      ? glm::vec4(1, 1, 0.4f, 1)
                      : glm::vec4(0, 1, 1, 1);

        // Neon falloff
        L.function = glm::vec3(1.0f, 0.14f, 0.07f);

        lights.push_back(L);
    }
}
