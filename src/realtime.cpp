#include "realtime.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "utils/debug.h"
#include <cstdlib>
#include "utils/sphere.h"

void checkFramebufferStatus() {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "!!! FBO ERROR: 0x" << std::hex << status << std::dec << std::endl;
    }
}

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent), m_mouseDown(false),
    m_camPos(0.f, 50.f, 60.f),
    m_camLook(0.f, -0.6f, -0.8f),
    m_camUp(0.f, 1.f, 0.f),
    m_gameState(START_SCREEN)
{
    setFocusPolicy(Qt::StrongFocus);
}

void Realtime::finish() {
    makeCurrent();
    glDeleteVertexArrays(1, &m_cubeVAO);
    glDeleteBuffers(1, &m_cubeVBO);
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers(1, &m_quadVBO);
    glDeleteProgram(m_gbufferShader);
    glDeleteProgram(m_deferredShader);
    glDeleteProgram(m_blurShader);
    glDeleteProgram(m_compositeShader);
    glDeleteFramebuffers(1, &m_lightingFBO);
    glDeleteTextures(1, &m_lightingTexture);
    glDeleteFramebuffers(2, m_pingpongFBO);
    glDeleteTextures(2, m_pingpongColorbuffers);
    doneCurrent();
}

void Realtime::initializeGL() {
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.01f, 0.01f, 0.01f, 1.0f);

    int w = size().width() * devicePixelRatio();
    int h = size().height() * devicePixelRatio();
    m_defaultFBO = defaultFramebufferObject();

    m_gbuffer.init(w, h);

    m_gbufferShader = ShaderLoader::createShaderProgram("resources/shaders/gbuffer.vert", "resources/shaders/gbuffer.frag");
    m_deferredShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/deferredLighting.frag");
    m_blurShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/blur.frag");
    m_compositeShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/composite.frag");

    glGenFramebuffers(1, &m_lightingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_lightingFBO);
    glGenTextures(1, &m_lightingTexture);
    glBindTexture(GL_TEXTURE_2D, m_lightingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_lightingTexture, 0);
    checkFramebufferStatus();

    glGenFramebuffers(2, m_pingpongFBO);
    glGenTextures(2, m_pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, m_pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpongColorbuffers[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    initCube();
    initSphere(); //FOOD
    initQuad();
    initTerrain();

    // ** LOAD TEXTURES **
    m_grassDiffuseTex = loadTexture2D("resources/textures/grass_color.jpg");
    m_startTexture = loadTexture2D("resources/textures/start_screen.jpg");
    m_wallTexture = loadTexture2D("resources/textures/wall_texture.jpg");

    buildNeonScene();
    // m_snake.init(); We are changing this to resetSnake
    resetSnake();

    m_camera.setViewMatrix(m_camPos, m_camLook, glm::vec3(0,1,0));

    // --- Snake initial state (center of arena) ---
    // m_snakeState.pos = glm::vec3(0.f, 0.5f, 0.f); // sit slightly above floor
    // m_snakeState.vel = glm::vec3(0.f);

    m_snakeForceDir   = glm::vec3(0.f);
    m_snakeMass       = 1.0f;
    m_snakeForceMag   = 110.0f;
    m_snakeFriction   = 8.0f;
    m_snakeMaxSpeed   = 12.0f;

    m_snakeJumpOffset  = 0.0f;
    m_snakeJumpVel     = 0.0f;
    m_snakeJumpImpulse = 12.0f;
    m_snakeGravity     = 20.0f;
    m_snakeOnGround    = true;


    float aspect = (float)width() / (float)height();
    m_camera.setProjectionMatrix(aspect, 0.1f, 100.f, glm::radians(45.f));

    m_elapsedTimer.start();
    m_timer = startTimer(1000/60);
}

void Realtime::timerEvent(QTimerEvent *event) {
    Q_UNUSED(event);

    // Delta time
    float deltaTime = m_elapsedTimer.elapsed() * 0.001f;
    m_elapsedTimer.restart();

    // --- Power-up timers ---
    if (m_speedBoostActive) {
        m_speedBoostTimer -= deltaTime;
        if (m_speedBoostTimer <= 0.f) {
            m_speedBoostActive = false;
            m_speedBoostTimer  = 0.f;
        }
    }

    if (m_jumpBoostActive) {
        m_jumpBoostTimer -= deltaTime;
        if (m_jumpBoostTimer <= 0.f) {
            m_jumpBoostActive = false;
            m_jumpBoostTimer  = 0.f;
        }
    }

    // Lights keep bouncing
    updateLightPhysics();

    // If snake is in death animation, just advance timer and redraw
    if (m_snakeDead) {
        m_deathTimer += deltaTime;
        if (m_deathTimer >= m_deathDuration) {
            resetSnake();
        }
        update();
        return;
    }


    if (m_gameState != PLAYING) {
        update();
        return;
    }

    // ======= 1) SNAKE PHYSICS (head) =======
    // Force from WASD
    glm::vec3 F_input = m_snakeForceDir * m_snakeForceMag;
    glm::vec3 F_fric  = -m_snakeState.vel * m_snakeFriction;
    glm::vec3 F_total = F_input + F_fric;

    glm::vec3 a = F_total / m_snakeMass;
    m_snakeState.vel += a * deltaTime;

    // Clamp speed (with possible boost)
    float maxSpeed = m_snakeMaxSpeed;
    if (m_speedBoostActive) {
        maxSpeed *= 1.8f;   //
    }

    float speed = glm::length(m_snakeState.vel);
    if (speed > maxSpeed) {
        m_snakeState.vel = (m_snakeState.vel / speed) * maxSpeed;
    }

    // Integrate position on XZ plane
    m_snakeState.pos += m_snakeState.vel * deltaTime;
    m_snakeState.pos.y = 1.0f;   // stay on floor plane

    // Jump motion
    if (!m_snakeOnGround) {
        m_snakeJumpVel    -= m_snakeGravity * deltaTime;
        m_snakeJumpOffset += m_snakeJumpVel * deltaTime;

        if (m_snakeJumpOffset <= 0.f) {
            m_snakeJumpOffset = 0.f;
            m_snakeJumpVel    = 0.f;
            m_snakeOnGround   = true;
        }
    }

    // ======= 2) UPDATE TRAIL =======
    float stepDist = glm::length(m_snakeState.pos - m_lastTrailPos);
    m_trailAccumDist += stepDist;

    if (m_trailAccumDist >= m_trailSampleDist) {
        m_snakeTrail.push_front(m_snakeState.pos);
        m_lastTrailPos   = m_snakeState.pos;
        m_trailAccumDist = 0.f;

        // Keep a reasonable trail length
        size_t maxTrail = (m_snakeBody.size() + 5) * 8;
        while (m_snakeTrail.size() > maxTrail) {
            m_snakeTrail.pop_back();
        }
    }

    // ======= 3) BODY SEGMENTS FOLLOW TRAIL =======
    for (size_t i = 0; i < m_snakeBody.size(); ++i) {
        size_t idx = (i + 1) * 6;  // spacing along trail
        if (idx < m_snakeTrail.size()) {
            m_snakeBody[i] = m_snakeTrail[idx];
        } else {
            m_snakeBody[i] = m_snakeState.pos;
        }
    }

    // ======= 4) FOOD COLLISION =======
    if (m_hasFood) {
        float d = glm::length(m_snakeState.pos - m_foodPos);
        if (d < m_foodRadius) {

            // All food types still grow the snake a bit
            glm::vec3 newSeg = m_snakeState.pos;
            if (!m_snakeBody.empty()) {
                newSeg = m_snakeBody.back();
            }
            m_snakeBody.push_back(newSeg);

            // Apply effect based on type
            if (m_foodType == FOOD_NORMAL) {
                // Just growth (already done above)
            }
            else if (m_foodType == FOOD_SPEED) {
                m_speedBoostActive = true;
                m_speedBoostTimer  = m_speedBoostDuration;
            }
            else if (m_foodType == FOOD_JUMP) {
                m_jumpBoostActive = true;
                m_jumpBoostTimer  = m_jumpBoostDuration;
            }

            m_hasFood = false;
            spawnFood();
        }
    }

    // ======= 4.5) SELF-COLLISION (HEAD VS BODY) =======
    {
        // We skip the first few segments so tiny overlaps / jitter
        // near the neck don't insta-kill you.
        const float headHitRadius = 1.7f;  // pretty close, but forgiving

        for (size_t i = 1; i < m_snakeBody.size(); ++i) {
            float d = glm::length(m_snakeBody[i] - m_snakeState.pos);
            if (d < headHitRadius) {
                startSnakeDeath();
                break;
            }
        }
    }

    // ======= 5) WALL COLLISION (head) =======
    if (!m_snakeDead && snakeHeadHitsWall()) {
        startSnakeDeath();
    }

    // ======= 5B) TIMER + BOSS WAKE-UP =======
    if (!m_bossActive) {
        m_timeLeft -= deltaTime;
        if (m_timeLeft <= 0.f) {
            m_timeLeft   = 0.f;
            m_bossActive = true;

            // Spawn boss somewhere away from player
            m_bossPos = glm::vec3(-24.f, 1.0f, 24.f);
            m_bossVel = glm::vec3(0.f);
        }
    }

    // ======= 6) BOSS PATHFIND CHASE =======
    if (m_bossActive) {

        // Convert to grid space
        int bx = int(m_bossPos.x / GRID_SCALE) + GRID_SIZE/2;
        int bz = int(m_bossPos.z / GRID_SCALE) + GRID_SIZE/2;

        int sx = int(m_snakeState.pos.x / GRID_SCALE) + GRID_SIZE/2;
        int sz = int(m_snakeState.pos.z / GRID_SCALE) + GRID_SIZE/2;

        // Validate grid bounds
        if (bx>=0 && bx<GRID_SIZE && bz>=0 && bz<GRID_SIZE &&
            sx>=0 && sx<GRID_SIZE && sz>=0 && sz<GRID_SIZE)
        {
            glm::vec3 move(0.f);

            float dx = sx - bx;
            float dz = sz - bz;

            // Try X-first or Z-first based on larger distance
            if (std::abs(dx) > std::abs(dz)) {
                int step = (dx > 0) ? 1 : -1;
                if (m_mazeGrid[bx+step][bz] == 0)
                    move.x = step * GRID_SCALE;
                else {
                    int stepZ = (dz > 0) ? 1 : -1;
                    if (m_mazeGrid[bx][bz+stepZ] == 0)
                        move.z = stepZ * GRID_SCALE;
                }
            } else {
                int step = (dz > 0) ? 1 : -1;
                if (m_mazeGrid[bx][bz+step] == 0)
                    move.z = step * GRID_SCALE;
                else {
                    int stepX = (dx > 0) ? 1 : -1;
                    if (m_mazeGrid[bx+stepX][bz] == 0)
                        move.x = stepX * GRID_SCALE;
                }
            }

            // Smooth move toward next valid tile
            glm::vec3 targetPos = m_bossPos + move;
            glm::vec3 delta = targetPos - m_bossPos;
            float dist = glm::length(delta);
            if (dist > 0.001f) {
                glm::vec3 dir = delta / dist;
                m_bossPos += dir * m_bossSpeed * deltaTime;
                m_bossPos.y = 1.0f;
            }
        }

        // Boss-snake collision
        if (glm::length(m_bossPos - m_snakeState.pos) < m_bossHitRadius) {
            startSnakeDeath();
        }
    }


    update();
}



void Realtime::updateLightPhysics() {
    float arenaBounds = 28.0f;
    for(auto& light : m_lights) {
        if (light.radius != 0.0f) continue;
        glm::vec3 nextPos = light.pos + light.vel;
        int gx = (int)(nextPos.x / GRID_SCALE) + 30;
        int gz = (int)(nextPos.z / GRID_SCALE) + 30;
        bool hit = false;
        if (abs(nextPos.x) > arenaBounds) { light.vel.x *= -1; hit = true; }
        if (abs(nextPos.z) > arenaBounds) { light.vel.z *= -1; hit = true; }
        if (!hit && gx >= 0 && gx < GRID_SIZE && gz >= 0 && gz < GRID_SIZE) {
            if (m_mazeGrid[gx][gz] == 1) {
                int prevGx = (int)(light.pos.x / GRID_SCALE) + 30;
                int prevGz = (int)(light.pos.z / GRID_SCALE) + 30;
                if (gx != prevGx) light.vel.x *= -1;
                else if (gz != prevGz) light.vel.z *= -1;
                else light.vel *= -1.0f;
                hit = true;
            }
        }
        if (!hit) light.pos = nextPos;
        else light.pos += light.vel;
    }
}

//updagin snake dir vector after key is pressed
void Realtime::updateSnakeForceDirFromKeys() {
    glm::vec3 dir(0.f);

    if (m_keyMap[Qt::Key_W]) dir += glm::vec3(0.f, 0.f, -1.f);
    if (m_keyMap[Qt::Key_S]) dir += glm::vec3(0.f, 0.f,  1.f);
    if (m_keyMap[Qt::Key_A]) dir += glm::vec3(-1.f, 0.f, 0.f);
    if (m_keyMap[Qt::Key_D]) dir += glm::vec3( 1.f, 0.f, 0.f);

    if (glm::length(dir) > 0.f) dir = glm::normalize(dir);
    m_snakeForceDir = dir;
}

bool Realtime::snakeHeadHitsWall() const {
    // Match arena radius used in buildNeonScene / updateLightPhysics
    const float arenaBounds = 28.0f;

    // Head center position
    glm::vec3 p = m_snakeState.pos;
    float headCenterY = 1.0f + m_snakeJumpOffset; // base y is 1.0
    float headRadius  = 1.0f;                     // because scale = 2.f
    float headBottomY = headCenterY - headRadius;

    // Approximate top of the inner walls:
    // WALL_H = 3.5, center y = WALL_H/2 - 0.5 → 1.25
    // top ≈ 1.25 + WALL_H/2 = 3.0
    const float wallTopY = 3.0f;

    // --- 1) Outer arena bounds ---
    if ((std::abs(p.x) > arenaBounds || std::abs(p.z) > arenaBounds) &&
        headBottomY < wallTopY) {
        return true;
    }

    // --- 2) Maze walls using m_mazeGrid ---
    int gx = static_cast<int>(p.x / GRID_SCALE) + GRID_SIZE / 2;
    int gz = static_cast<int>(p.z / GRID_SCALE) + GRID_SIZE / 2;

    if (gx < 0 || gx >= GRID_SIZE || gz < 0 || gz >= GRID_SIZE) {
        return false;
    }

    // If this cell is a wall and the snake is not high enough to clear it,
    // we treat it as a collision.
    if (m_mazeGrid[gx][gz] == 1 && headBottomY < wallTopY) {
        return true;
    }

    return false;
}

void Realtime::startSnakeDeath() {
    if (m_snakeDead) return; // already animating death

    m_snakeDead    = true;
    m_deathTimer   = 0.0f;

    // Optional: you could also clear input so it doesn't "queue" movement
    m_snakeForceDir = glm::vec3(0.f);
    m_snakeState.vel = glm::vec3(0.f);
}



// Fixed 5 Arguments: Pos, Text, Color, Scale, TextureID
void Realtime::drawVoxelText(glm::vec3 startPos, std::string text, glm::vec3 color, float scale, GLuint texID) {
    std::unordered_map<char, std::vector<std::string>> font = {
                                                               {'C', {"###", "#..", "#..", "#..", "###"}}, {'S', {"###", "#..", "###", "..#", "###"}},
                                                               {'1', {".#.", "##.", ".#.", ".#.", "###"}}, {'2', {"###", "..#", "###", "#..", "###"}},
                                                               {'3', {"###", "..#", "###", "..#", "###"}}, {'0', {"###", "#.#", "#.#", "#.#", "###"}},
                                                               {'F', {"###", "#..", "###", "#..", "#.."}}, {'P', {"###", "#.#", "###", "#..", "#.."}},
                                                               };
    float spacing = 4.0f * scale;
    for (char c : text) {
        if (font.find(c) != font.end()) {
            auto bitmap = font[c];
            for (int y = 0; y < 5; y++) {
                for (int x = 0; x < 3; x++) {
                    if (bitmap[y][x] == '#') {
                        glm::vec3 pos = startPos + glm::vec3(x * scale, 0, (y) * scale);
                        m_props.push_back({ pos, glm::vec3(scale, 0.1f, scale), color, 4.0f, texID });
                    }
                }
            }
        }
        startPos.x += spacing;
    }
}



void Realtime::buildNeonScene() {
    m_props.clear();
    m_lights.clear();
    m_mazeGrid.assign(GRID_SIZE, std::vector<int>(GRID_SIZE, 0));

    // --- SETTINGS ---
    const int RADIUS = 28;
    const float WALL_H = 3.5f;
    const float INNER_THICK = 0.8f;
    const glm::vec3 cFloor(0.0f, 0.0f, 0.0f);

    // 1. FLOOR
    for(int x = -RADIUS - 6; x <= RADIUS + 6; x+=2) {
        for(int z = -RADIUS - 6; z <= RADIUS + 6; z+=2) {
            float brightness = ((x/2 + z/2) % 2 == 0) ? 0.8f : 0.6f;
            m_props.push_back({
                glm::vec3(x, -0.6f, z),
                glm::vec3(1.9f, 0.1f, 1.9f),
                cFloor + glm::vec3(0.015f) * brightness,
                0.0f, 0
            });
        }
    }

    // 2. STADIUM BOUNDARY (CLASSIC LOOK) + TEXTURE
    glm::vec3 cBorder(0.0f, 1.0f, 1.0f); // Cyan
    float gapSize = 8.0f; // Side Portal Gaps

    // Layer 1 (Low)
    float b1 = RADIUS;
    m_props.push_back({ glm::vec3(0, 0.5f, -b1), glm::vec3(b1*2, 1.0f, 1.0f), cBorder, 1.0f, m_wallTexture });
    m_props.push_back({ glm::vec3(0, 0.5f,  b1), glm::vec3(b1*2, 1.0f, 1.0f), cBorder, 1.0f, m_wallTexture });
    float len1 = b1 - gapSize/2.0f; float off1 = gapSize/2.0f + len1/2.0f;
    m_props.push_back({ glm::vec3(-b1, 0.5f, off1), glm::vec3(1.0f, 1.0f, len1*2), cBorder, 1.0f, m_wallTexture });
    m_props.push_back({ glm::vec3(-b1, 0.5f, -off1), glm::vec3(1.0f, 1.0f, len1*2), cBorder, 1.0f, m_wallTexture });
    m_props.push_back({ glm::vec3( b1, 0.5f, off1), glm::vec3(1.0f, 1.0f, len1*2), cBorder, 1.0f, m_wallTexture });
    m_props.push_back({ glm::vec3( b1, 0.5f, -off1), glm::vec3(1.0f, 1.0f, len1*2), cBorder, 1.0f, m_wallTexture });

    // Layer 2 (Mid)
    float b2 = RADIUS + 1.5f;
    m_props.push_back({ glm::vec3(0, 1.5f, -b2), glm::vec3(b2*2, 2.0f, 1.0f), cBorder, 1.5f, m_wallTexture });
    m_props.push_back({ glm::vec3(0, 1.5f,  b2), glm::vec3(b2*2, 2.0f, 1.0f), cBorder, 1.5f, m_wallTexture });
    float len2 = b2 - gapSize/2.0f; float off2 = gapSize/2.0f + len2/2.0f;
    m_props.push_back({ glm::vec3(-b2, 1.5f, off2), glm::vec3(1.0f, 2.0f, len2*2), cBorder, 1.5f, m_wallTexture });
    m_props.push_back({ glm::vec3(-b2, 1.5f, -off2), glm::vec3(1.0f, 2.0f, len2*2), cBorder, 1.5f, m_wallTexture });
    m_props.push_back({ glm::vec3( b2, 1.5f, off2), glm::vec3(1.0f, 2.0f, len2*2), cBorder, 1.5f, m_wallTexture });
    m_props.push_back({ glm::vec3( b2, 1.5f, -off2), glm::vec3(1.0f, 2.0f, len2*2), cBorder, 1.5f, m_wallTexture });

    // Layer 3 (High)
    float b3 = RADIUS + 3.0f;
    m_props.push_back({ glm::vec3(0, 3.0f, -b3), glm::vec3(b3*2, 4.0f, 1.0f), cBorder, 2.0f, m_wallTexture });
    m_props.push_back({ glm::vec3(0, 3.0f,  b3), glm::vec3(b3*2, 4.0f, 1.0f), cBorder, 2.0f, m_wallTexture });
    float len3 = b3 - gapSize/2.0f; float off3 = gapSize/2.0f + len3/2.0f;
    m_props.push_back({ glm::vec3(-b3, 3.0f, off3), glm::vec3(1.0f, 4.0f, len3*2), cBorder, 2.0f, m_wallTexture });
    m_props.push_back({ glm::vec3(-b3, 3.0f, -off3), glm::vec3(1.0f, 4.0f, len3*2), cBorder, 2.0f, m_wallTexture });
    m_props.push_back({ glm::vec3( b3, 3.0f, off3), glm::vec3(1.0f, 4.0f, len3*2), cBorder, 2.0f, m_wallTexture });
    m_props.push_back({ glm::vec3( b3, 3.0f, -off3), glm::vec3(1.0f, 4.0f, len3*2), cBorder, 2.0f, m_wallTexture });

    // Portal Pads
    glm::vec3 cPortal(1.0f, 0.0f, 1.0f); // Magenta
    m_props.push_back({ glm::vec3(-RADIUS, -0.5f, 0), glm::vec3(1.0f, 0.1f, gapSize), cPortal, 5.0f, 0 });
    m_props.push_back({ glm::vec3( RADIUS, -0.5f, 0), glm::vec3(1.0f, 0.1f, gapSize), cPortal, 5.0f, 0 });

    // Portal Lights
    m_lights.push_back({ glm::vec3(-RADIUS, 1.0f, 0), glm::vec3(0), cPortal, 5.0f });
    m_lights.push_back({ glm::vec3( RADIUS, 1.0f, 0), glm::vec3(0), cPortal, 5.0f });

    // 3. CIRCUIT BOARD MAZE (Sleek inner walls, No Texture)
    srand(1234);
    auto getRainbow = [](float t) {
        return glm::vec3(0.5f+0.5f*sin(t), 0.5f+0.5f*sin(t+2.0f), 0.5f+0.5f*sin(t+4.0f));
    };

    for (int x = 6; x < RADIUS - 4; x += 10) {
        for (int z = 6; z < RADIUS - 4; z += 10) {
            glm::vec3 color = getRainbow(x * 0.1f + z * 0.1f);
            glm::vec3 glassColor = color * 0.15f;
            int type = rand() % 4;
            std::vector<glm::vec3> shapes;
            if (type == 0) { shapes.push_back({0,0,0}); shapes.push_back({0,0,2}); shapes.push_back({0,0,-2}); }
            else if (type == 1) { shapes.push_back({0,0,0}); shapes.push_back({2,0,0}); shapes.push_back({-2,0,0}); }
            else if (type == 2) { shapes.push_back({0,0,0}); shapes.push_back({2,0,0}); shapes.push_back({0,0,2}); }
            else { shapes.push_back({0,0,0}); shapes.push_back({2,0,0}); shapes.push_back({-2,0,0}); shapes.push_back({2,0,2}); shapes.push_back({-2,0,2}); }

            for (auto& offset : shapes) {
                float q1x = x + offset.x; float q1z = z + offset.z;
                glm::vec3 hScale(INNER_THICK, WALL_H, 2.0f);
                glm::vec3 wScale(2.0f, WALL_H, INNER_THICK);
                glm::vec3 finalScale = (type == 1 || (type > 1 && offset.z == 0)) ? wScale : hScale;
                std::vector<glm::vec3> positions = { { q1x, WALL_H/2.0f - 0.5f, q1z }, { -q1x, WALL_H/2.0f - 0.5f, q1z }, { q1x, WALL_H/2.0f - 0.5f, -q1z }, { -q1x, WALL_H/2.0f - 0.5f, -q1z } };
                for(auto& p : positions) {
                    m_props.push_back({ p, finalScale, glassColor, 4.0f, 0 });
                    int rX = (int)(finalScale.x/2.0f)+1; int rZ = (int)(finalScale.z/2.0f)+1;
                    for(int dx = -rX; dx <= rX; dx++) { for(int dz = -rZ; dz <= rZ; dz++) {
                            int gx = (int)(p.x + dx) + 30; int gz = (int)(p.z + dz) + 30;
                            if(gx>=0 && gx<60 && gz>=0 && gz<60) m_mazeGrid[gx][gz] = 1;
                        }}
                }
            }
        }
    }

    // 4. BOUNCING LIGHTS
    for(int i=0; i<80; i++) {
        float rX = (rand() % (RADIUS*2)) - RADIUS; float rZ = (rand() % (RADIUS*2)) - RADIUS;
        int gx = (int)(rX) + 30; int gz = (int)(rZ) + 30;
        if(gx >=0 && gx<60 && gz>=0 && gz<60 && m_mazeGrid[gx][gz] == 1) continue;
        float vx = ((rand() % 100) / 100.0f - 0.5f) * 0.3f; float vz = ((rand() % 100) / 100.0f - 0.5f) * 0.3f;
        if (abs(vx) < 0.05f) vx = 0.1f;
        glm::vec3 color = getRainbow(i * 0.3f);
        m_lights.push_back({ glm::vec3(rX, 1.5f, rZ), glm::vec3(vx, 0, vz), color, 0.0f });
    }

    // 5. TITLE TEXT (With Texture!)
    drawVoxelText(glm::vec3(-25.0f, 12.0f, -RADIUS - 5.0f), "CS1230", glm::vec3(0,1,1), 2.5f, m_wallTexture);
}

void Realtime::paintGL() {
    m_defaultFBO = defaultFramebufferObject();
    while (glGetError() != GL_NO_ERROR);

    int w = size().width() * devicePixelRatio();
    int h = size().height() * devicePixelRatio();

    // A. START SCREEN
    if (m_gameState == START_SCREEN) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (m_startTexture != 0) {
            glUseProgram(m_compositeShader);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_startTexture);
            glUniform1i(glGetUniformLocation(m_compositeShader, "scene"), 0);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_startTexture);
            glUniform1i(glGetUniformLocation(m_compositeShader, "bloomBlur"), 1);
            glUniform1f(glGetUniformLocation(m_compositeShader, "exposure"), 1.0f);
            glBindVertexArray(m_quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        } else {
            glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        return;
    }


    // --- PHASE 1: GEOMETRY ---
    m_gbuffer.bindForWriting();
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(m_gbufferShader);

    glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "view"), 1, GL_FALSE,
                       &m_camera.getViewMatrix()[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "proj"), 1, GL_FALSE,
                       &m_camera.getProjMatrix()[0][0]);

    // Death animation progress [0,1]
    float deathT = 0.0f;
    if (m_snakeDead && m_deathDuration > 0.0f) {
        deathT = m_deathTimer / m_deathDuration;
        if (deathT < 0.0f) deathT = 0.0f;
        if (deathT > 1.0f) deathT = 1.0f;
    }

    // === Use cube VAO for arena + snake ===
    glBindVertexArray(m_cubeVAO);

    // 1) ARENA PROPS
    for (const auto& prop : m_props) {
        glm::mat4 model =
            glm::translate(glm::mat4(1.f), prop.pos) *
            glm::scale(glm::mat4(1.f), prop.scale);

        glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "albedoColor"), 1, &prop.color[0]);

        glm::vec3 emissive = prop.color * prop.emissiveStrength;
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "emissiveColor"), 1, &emissive[0]);

        if (prop.textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, prop.textureID);
            glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 1);
            glUniform1i(glGetUniformLocation(m_gbufferShader, "textureSampler"), 0);
        } else {
            glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 0);
        }

        glDrawArrays(GL_TRIANGLES, 0, m_cubeNumVerts);
    }

    // 2) snake head (with optional death squish)
    {
        glm::vec3 snakePos =
            m_snakeState.pos + glm::vec3(0.f, m_snakeJumpOffset, 0.f);

        // Base scale for alive snake
        glm::vec3 baseScale = glm::vec3(2.f);

        // Apply squish/stretch during death animation
        // Squash Y down, stretch XZ out a bit as deathT -> 1
        float squash  = 1.0f - 0.5f * deathT;  // from 1.0 to 0.5
        float stretch = 1.0f + 0.4f * deathT;  // from 1.0 to 1.4

        glm::vec3 headScale = glm::vec3(baseScale.x * stretch,
                                        baseScale.y * squash,
                                        baseScale.z * stretch);

        glm::mat4 model =
            glm::translate(glm::mat4(1.f), snakePos) *
            glm::scale(glm::mat4(1.f), headScale);

        glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "model"), 1, GL_FALSE, &model[0][0]);

        // Base colors
        glm::vec3 aliveColor    = glm::vec3(0.0f, 0.55f, 1.0f);
        glm::vec3 aliveEmissive = glm::vec3(0.0f, 2.0f, 3.5f);

        // Fade to black + no glow as deathT -> 1
        glm::vec3 snakeColor =
            (1.0f - deathT) * aliveColor;
        glm::vec3 snakeEmissive =
            (1.0f - deathT) * aliveEmissive;

        glUniform3fv(glGetUniformLocation(m_gbufferShader, "albedoColor"), 1, &snakeColor[0]);
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "emissiveColor"), 1, &snakeEmissive[0]);

        glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 0);

        glDrawArrays(GL_TRIANGLES, 0, m_cubeNumVerts);
    }


    // 3) BODY SEGMENTS
    for (const glm::vec3 &segPos : m_snakeBody) {
        glm::vec3 segRenderPos = segPos + glm::vec3(0.f, m_snakeJumpOffset, 0.f);

        glm::mat4 bodyModel =
            glm::translate(glm::mat4(1.f), segRenderPos) *
            glm::scale(glm::mat4(1.f), glm::vec3(1.6f));

        glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "model"),
                           1, GL_FALSE, &bodyModel[0][0]);

        glm::vec3 bodyColor    = glm::vec3(0.0f, 0.45f, 0.9f);
        glm::vec3 bodyEmissive = glm::vec3(0.0f, 1.0f, 2.0f);

        glUniform3fv(glGetUniformLocation(m_gbufferShader, "albedoColor"),  1, &bodyColor[0]);
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "emissiveColor"),1, &bodyEmissive[0]);
        glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 0);

        glDrawArrays(GL_TRIANGLES, 0, m_cubeNumVerts);
    }

    // --- FOOD SPHERE ---
    if (m_hasFood && m_sphereVAO != 0 && m_sphereNumVerts > 0) {
        glm::mat4 foodModel =
            glm::translate(glm::mat4(1.f), m_foodPos) *
            glm::scale(glm::mat4(1.f), glm::vec3(2.0f));

        glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "model"),
                           1, GL_FALSE, &foodModel[0][0]);

        // Soft glowing green-yellow
        // Pick color based on type
        glm::vec3 foodColor;
        glm::vec3 foodEmissive;

        if (m_foodType == FOOD_NORMAL) {
            // Dirty green-yellow (classic)
            foodColor    = glm::vec3(0.25f, 0.30f, 0.10f);
            foodEmissive = glm::vec3(0.20f, 0.30f, 0.12f);
        }
        else if (m_foodType == FOOD_SPEED) {
            // Electric cyan-ish
            foodColor    = glm::vec3(0.0f, 0.7f, 1.0f);
            foodEmissive = glm::vec3(0.0f, 1.4f, 2.0f);
        }
        else { // FOOD_JUMP
            // Neon purple
            foodColor    = glm::vec3(0.7f, 0.25f, 0.9f);
            foodEmissive = glm::vec3(1.4f, 0.5f, 1.8f);
        }

        glUniform3fv(glGetUniformLocation(m_gbufferShader, "albedoColor"),
                     1, &foodColor[0]);
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "emissiveColor"),
                     1, &foodEmissive[0]);

        glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 0);

        // no specular so it glows instead of looking plasticky
        glUniform1f(glGetUniformLocation(m_gbufferShader, "k_d"), 0.0f);
        glUniform1f(glGetUniformLocation(m_gbufferShader, "k_s"), 0.0f);
        glUniform1f(glGetUniformLocation(m_gbufferShader, "shininess"), 1.0f);


        glBindVertexArray(m_sphereVAO);
        glDrawArrays(GL_TRIANGLES, 0, m_sphereNumVerts);
        glBindVertexArray(0);
    }

    // === BOSS CUBE ===
    if (m_bossActive) {
        glBindVertexArray(m_cubeVAO);

        glm::mat4 model =
            glm::translate(glm::mat4(1.f), m_bossPos) *
            glm::scale(glm::mat4(1.f), glm::vec3(2.0f)); // good size

        glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "model"),
                           1, GL_FALSE, &model[0][0]);

        glm::vec3 bossColor    = glm::vec3(1.0f, 0.15f, 0.3f);
        glm::vec3 bossEmissive = glm::vec3(3.0f, 0.25f, 1.0f);

        glUniform3fv(glGetUniformLocation(m_gbufferShader, "albedoColor"),
                     1, &bossColor[0]);
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "emissiveColor"),
                     1, &bossEmissive[0]);

        glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 0);

        glDrawArrays(GL_TRIANGLES, 0, m_cubeNumVerts);
    }


    // Done with geometry
    glBindVertexArray(0);
    GL_CHECK();


    // --- PHASE 2: LIGHTING ---
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, m_lightingFBO);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_deferredShader);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getPositionTex());
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getNormalTex());
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getAlbedoTex());
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getEmissiveTex());
    glUniform1i(glGetUniformLocation(m_deferredShader, "gPosition"), 0);
    glUniform1i(glGetUniformLocation(m_deferredShader, "gNormal"), 1);
    glUniform1i(glGetUniformLocation(m_deferredShader, "gAlbedo"), 2);
    glUniform1i(glGetUniformLocation(m_deferredShader, "gEmissive"), 3);
    glUniform3fv(glGetUniformLocation(m_deferredShader, "camPos"), 1, &m_camera.getPosition()[0]);
    int numLights = std::min((int)m_lights.size(), 100);
    glUniform1i(glGetUniformLocation(m_deferredShader, "numLights"), numLights);

    // --- Fog uniforms (NEW) ---
    // arena radius ≈ 28, so start just before the wall and fade outwards
    float fogStart = 26.0f;  // radius where fog starts to appear
    float fogEnd   = 40.0f;  // fully fogged

    glm::vec3 fogCol(0.05f, 0.06f, 0.10f); // dark bluish club fog;
    // You can also match your glClearColor if you want

    glUniform1f(glGetUniformLocation(m_deferredShader, "fogStartRadius"), fogStart);
    glUniform1f(glGetUniformLocation(m_deferredShader, "fogEndRadius"),   fogEnd);
    glUniform3fv(glGetUniformLocation(m_deferredShader, "fogColor"), 1, &fogCol[0]);


    glUniform1f(glGetUniformLocation(m_deferredShader, "k_s"), 1.5f);
    for(int i=0; i<numLights; i++) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform1i(glGetUniformLocation(m_deferredShader, (base + ".type").c_str()), 0);
        glUniform3fv(glGetUniformLocation(m_deferredShader, (base + ".pos").c_str()), 1, &m_lights[i].pos[0]);
        glUniform3fv(glGetUniformLocation(m_deferredShader, (base + ".color").c_str()), 1, &m_lights[i].color[0]);
        glUniform3f(glGetUniformLocation(m_deferredShader, (base + ".atten").c_str()), 0.1f, 0.05f, 0.005f);
    }
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL_CHECK();

    // --- PHASE 3: BLUR ---
    bool horizontal = true;
    glUseProgram(m_blurShader);
    for (int i = 0; i < 10; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
        glUniform1i(glGetUniformLocation(m_blurShader, "horizontal"), horizontal);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (i==0) ? m_gbuffer.getEmissiveTex() : m_pingpongColorbuffers[!horizontal]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horizontal = !horizontal;
    }
    GL_CHECK();

    // --- PHASE 4: COMPOSITE ---
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_compositeShader);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_lightingTexture);
    glUniform1i(glGetUniformLocation(m_compositeShader, "scene"), 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_pingpongColorbuffers[!horizontal]);
    glUniform1i(glGetUniformLocation(m_compositeShader, "bloomBlur"), 1);
    glUniform1f(glGetUniformLocation(m_compositeShader, "exposure"), 1.2f);
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL_CHECK();
    glEnable(GL_DEPTH_TEST);
}

