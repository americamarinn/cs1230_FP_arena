#include "portal.h"

#include <glm/gtc/matrix_access.hpp>
#include <iostream>

Portal::Portal(const glm::vec3& position,
               const glm::vec3& normal,
               const glm::vec2& size)
    : m_center(position)
    , m_normal(glm::normalize(normal))
    , m_size(size)
    , m_color(1.0f)
    , m_pair(nullptr)
    , m_vbo(0)
    , m_borderVBO(0)
    , m_borderVAO(0)
    , m_vao(0)
    , m_initialized(false)

{
    updateTransform();
    generateQuadGeometry();
    generateBorderGeometry(0.5f);
}

Portal::~Portal() {
    cleanup();
}

void Portal::updateTransform() {

    // build transform matrix using portal position and portal normal
    // portal faces along normal direction

    // need right and up vectors for portal orientation
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    // use a different reference if portal is horizontal
    if (glm::abs(glm::dot(m_normal, worldUp)) > 0.95f) {
        worldUp = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 right = glm::normalize(glm::cross(worldUp, m_normal));
    glm::vec3 up = glm::normalize(glm::cross(m_normal, right));

    // build transformation matrix (column-major, check w camera if any doubts)
    m_model = glm::mat4(1.0f);
    m_model[0] = glm::vec4(right, 0.0f);
    m_model[1] = glm::vec4(up, 0.0f);
    m_model[2] = glm::vec4(m_normal, 0.0f);
    m_model[3] = glm::vec4(m_center, 1.0f);
}

// generates quad with two triangles
void Portal::generateQuadGeometry() {

    // need local space coordinates that are centered at origin
    float hw = m_size.x * 0.5f;
    float hh = m_size.y * 0.5f;

    glm::vec4 lsVerts[] = {
        glm::vec4(-hw, -hh, 0.0f, 1.0f),    // bottom left
        glm::vec4(hw,  -hh, 0.0f, 1.0f),    // bottom right
        glm::vec4(hw,   hh, 0.0f, 1.0f),    // top right
        glm::vec4(-hw,  hh, 0.0f, 1.0f)     // top left
    };

    m_vertices.clear();

    // transform local vertex and push back into m_vertices

    // triangle 1: bottom left, bottom right, top right
    for (int i : {0, 1, 2}) {
        glm::vec4 wsVert = m_model * lsVerts[i];
        addToVector(wsVert, m_vertices);
    }

    // triangle 2: bottom left, top right, top left
    for (int i : {0, 2, 3}) {
        glm::vec4 wsVert = m_model * lsVerts[i];
        addToVector(wsVert, m_vertices);
    }
}

void Portal::generateBorderGeometry(float borderWidth) {

    // create outline around portal

    // inner border
    float hw = m_size.x * 0.5f;
    float hh = m_size.y * 0.5f;

    // outer border
    float ohw = hw + borderWidth;
    float ohh = hh + borderWidth;

    // organize vertices
    glm::vec4 outerVerts[] = {
        glm::vec4(-ohw, -ohh, 0.0f, 1.0f),  // outer bottom left
        glm::vec4(ohw, -ohh, 0.0f, 1.0f),   // outer bottom right
        glm::vec4(ohw, ohh, 0.0f, 1.0f),    // outer top right
        glm::vec4(-ohw, ohh, 0.0f, 1.0f)    // outer top left
    };

    glm::vec4 innerVerts[] = {
        glm::vec4(-hw, -hh, 0.0f, 1.0f),    // inner bottom left
        glm::vec4(hw, -hh, 0.0f, 1.0f),     // inner bottom right
        glm::vec4(hw, hh, 0.0f, 1.0f),      // inner top right
        glm::vec4(-hw, hh, 0.0f, 1.0f)      // inner top left
    };

    m_borderVerts.clear();

    // transform to world space
    for (int i = 0; i < 4; i++) {
        outerVerts[i] = m_model * outerVerts[i];
        innerVerts[i] = m_model * innerVerts[i];
    }

    // make the border segments, 2 triangles per segment

    // bottom border
    addToVector(outerVerts[0], m_borderVerts);      addToVector(outerVerts[1], m_borderVerts);      addToVector(innerVerts[0], m_borderVerts);
    addToVector(innerVerts[0], m_borderVerts);      addToVector(outerVerts[1], m_borderVerts);      addToVector(innerVerts[1], m_borderVerts);

    // right border
    addToVector(innerVerts[1], m_borderVerts);      addToVector(outerVerts[1], m_borderVerts);      addToVector(outerVerts[2], m_borderVerts);
    addToVector(innerVerts[1], m_borderVerts);      addToVector(outerVerts[2], m_borderVerts);      addToVector(innerVerts[2], m_borderVerts);

    // top border
    addToVector(innerVerts[2], m_borderVerts);      addToVector(outerVerts[2], m_borderVerts);      addToVector(outerVerts[3], m_borderVerts);
    addToVector(innerVerts[2], m_borderVerts);      addToVector(outerVerts[3], m_borderVerts);      addToVector(innerVerts[3], m_borderVerts);

    // left border
    addToVector(innerVerts[3], m_borderVerts);      addToVector(outerVerts[3], m_borderVerts);      addToVector(outerVerts[0], m_borderVerts);
    addToVector(innerVerts[3], m_borderVerts);      addToVector(outerVerts[0], m_borderVerts);      addToVector(innerVerts[0], m_borderVerts);
}

void Portal::render(GLuint shader,
                    const glm::mat4& mvp) const {

    // one sided portals :(

    // link portal shader
    glUseProgram(shader);

    // set uniforms
    GLint loc = glGetUniformLocation(shader, "mvp");
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp[0][0]);
    }

    loc = glGetUniformLocation(shader, "portalColor");
    if (loc != -1) {
        glUniform3f(loc, m_color.x, m_color.y, m_color.z);
    }

    // bind and draw
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);   // 2 triangles, 6 vertices

    // unbind and restore defaults
    glBindVertexArray(0);
    glUseProgram(0);
}

