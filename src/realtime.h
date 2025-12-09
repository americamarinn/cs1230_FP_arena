#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <unordered_map>
#include <vector>

// Utils
#include "utils/camera.h"
#include "utils/gbuffer.h"
#include "utils/shaderloader.h"
#include "utils/snakegame.h"
#include "terraingenerator.h"
#include "utils/cube.h"

enum GameState {
    START_SCREEN,
    PLAYING
};

class Realtime : public QOpenGLWidget {
public:
    Realtime(QWidget* parent = nullptr);
    void finish();
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(const std::string& path);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void timerEvent(QTimerEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

private:
    // --- CORE & INPUT ---
    QElapsedTimer m_elapsedTimer;
    int m_timer;
    bool m_mouseDown = false;
    glm::vec2 m_prevMousePos;
    std::unordered_map<int, bool> m_keyMap;
    float m_devicePixelRatio = 1.0f;

    // Game State
    GameState m_gameState = START_SCREEN;
    GLuint m_startTexture = 0;

    // --- CAMERA ---
    Camera m_camera;
    glm::vec3 m_camPos;
    glm::vec3 m_camLook;
    glm::vec3 m_camUp;
    void updateCamera(float deltaTime);

    // --- DEFERRED RENDERING PIPELINE ---
    GBuffer m_gbuffer;
    GLuint m_defaultFBO = 2; // Handle for the screen

    // Shaders
    GLuint m_gbufferShader   = 0;
    GLuint m_deferredShader  = 0;
    GLuint m_blurShader      = 0;
    GLuint m_compositeShader = 0;

    // Fullscreen Quad
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    // Post-Process FBOs
    GLuint m_lightingFBO = 0;
    GLuint m_lightingTexture = 0;

    GLuint m_pingpongFBO[2] = {0, 0};
    GLuint m_pingpongColorbuffers[2] = {0, 0};

    // --- NEON ARENA DATA ---

    // 1. Props (Walls, Snake Body, Apple)
    struct ArenaProp {
        glm::vec3 pos;
        glm::vec3 scale;
        glm::vec3 color;
        float emissiveStrength; // Glow intensity
    };
    std::vector<ArenaProp> m_props;

    // 2. Lights
    struct SimpleLight {
        glm::vec3 pos;
        glm::vec3 color;
        float radius;
    };
    std::vector<SimpleLight> m_lights;

    // --- RESOURCES ---

    // Reusable Geometry
    GLuint m_cubeVAO = 0;
    GLuint m_cubeVBO = 0;
    int m_cubeNumVerts = 0;

    // Terrain (Legacy/Background)
    TerrainGenerator m_terrainGen;
    GLuint m_terrainVAO = 0;
    GLuint m_terrainVBO = 0;
    int m_terrainNumVerts = 0;

    // Textures
    GLuint m_grassDiffuseTex = 0;

    // Game Logic (Updating in background)
    SnakeGame m_snake;

    // --- HELPERS ---
    void buildNeonScene();
    void initCube();
    void initQuad();
    void initTerrain(); // Kept for compatibility
    GLuint loadTexture2D(const std::string &path);
};

