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
#include <string>

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
    GLuint m_defaultFBO = 2;

    GLuint m_gbufferShader   = 0;
    GLuint m_deferredShader  = 0;
    GLuint m_blurShader      = 0;
    GLuint m_compositeShader = 0;

    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    GLuint m_lightingFBO = 0;
    GLuint m_lightingTexture = 0;
    GLuint m_pingpongFBO[2] = {0, 0};
    GLuint m_pingpongColorbuffers[2] = {0, 0};

    // --- NEON ARENA DATA ---
    struct ArenaProp {
        glm::vec3 pos;
        glm::vec3 scale;
        glm::vec3 color;
        float emissiveStrength;
        GLuint textureID; // <--- NEW: Stores which texture to use (0 = None)
    };
    std::vector<ArenaProp> m_props;

    // Lights
    struct SimpleLight {
        glm::vec3 pos;
        glm::vec3 vel;
        glm::vec3 color;
        float radius;
    };
    std::vector<SimpleLight> m_lights;

    // Collision Data
    std::vector<std::vector<int>> m_mazeGrid;
    const int GRID_SIZE = 60;
    const float GRID_SCALE = 1.0f;

    // --- RESOURCES ---
    GLuint m_cubeVAO = 0;
    GLuint m_cubeVBO = 0;
    int m_cubeNumVerts = 0;

    TerrainGenerator m_terrainGen;
    GLuint m_terrainVAO = 0;
    GLuint m_terrainVBO = 0;
    int m_terrainNumVerts = 0;

    GLuint m_grassDiffuseTex = 0;
    GLuint m_wallTexture = 0; // <--- NEW: Handle for the wall texture

    SnakeGame m_snake;

    // --- HELPERS ---
    void buildNeonScene();
    void updateLightPhysics();
    void drawVoxelText(glm::vec3 startPos, std::string text, glm::vec3 color, float scale, GLuint texID); // Updated Arg

    void initCube();
    void initQuad();
    void initTerrain();
    GLuint loadTexture2D(const std::string &path);
};