void Portal::renderBorder(GLuint shader, const glm::mat4& mvp) const {

    // disable backface culling so we can see border from front and back
    glDisable(GL_CULL_FACE);

    // use shader provided, should be the border shader
    glUseProgram(shader);

    // set uniforms
    GLint loc = glGetUniformLocation(shader, "mvp");
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp[0][0]);
    }

    loc = glGetUniformLocation(shader, "borderColor");
    if (loc != -1) {
        glUniform3f(loc, m_color.x, m_color.y, m_color.z);
    }

    // draw arrays
    glBindVertexArray(m_borderVAO);
    glDrawArrays(GL_TRIANGLES, 0, 24);

    // restore defaults
    glBindVertexArray(0);
    glUseProgram(0);
    glEnable(GL_CULL_FACE);
}


// // utility only needed if using one-sided portals
// void Portal::renderColoredBack(GLuint shader, const glm::mat4& mvp, const glm::vec3& camPos) const {

//     if (!m_initialized) {
//         std::cerr << "error: portal not initialized" << std::endl;
//         return;
//     }

//     // else camera is on side B (back), render colored quad

//     // want to render back side
//     glDisable(GL_CULL_FACE);
//     glDepthMask(GL_FALSE);

//     glUseProgram(shader);

//     // set uniforms
//     GLint loc = glGetUniformLocation(shader, "mvp");
//     if (loc != -1) {
//         glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp[0][0]);
//     }

//     loc = glGetUniformLocation(shader, "borderColor");
//     if (loc != -1) {
//         glUniform3f(loc, m_color.x, m_color.y, m_color.z);
//     }

//     // bind and draw
//     glBindVertexArray(m_vao);
//     glDrawArrays(GL_TRIANGLES, 0, 6);

//     // unbind
//     glBindVertexArray(0);
//     glUseProgram(0);

//     // restore pre-call state
//     glEnable(GL_CULL_FACE);
//     glDepthMask(GL_TRUE);
// }

void Portal::linkPortal(std::shared_ptr<Portal> pair) {
    m_pair = pair;
}

