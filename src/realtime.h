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
#include <deque>

// Utils
#include "utils/camera.h"
#include "utils/gbuffer.h"
#include "utils/shaderloader.h"
#include "utils/snakegame.h"
#include "terraingenerator.h"
#include "utils/cube.h"
#include "utils/sphere.h"

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
    void keyReleaseEvent(QKeyEvent*) override;
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
        GLuint textureID; // 5th Argument
    };
    std::vector<ArenaProp> m_props;

    struct SimpleLight {
        glm::vec3 pos;
        glm::vec3 vel;    // 2nd Argument (Physics)
        glm::vec3 color;
        float radius;
    };
    std::vector<SimpleLight> m_lights;

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
    GLuint m_wallTexture = 0;

    SnakeGame m_snake;

    // --- SIMPLE SNAKE (STEP 1: HEAD ONLY) ---
    struct SnakeState {
        glm::vec3 pos; // center of cube
        glm::vec3 vel;
    };

    SnakeState m_snakeState;

    // Body segments
    std::vector<glm::vec3> m_snakeBody;

    // High-res trail for body following
    std::deque<glm::vec3> m_snakeTrail;
    glm::vec3 m_lastTrailPos = glm::vec3(0.f);
    float     m_trailAccumDist = 0.f;
    float     m_trailSampleDist = 0.4f;   // spacing along trail

    glm::vec3 m_snakeForceDir = glm::vec3(0.f);

    float m_snakeMass       = 1.0f;
    float m_snakeForceMag   = 70.0f;  // input strength
    float m_snakeFriction   = 8.0f;   // damping
    float m_snakeMaxSpeed   = 8.0f;   // clamp

    // jump
    float m_snakeJumpOffset  = 0.0f;
    float m_snakeJumpVel     = 0.0f;
    float m_snakeJumpImpulse = 7.0f;
    float m_snakeGravity     = 20.0f;
    bool  m_snakeOnGround    = true;

    void updateSnakeForceDirFromKeys();

    // --- Food / powerup types ---
    enum FoodType {
        FOOD_NORMAL = 0,
        FOOD_SPEED  = 1,
        FOOD_JUMP   = 2
    };

    // --- FOOD ---
    glm::vec3 m_foodPos      = glm::vec3(0.f);
    bool      m_hasFood      = false;
    float     m_foodRadius   = 1.2f;   // collision radius

    FoodType  m_foodType   = FOOD_NORMAL;

    // --- Power-up state ---
    bool  m_speedBoostActive = false;
    float m_speedBoostTimer  = 0.f;
    float m_speedBoostDuration = 7.0f;   // seconds

    bool  m_jumpBoostActive  = false;
    float m_jumpBoostTimer   = 0.f;
    float m_jumpBoostDuration = 7.0f;    // seconds

    // --- FOOD MESH (sphere) ---
    GLuint m_sphereVAO       = 0;
    GLuint m_sphereVBO       = 0;
    int    m_sphereNumVerts  = 0;

    struct Laser {
        glm::vec3 center;    // base center position
        glm::vec3 axis;      // (1,0,0) for X-aligned, (0,0,1) for Z-aligned
        float length;
        float thickness;
        float speed;         // how fast it sweeps
        float phase;         // per-laser offset
        glm::vec3 color;     // bright "volt" colour
        float emissive;      // emissive strength
    };

    // Cloth ghost settings
    static const int GHOST_W = 18;   // wider
    static const int GHOST_H = 6;    // grid height

    struct GhostParticle {
        glm::vec3 pos;
        glm::vec3 vel;
        bool pinned;
    };


    float m_ghostRestLenX = 0.45f;   // spacing left–right
    float m_ghostRestLenZ = 0.28f;   // spacing front–back



    glm::vec3 m_ghostOffset = glm::vec3(0.0f, 1.2f, 0.0f); // tweak 1.0–1.4 to taste
    float     m_ghostRadius = 0.7f;                        // invisible head sphere



    float m_ghostTime = 0.0f; // for wind

    GhostParticle m_ghost[GHOST_W * GHOST_H];

    float m_ghostRestX;      // rest length horizontally
    float m_ghostRestZ;      // rest length vertically
    float m_ghostK;          // spring stiffness
    float m_ghostDamping;    // velocity damping
    float m_ghostMass;       // mass per particle

    // helper
    int ghostIndex(int x, int y) const;
    void initGhostCloth(const glm::vec3 &bossPos);
    bool isGhostPinned(int x, int y) const;
    void updateGhostCloth(float dt, const glm::vec3 &bossPos, const glm::vec3 &bossVel);
    void drawGhostCloth();   // called from paintGL()
    //END OF GHOST STUFF

    // ---- Cloth ghost rendering ----
    float m_bossPulseTime = 0.f;
    GLuint m_ghostVAO = 0;
    GLuint m_ghostVBO = 0;
    int    m_ghostNumVerts = 0;

    void initGhostBuffers();




    // --- HELPERS ---
    void buildNeonScene();
    void updateLightPhysics();
    void drawVoxelText(glm::vec3 startPos, std::string text, glm::vec3 color, float scale, GLuint texID);

    void initCube();
    void initQuad();
    void initTerrain();

    void initSphere();
    void resetSnake();

    // --- Snake death / squish animation ---
    bool  m_snakeDead      = false;
    float m_deathTimer     = 0.0f;
    float m_deathDuration  = 0.25f; // length of squish animation (seconds)

    // Round timer -> when this hits 0, boss wakes up
    float m_roundTime   = 20.0f;  // total seconds per round
    float m_timeLeft    = 20.0f;

    // Boss (chaser) data
    bool        m_bossActive   = false;
    glm::vec3   m_bossPos      = glm::vec3(0.f);
    glm::vec3   m_bossVel      = glm::vec3(0.f);
    float       m_bossSpeed    = 9.0f;   // slightly faster than snake max
    float       m_bossHitRadius = 1.8f;   // collision radius with snake head


    void startSnakeDeath();
    bool snakeHeadHitsWall() const;

    void spawnFood();
    GLuint loadTexture2D(const std::string &path);
};
