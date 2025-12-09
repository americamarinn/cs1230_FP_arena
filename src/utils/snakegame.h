#pragma once

#include <vector>
#include <glm/glm.hpp>

// ✅ This is where RenderShapeData + SceneLightData live
#include "utils/scenedata.h"
#include "utils/sceneparser.h"

struct SnakeSegment {
    glm::vec3 pos;
};

class SnakeGame {
public:
    void init();
    void update(float dt);

    void submitGeometry(std::vector<RenderShapeData>& shapes);
    void submitLights(std::vector<SceneLightData>& lights);

private:
    std::vector<SnakeSegment> m_body;
    glm::vec3 m_direction = glm::vec3(1, 0, 0);
    float m_speed = 3.0f;
};