glm::mat4 Portal::calculateViewMatrix(const glm::mat4 &camView) const {

    if (!m_pair) {
        std::cerr << "error: no pair found for portal, returning camera view" << std::endl;
        return camView;
    }

    // from wikibooks tutorial
    // 1. locate camera's position relative to this portal (entrance)
    // 2. place camera at same relative position at paired portal (exit)
    // 3. rotate by 180 so we see the other side of the portal

    // model-view matrix for this portal
    // localEntry -> world -> camera space
    glm::mat4 mv = camView * m_model;


    // portal view matrix, for showing view from portal camera pov
    glm::mat4 portalView =

        // localEntry -> camera
        mv

        // rotate 180 degrees because camera and portal are facing opposite to each other
        * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f))

        // camera -> localExit
        * glm::inverse(m_pair->m_model);


    return portalView;
}

// shoutout eric lengyel from terathon software
// "oblique view frustum depth projection and clipping"
glm::mat4 Portal::getObliqueProjection(const glm::mat4 &viewMatrix,
                                       const glm::mat4 &baseProj) const {

    if (!m_pair) return baseProj;

    // destination portal plane in world space
    const auto dst = m_pair;
    glm::vec3 Nw = dst->m_normal;
    glm::vec3 Pw = dst->m_center;

    // construct plane in camera (portal) space
    glm::vec3 Nc = glm::normalize(glm::mat3(viewMatrix) * Nw);
    glm::vec3 Pc = glm::vec3(viewMatrix * glm::vec4(Pw, 1.0f));

    // exitPlane = (Nx, Ny, Nz, d) where d = -N·P (camera-space)
    float d = -glm::dot(Nc, Pc);
    glm::vec4 exitPlane(Nc, d);

    // ensure plane faces toward the camera (camera at origin in view space).
    // if the signed distance (d == exitPlane.w) is positive, flip the plane.
    if (exitPlane.w > 0.0f) exitPlane = -exitPlane;

    // inverse of projection, clip -> camera
    glm::mat4 invM = glm::inverse(baseProj);

    // eq 7: q' = clip-space corner opposite exitPlane

    // "(For most perspective projections, it is safe to assume that the signs of [exitPlane.x] and [exitPlane.y]
    // in camera space are the same as [exitPlane.x] and [exitPlane.y] in clip space, so the projection of exitPlane
    // into clip space can be avoided.)"

    // transform q' to camera-space
    glm::vec4 q = invM * glm::vec4(glm::sign(exitPlane.x),
                                   glm::sign(exitPlane.y),
                                   1.0f,
                                   1.0f);

    // eq 14: M3' = [(-2 * q.z) / dot(exitPlane, q)] * exitPlane + <0,0,1,0>
    // q.z is always -1 so this simplifies to
    // M3' = a * exitPlane + <0,0,1,0> where a = 2 / dot(exitPlane, q)

    // calculate a first
    float denom = glm::dot(exitPlane, q);
    const float epsilon = 1e-6f;
    if (glm::abs(denom) < epsilon) {
        // fallback to baseProj
        return baseProj;
    }
    float a = 2.0f / denom;

    // eq 10: M3' = a * exitPlane - M4 (fourth row in baseProj = <0,0,1,0>)
    glm::vec4 M4 = glm::row(baseProj, 3);

    glm::vec4 newRow = a * exitPlane - M4;

    // replace third row of base projection to adjust near clipping plane
    glm::mat4 oblique = baseProj;
    oblique = glm::row(oblique, 2, newRow);

    return oblique;
}

void Portal::initialize() {
    if (m_initialized) return;

    // setup portal quad
    glGenBuffers(1, &m_vbo);
    glGenVertexArrays(1, &m_vao);

    // bind vbo and populate
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 m_vertices.size() * sizeof(float),
                 m_vertices.data(),
                 GL_STATIC_DRAW);

    // bind vao
    glBindVertexArray(m_vao);

    // position is the only attribute to enable
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          3 * sizeof(float),
                          reinterpret_cast<void*>(0));

    // portal border geometry
    glGenBuffers(1, &m_borderVBO);
    glGenVertexArrays(1, &m_borderVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_borderVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_borderVerts.size() * sizeof(float),
                 m_borderVerts.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(m_borderVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          3 * sizeof(float),
                          reinterpret_cast<void*>(0));

    // unbind vao and vbo
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // set to true
    m_initialized = true;
}

