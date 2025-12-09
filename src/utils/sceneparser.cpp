#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>





//I USED THESE IN LAB 4!SO THEYRE OKAY
static glm::mat4 toMat(const SceneTransformation* t) {
    switch (t->type) {
    case TransformationType::TRANSFORMATION_TRANSLATE:
        return glm::translate(t->translate);

    case TransformationType::TRANSFORMATION_SCALE:
        return glm::scale(t->scale);

    case TransformationType::TRANSFORMATION_ROTATE:
        // note: angle is in RADIANS already
        return glm::rotate(t->angle, t->rotate);

    case TransformationType::TRANSFORMATION_MATRIX:
        return t->matrix;
    }

    // Fallback (should never hit)
    return glm::mat4(1.f);
}


static SceneLightData makeLight(const SceneLight* L, const glm::mat4 &CTM) {
    SceneLightData out{};
    out.id       = L->id;
    out.type     = L->type;
    out.color    = L->color;
    out.function = L->function;
    out.penumbra = L->penumbra;
    out.angle    = L->angle;
    out.width    = L->width;
    out.height   = L->height;

    // Position (for point + spot lights)
    if (L->type == LightType::LIGHT_POINT || L->type == LightType::LIGHT_SPOT) {
        out.pos = CTM * glm::vec4(0.f, 0.f, 0.f, 1.f);
    }

    // Direction (for directional + spot lights)
    if (L->type == LightType::LIGHT_DIRECTIONAL || L->type == LightType::LIGHT_SPOT) {
        glm::vec4 dLocal = glm::vec4(glm::vec3(L->dir), 0.f); // direction = w = 0
        glm::vec4 dWorld = CTM * dLocal;
        out.dir = glm::vec4(glm::normalize(glm::vec3(dWorld)), 0.f);
    }

    return out;
}


static void traverse(const SceneNode* node,
                     const glm::mat4 &parentCTM,
                     RenderData &out)
{
    // accumulate CTM
    glm::mat4 M = parentCTM;
    for (const SceneTransformation* t : node->transformations) {
        M = M * toMat(t);
    }

    // primitives
    for (const ScenePrimitive* p : node->primitives) {
        RenderShapeData rs{};
        rs.primitive = *p;   // copy primitive data
        rs.ctm       = M;    // cumulative transform
        out.shapes.push_back(rs);
    }

    // lights
    for (const SceneLight* L : node->lights) {
        out.lights.push_back(makeLight(L, M));
    }

    // recurse on children
    for (const SceneNode* child : node->children) {
        traverse(child, M, out);
    }
}



bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader(filepath);
    if (!fileReader.readJSON()) {
        std::cerr << "Failed to read scene file: " << filepath << std::endl;
        return false;
    }

    // 1) Global + camera data
    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();

    // 2) clear previous shapes/lights
    renderData.shapes.clear();
    renderData.lights.clear();

    // 3) traverse scene graph starting at root
    const SceneNode* root = fileReader.getRootNode();
    if (root != nullptr) {
        traverse(root, glm::mat4(1.f), renderData);
    }

    // for debug:
    std::cout << "[SceneParser] Parsed scene \""
              << filepath << "\"\n"
              << "  shapes = " << renderData.shapes.size() << "\n"
              << "  lights = " << renderData.lights.size() << std::endl;

    return true;
}
