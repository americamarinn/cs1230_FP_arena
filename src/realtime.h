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

// #pragma once

// #ifdef __APPLE__
// #define GL_SILENCE_DEPRECATION
// #endif

// #include <GL/glew.h>
// #include <QOpenGLWidget>
// #include <QElapsedTimer>
// #include <unordered_map>

// #include "utils/sceneparser.h"
// #include "utils/camera.h"
// #include "utils/gbuffer.h"
// #include "utils/snakegame.h"
// #include "utils/shaderloader.h"

// class Realtime : public QOpenGLWidget {
// public:
//     Realtime(QWidget* parent = nullptr);
//     void finish();
//     void sceneChanged();

// protected:
//     void initializeGL() override;
//     void paintGL() override;
//     void resizeGL(int w, int h) override;
//     void timerEvent(QTimerEvent*) override;

//     void keyPressEvent(QKeyEvent*) override;
//     void keyReleaseEvent(QKeyEvent*) override;
//     void mouseMoveEvent(QMouseEvent*) override;

// private:
//     // ---------------- RENDERING ----------------
//     GBuffer m_gbuffer;

//     GLuint m_gbufferShader   = 0;
//     GLuint m_deferredShader  = 0;
//     GLuint m_compositeShader = 0;
//     GLuint m_fullscreenVAO   = 0;

//     std::unordered_map<int, GLuint> m_shapeVAOs;
//     std::unordered_map<int, int>    m_shapeVertexCounts;

//     // ---------------- SCENE ----------------
//     RenderData m_renderData;

//     // ---------------- CAMERA ----------------
//     Camera m_camera;

//     // ---------------- GAMEPLAY ----------------
//     SnakeGame m_snake;

//     // ---------------- TIMING ----------------
//     QElapsedTimer m_elapsedTimer;
// };


// #pragma once

// // Defined before including GLEW to suppress deprecation messages on macOS
// #ifdef __APPLE__
// #define GL_SILENCE_DEPRECATION
// #endif

// #include <GL/glew.h>
// #include <glm/glm.hpp>

// #include <vector>
// #include <unordered_map>

// #include <QElapsedTimer>
// #include <QOpenGLWidget>
// #include <QTime>
// #include <QTimer>

// #include <algorithm>
// #include <string>

// #include "cube.h"
// #include "cone.h"
// #include "sphere.h"
// #include "cylinder.h"
// #include "shaderloader.h"
// #include "camera.h"
// #include "scenedata.h"
// #include "sceneparser.h"
// #include "terraingenerator.h"
// #include <QImage>
// #include <deque>
// #include <QDebug>

// class Realtime : public QOpenGLWidget
// {
//     Q_OBJECT
//     bool m_useNormalMap = true;
// public:
//     Realtime(QWidget *parent = nullptr);
//     void finish();                                      // Called on program exit
//     void sceneChanged();
//     void settingsChanged();
//     void saveViewportImage(std::string filePath);


// protected:
//     void initializeGL() override;                       // Called once at the start of the program
//     void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
//     void resizeGL(int width, int height) override;      // Called when window size changes

// private:
//     // Input events
//     void keyPressEvent(QKeyEvent *event) override;
//     void keyReleaseEvent(QKeyEvent *event) override;
//     void mousePressEvent(QMouseEvent *event) override;
//     void mouseReleaseEvent(QMouseEvent *event) override;
//     void mouseMoveEvent(QMouseEvent *event) override;
//     void timerEvent(QTimerEvent *event) override;

//     // ========== Tick / input state ==========
//     int m_timer;                                        // ~60 Hz timer
//     QElapsedTimer m_elapsedTimer;                       // time between frames

//     bool m_mouseDown = false;
//     glm::vec2 m_prev_mouse_pos;
//     std::unordered_map<Qt::Key, bool> m_keyMap;

//     double m_devicePixelRatio = 1.0;

//     // ========== Shaders ==========
//     GLuint m_shader = 0;

//     // ========== Shape VAOs from scenefile (if used) ==========
//     struct ShapeVAO {
//         GLuint vao = 0;
//         GLuint vbo = 0;
//         int    vertexCount = 0;
//     };

//     std::vector<ShapeVAO> m_shapeVAOs;

//     void loadScene();
//     void generateShapeVAOs();
//     void cleanupVAOs();
//     std::vector<float> generateShapeData(PrimitiveType type, int param1, int param2);

//     // ========== Camera ==========
//     Camera    m_camera;
//     glm::vec3 m_camPos  = glm::vec3(0.f, 0.f, 5.f);
//     glm::vec3 m_camLook = glm::vec3(0.f, 0.f, -1.f);
//     glm::vec3 m_camUp   = glm::vec3(0.f, 1.f, 0.f);

//     // ========== Terrain ==========
//     TerrainGenerator m_terrainGenerator;
//     GLuint m_terrainVAO        = 0;
//     GLuint m_terrainVBO        = 0;
//     int    m_terrainVertexCount = 0;

//     void generateTerrain();
//     void cleanupTerrain();