void Realtime::resizeGL(int w, int h) {
    if(w<=0||h<=0) return;
    glViewport(0,0,w,h);
    int w_dpi = w*devicePixelRatio();
    int h_dpi = h*devicePixelRatio();
    m_gbuffer.resize(w_dpi, h_dpi);
    glBindTexture(GL_TEXTURE_2D, m_lightingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w_dpi, h_dpi, 0, GL_RGBA, GL_FLOAT, NULL);
    for(int i=0; i<2; i++) {
        glBindTexture(GL_TEXTURE_2D, m_pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w_dpi, h_dpi, 0, GL_RGBA, GL_FLOAT, NULL);
    }
    float aspect = (float)w / (float)h;
    m_camera.setProjectionMatrix(aspect, 0.1f, 100.f, glm::radians(45.f));
}

void Realtime::initCube() {
    Cube cube; cube.updateParams(1, 1);
    std::vector<float> data = cube.generateShape();
    m_cubeNumVerts = data.size() / 6;
    glGenVertexArrays(1, &m_cubeVAO); glBindVertexArray(m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO); glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glBindVertexArray(0);
}

//sphere for food
void Realtime::initSphere() {
    Sphere sphere;
    sphere.updateParams(20, 20);             // reasonably smooth
    std::vector<float> data = sphere.generateShape();

    m_sphereNumVerts = data.size() / 6;      // pos(3) + norm(3)

    glGenVertexArrays(1, &m_sphereVAO);
    glBindVertexArray(m_sphereVAO);

    glGenBuffers(1, &m_sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 data.size() * sizeof(float),
                 data.data(),
                 GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);

    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}