void Portal::cleanup() {
    if (m_initialized) {
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_borderVBO);
        glDeleteVertexArrays(1, &m_vao);
        glDeleteVertexArrays(1, &m_borderVAO);
        m_vao = 0;
        m_vbo = 0;
        m_borderVAO = 0;
        m_borderVBO = 0;
        m_initialized = false;
    }
}

bool Portal::intersectsLine(const glm::vec3 &lineStart,
                            const glm::vec3 &lineEnd) const {

    // test intersection with both triangles of quad

    // get quad corners in world space
    float hw = m_size.x * 0.5f;
    float hh = m_size.y * 0.5f;

    glm::vec4 lsVerts[] = {
        glm::vec4(-hw, -hh, 0.0f, 1.0f),  // p0: bottom left
        glm::vec4( hw, -hh, 0.0f, 1.0f),  // p1: bottom right
        glm::vec4( hw,  hh, 0.0f, 1.0f),  // p2: top right
        glm::vec4(-hw,  hh, 0.0f, 1.0f),  // p3: top left
    };

    glm::vec3 p0 = glm::vec3(m_model * lsVerts[0]);
    glm::vec3 p1 = glm::vec3(m_model * lsVerts[1]);
    glm::vec3 p2 = glm::vec3(m_model * lsVerts[2]);
    glm::vec3 p3 = glm::vec3(m_model * lsVerts[3]);

    // triangle 1: bottom left, bottom right, top right
    if (intersectsTriangle(lineStart, lineEnd, p0, p1, p2)) {
        return true;
    }

    // triangle 2: bottom left, top right, top left
    if (intersectsTriangle(lineStart, lineEnd, p0, p2, p3)) {
        return true;
    }

    return false;
}

bool Portal::intersectsTriangle(const glm::vec3 &la,
                                const glm::vec3 &lb,
                                const glm::vec3 &p0,
                                const glm::vec3 &p1,
                                const glm::vec3 &p2) const {
    // parametric line-plane intersection

    // check if line has length
    if (glm::length(lb - la) < 1e-6f) {
        return false;
    }

    // build matrix for intersection equation
    // [t, u, v]  = M^-1 * (la - p0)
    // M = [la - lb, p1 - p0, p2 - p0]

    glm::mat3 M (
        glm::vec3(la.x - lb.x, la.y - lb.y, la.z - lb.z),
        glm::vec3(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z),
        glm::vec3(p2.x - p0.x, p2.y - p0.y, p2.z - p0.z)
        );

    // check if M is invertible (non-zero determinant)
    float det = glm::determinant(M);
    if (glm::abs(det) < 1e-6f ) {
        return false; // line parallel to triangle plane
    }

    glm::vec3 tuv = glm::inverse(M) * glm::vec3(la.x - p0.x, la.y - p0.y, la.z - p0.z);

    float t = tuv.x;    // parameter along line (0 = la, 1 = lb)
    float u = tuv.y;    // barycentric coordinate 1
    float v = tuv.z;    // barycentric coordinate 2

    // safety comparison value
    const float eps = 1e-6f;

    // check if intersection is within segment
    if ((t < 0.0f - eps) || (t > 1.0f + eps)) {
        return false;
    }

    // check if intersection is inside triangle
    // valid if u >= 0, v >= 0, u + v <= 1
    if ((u < 0.0f - eps) || (v < 0.0f - eps) || ((u + v) > 1.0f + eps)) {
        return false;
    }

    return true;
}

float Portal::signedDistanceToPortalPlane(const glm::vec3 &point) const {

    // from plane equation: n * p = d
    // positive if point is in front, negative if behind
    return glm::dot(m_normal, point - m_center);
}

