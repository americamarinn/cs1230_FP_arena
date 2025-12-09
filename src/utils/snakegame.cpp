#include "utils/snakegame.h"
#include <glm/gtx/transform.hpp>

void SnakeGame::init() {
    m_body.clear();

    // Start with 4 segments
    for (int i = 0; i < 4; i++) {
        m_body.push_back({ glm::vec3(-i, 0, 0) });
    }

    m_direction = glm::vec3(1, 0, 0);
}

void SnakeGame::update(float dt) {
    // Move body from back to front
    for (int i = (int)m_body.size() - 1; i > 0; i--) {
        m_body[i].pos = m_body[i - 1].pos;
    }

    // Move head forward
    m_body[0].pos += m_direction * m_speed * dt;
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