//When snake dies/ to spawn snake
void Realtime::resetSnake() {
    // head
    m_snakeState.pos = glm::vec3(0.f, 1.0f, 0.f);
    m_snakeState.vel = glm::vec3(0.f);
    m_snakeForceDir  = glm::vec3(0.f);

    // jump
    m_snakeJumpOffset  = 0.f;
    m_snakeJumpVel     = 0.f;
    m_snakeOnGround    = true;

    // trail + body
    m_snakeTrail.clear();
    m_snakeBody.clear();
    m_lastTrailPos    = m_snakeState.pos;
    m_trailAccumDist  = 0.f;
    m_trailSampleDist = 1.2f;   // spacing between samples (tweak feel)

    // food
    m_hasFood   = false;
    m_foodRadius = 2.0f;       // works with 2.0f sphere scale
    spawnFood();

    // Reset death animation state
    m_snakeDead     = false;
    m_deathTimer    = 0.0f;
    m_deathDuration = 0.25f;

    m_bossActive = false;
    m_bossPos    = glm::vec3(0.f);   // doesn't matter, not drawn when inactive
    m_bossVel    = glm::vec3(0.f);
    m_timeLeft   = 20.0f;

    m_speedBoostActive = false;
    m_speedBoostTimer  = 0.f;
    m_jumpBoostActive  = false;
    m_jumpBoostTimer   = 0.f;
}