//     // ========== Cube mesh re-used for walls/snake/etc. ==========
//     enum MaterialType {
//         MAT_DEFAULT = 0,
//         MAT_PATH    = 1,
//         MAT_WALL    = 2
//     };

//     struct CubeInstance {
//         glm::vec3 pos;
//         glm::vec3 scale;
//         glm::vec3 color;
//         int material = MAT_DEFAULT; // safe now, enum is above
//     };

//     GLuint m_cubeVAO        = 0;
//     GLuint m_cubeVBO        = 0;
//     int    m_cubeVertexCount = 0;

//     std::vector<CubeInstance> m_cubes;   // arena walls, props, etc.
//     bool cellBlocked(int gx, int gz) const;

//     // collision helpers
//     bool blockedAt(const glm::vec3 &p) const;
//     void openFrontDoor();

//     // door timer
//     bool  m_doorOpened    = false;
//     float m_doorTimer     = 0.f;
//     float m_doorOpenDelay = 20.f;  // seconds until door opens

//     bool  m_pathMode      = false; // later: when snake actually leaves arena

//     // Path geometry (Minecraft-y strip)
//     int   m_pathWidth    = 3;   // tiles wide (roughly -1..+1 in x)
//     int   m_pathLengthZ  = 40;  // how far it extends in -Z
//     int   m_pathStartZ   = -10; // first z row for the path, just outside z = -10 wall

//     // Camera follow
//     bool      m_followSnake      = true;
//     glm::vec3 m_camOffsetFromSnake;




//     GLuint m_wallDiffuseTex = 0;
//     GLuint m_wallNormalTex  = 0;

//     // --- Path normal-mapped brick textures ---
//     GLuint m_pathDiffuseTex = 0;
//     GLuint m_pathNormalTex  = 0;
//     float  m_pathUVScale    = 0.4f; // how “zoomed” the bricks are

//     GLuint loadTexture2D(const QString &path);


//     // --- Dead Snaek functionality ---
//     bool  m_snakeDead      = false;
//     float m_snakeDeathTime = 0.f;   // seconds since death
//     float m_snakeStartY    = 0.5f;  // baseline height


//     //L-system for flowers
//     void generateLSystemFoliageStrip(int zStart, int zEnd, bool leftSide);
//     void addLSystemPlant(float baseX, float baseZ);                // existing
//     void addLSystemPlantCustom(float baseX, float baseZ,
//                                int iterations,
//                                float segH,
//                                float horizStep);
//     void buildLSystemTestScene(bool singleTall);
//     void buildLSystemOnlyScene();      // 3 trees (old behavior)
//     void buildLSystemTallTreeScene();  // 1 tall tree


//     // ----- Game mode -----
//     enum class GameMode { Arena, Path };
//     GameMode m_mode = GameMode::Arena;

//     void buildInitialPathStrip();   // builds the Minecraft-style path

//     void generateCubeMesh();
//     void cleanupCubeMesh();
//     void buildArenaLayout();

//     // ========== Snake (single rigid body cube) ==========
//     struct SnakeState {
//         glm::vec3 pos;   // world-space center
//         glm::vec3 vel;   // velocity (units / second)
//     };

//     // Simple snake body: positions of trailing cubes
//     std::vector<glm::vec3> m_snakeBody;

//     // High-resolution trail of head positions so body can follow
//     std::deque<glm::vec3> m_snakeTrail;
//     glm::vec3 m_lastTrailPos;
//     float m_trailAccumDist  = 0.f;
//     float m_trailSampleDist = 0.4f; // distance between trail samples

//     // Food

//     // Food
//     void resetSnake();



//     void spawnFood();
//     glm::vec3 m_foodPos;
//     bool      m_hasFood = false;
//     float     m_foodRadius = 0.6f; // collision radius

//     // --- Grass bump-mapped terrain textures ---
//     GLuint m_grassDiffuseTex = 0;
//     GLuint m_grassHeightTex  = 0;   // height / bump map
//     float  m_grassUVScale    = 0.35f; // tiling amount
//     float  m_grassBumpScale  = 10.0f; // how strong the bumps look



//     SnakeState m_snake;
//     float      m_snakeSpeed = 6.f;

//     // --- simple physics parameters for rigid-body translation ---
//     glm::vec3  m_snakeForceDir = glm::vec3(0.f); // direction from input
//     float      m_snakeMass      = 1.0f;
//     float      m_snakeForceMag  = 70.0f;         // how “strong” WASD is
//     float      m_snakeFriction  = 8.0f;          // velocity damping
//     float      m_snakeMaxSpeed  = 8.0f;

//     // Parsed scene data from lab 4 (still here if you ever load JSON)
//     RenderData m_renderData;


//     //FOR TESTING:
//     void buildNormalMapTestScene();   // big brick cube in front of camera
//     void rebuildMainArenaScene();     // back to normal game
//     void buildLSystemTallWideTreeScene();
//     void buildGrassBumpTestScene();


//     bool m_enablePathNormalMap = true;
// };

