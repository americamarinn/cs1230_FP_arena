#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <memory>
#include <vector>

/**
 * portal - transformed view from other side of portal
 *
 * features:
 * - paired portal system with linked destinations
 * - stencil buffer-based rendering for perfect clipping
 * - oblique near-plane projection to prevent artifacts
 * - collision detection for player teleportation
 * - recursive portal rendering (portals viewing portals)
 */
class Portal {
public:

    /**
     * construct a portal w a given position and orientation
     *
     * position - 3D world space position of portal
     * normal - 3D direction the portal faces (to be normalized)
     * size - 2D vector of portal dimensions -> (width, height)
     */
    Portal(const glm::vec3& position,
           const glm::vec3& normal,
           const glm::vec2& size);

    ~Portal();

    // link portal with its pair, done before rendering
    void linkPortal(std::shared_ptr<Portal> pair);


    // calculate and return view matrix for looking through the portal
    glm::mat4 calculateViewMatrix(const glm::mat4& camView) const;


    // makes sure no objects behind portal get rendered
    // returns copy of the input modified projection matrix w near plane aligned w portal
    // view matrix either from another portal or just the camera
    glm::mat4 getObliqueProjection(const glm::mat4& viewMatrix,
                                   const glm::mat4& baseProj) const;


    // for teleportation, tests if line segment (WS) intersects this portal
    // returns true if given line crosses portal plane
    bool intersectsLine(const glm::vec3& lineStart, const glm::vec3& lineEnd) const;


    // for testing if a point has crossed through the portal
    // for robust than intersectsLine as it checks which side of portal
    // returns a +float if in front,  -float if behind, or ~0 if on plane
    float signedDistanceToPortalPlane(const glm::vec3& point) const;


    // set up vbo and vba, assumes openGL context exists
    void initialize();


    // renders the other side of the portal quad for a given mvp matrix
    // marks the stencil buffer and depth buffer for a given shader
    void render(GLuint shader, const glm::mat4& mvp) const;


    // cleanup openGL stuff
    void cleanup();


    // getters
    glm::vec3 getPosition() const { return m_center; }
    glm::vec3 getNormal() const { return m_normal; }
    glm::vec2 getSize() const { return m_size; }
    glm::mat4 getTransform() const { return m_model; }
    std::shared_ptr<Portal> getLinkedPortal() const { return m_pair; }
    bool isPaired() const { return m_pair != nullptr; }

    // for border
    void setColor(const glm::vec3& color) { m_color = color; }
    glm::vec3 getColor() const { return m_color; }
    void renderBorder(GLuint shader, const glm::mat4& mvp) const;
    void renderColoredBack(GLuint shader,
                           const glm::mat4& mvp,
                           const glm::vec3& camPos) const;


private:
    // portal properties
    glm::vec3 m_center;      // 3D ws position of portal center
    glm::vec3 m_normal;      // portal plane normal, denotes side A
    glm::vec2 m_size;        // (width, height)
    glm::mat4 m_model;       // portal model matrix, local -> world
    glm::vec3 m_color;

    // portal's pair
    std::shared_ptr<Portal> m_pair;

    // openGL stuff
    GLuint m_vbo, m_borderVBO;
    GLuint m_vao, m_borderVAO;
    bool m_initialized;

    // geometry
    std::vector<float> m_vertices;      // portal quad vertices (position only)
    std::vector<float> m_borderVerts;   // portal border vertices

    // update portal view transform matrix
    void updateTransform();

    // populates vertex vector
    void generateQuadGeometry();

    // populates borer vertex vector
    void generateBorderGeometry(float borderWidth);

    // vector population helper
    void addToVector(glm::vec4& vertAttrib, std::vector<float>& vertList) {
        vertList.push_back(vertAttrib.x);
        vertList.push_back(vertAttrib.y);
        vertList.push_back(vertAttrib.z);
    }
    \
        // helper to test line-trianlge intersection
        // ripped from wikibooks tutorial
        bool intersectsTriangle(const glm::vec3& la, const glm::vec3& lb,
                           const glm::vec3& p0, const glm::vec3& p1,
                           const glm::vec3& p2) const;
};