void Realtime::spawnFood() {
    const float arenaBounds = 26.0f;   // slightly inside walls
    const float margin      = 3.0f;

    // Random type: 0 = normal, 1 = speed, 2 = jump
    int r = rand() % 100;

    if (r < 70) {
        m_foodType = FOOD_NORMAL;
    }
    else if (r < 85) {
        m_foodType = FOOD_SPEED;
    }
    else {
        m_foodType = FOOD_JUMP;
    }

    // Find a free spot in the arena (avoid maze walls)
    for (int tries = 0; tries < 100; ++tries) {
        float x = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * (arenaBounds - margin);
        float z = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * (arenaBounds - margin);

        int gx = static_cast<int>(x / GRID_SCALE) + GRID_SIZE / 2;
        int gz = static_cast<int>(z / GRID_SCALE) + GRID_SIZE / 2;

        if (gx < 0 || gx >= GRID_SIZE || gz < 0 || gz >= GRID_SIZE) continue;
        if (m_mazeGrid[gx][gz] == 1) continue; // don't spawn in wall

        m_foodPos = glm::vec3(x, 1.0f, z); // same height as snake
        m_hasFood = true;
        return;
    }

    // Fallback: put it at center if all else fails
    m_foodPos = glm::vec3(0.f, 1.0f, 0.f);
    m_hasFood = true;
}


