


#pragma once
#include <vector>
#include <glm/glm.hpp>

class Cone {
public:
    void updateParams(int param1, int param2);
    std::vector<float> generateShape() { return m_vertexData; }

private:
    void insertVec3(std::vector<float> &data, glm::vec3 v);
    void setVertexData();

    void makeCapSlice(float theta0, float theta1);
    void makeSlopeSlice(float theta0, float theta1);
    void makeWedge(float theta0, float theta1);

    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;
    float m_radius = 0.5f;
};