void Realtime::initQuad() {
    float verts[] = {-1,-1,0,0, 1,-1,1,0, -1,1,0,1, 1,1,1,1, -1,1,0,1, 1,-1,1,0};
    glGenVertexArrays(1, &m_quadVAO); glBindVertexArray(m_quadVAO);
    glGenBuffers(1, &m_quadVBO); glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

GLuint Realtime::loadTexture2D(const std::string &path) {
    QImage img(QString::fromStdString(path));
    if (img.isNull()) return 0;
    img = img.convertToFormat(QImage::Format_RGBA8888).mirrored();
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

void Realtime::keyPressEvent(QKeyEvent *e) {
    // Start screen -> playing
    if (m_gameState == START_SCREEN && e->key() == Qt::Key_Space) {
        m_gameState = PLAYING;
        update();
        return;
    }

    if (m_gameState == PLAYING) {
        int key = e->key();

        // Jump
        if (key == Qt::Key_Space && m_snakeOnGround) {
            m_snakeOnGround = false;

            float jumpImpulse = m_snakeJumpImpulse;
            if (m_jumpBoostActive) {
                jumpImpulse *= 1.7f;   // jump higher when boosted
            }

            m_snakeJumpVel = jumpImpulse;
        }

        // WASD movement
        if (key == Qt::Key_W || key == Qt::Key_A ||
            key == Qt::Key_S || key == Qt::Key_D) {

            m_keyMap[key] = true;
            updateSnakeForceDirFromKeys();
        }
    }

    update();
}

void Realtime::keyReleaseEvent(QKeyEvent *e) {
    if (m_gameState != PLAYING) return;

    int key = e->key();
    m_keyMap[key] = false;

    if (key == Qt::Key_W || key == Qt::Key_A ||
        key == Qt::Key_S || key == Qt::Key_D) {
        updateSnakeForceDirFromKeys();
    }
}



void Realtime::mousePressEvent(QMouseEvent *e) { if(e->button()==Qt::LeftButton) { m_mouseDown=true; m_prevMousePos=glm::vec2(e->pos().x(),e->pos().y()); }}
void Realtime::mouseReleaseEvent(QMouseEvent *e) { m_mouseDown=false; }
void Realtime::mouseMoveEvent(QMouseEvent *e) {
    if(!m_mouseDown) return;
    glm::vec2 cur(e->pos().x(), e->pos().y());
    glm::vec2 d = cur - m_prevMousePos; m_prevMousePos = cur;
    m_camera.rotateAroundUp(-d.x * 0.005f); m_camera.rotateAroundRight(-d.y * 0.005f);
    update();
}
// Stubs
void Realtime::sceneChanged(){} void Realtime::settingsChanged(){} void Realtime::saveViewportImage(const std::string&){} void Realtime::initTerrain(){}
