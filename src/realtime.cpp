#include "realtime.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "utils/debug.h"
#include <cstdlib> // For rand()

// --- DEBUG HELPER ---
void checkFramebufferStatus() {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "!!! FBO ERROR: 0x" << std::hex << status << std::dec << std::endl;
    }
}

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent), m_mouseDown(false),
    m_camPos(0.f, 45.f, 40.f), // High up camera looking down
    m_camLook(0.f, -0.9f, -0.5f),
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
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    int w = size().width() * devicePixelRatio();
    int h = size().height() * devicePixelRatio();
    m_defaultFBO = defaultFramebufferObject();

    // 1. Init G-Buffer
    m_gbuffer.init(w, h);

    // 2. Load Shaders
    m_gbufferShader = ShaderLoader::createShaderProgram("resources/shaders/gbuffer.vert", "resources/shaders/gbuffer.frag");
    m_deferredShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/deferredLighting.frag");
    m_blurShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/blur.frag");
    m_compositeShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/composite.frag");

    // 3. Init Lighting FBO (High Precision RGBA16F)
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

    // 4. Init Ping-Pong FBOs (Blur)
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

    // 5. Init Geometry & Resources
    initCube();
    initQuad();
    initTerrain(); // Stub

    m_grassDiffuseTex = loadTexture2D("resources/textures/grass_color.jpg");
    m_startTexture = loadTexture2D("resources/textures/start_screen.jpg");

    // BUILD THE NEON WORLD
    buildNeonScene();

    m_snake.init();

    // Camera
    m_camera.setViewMatrix(m_camPos, m_camLook, glm::vec3(0,1,0));
    float aspect = (float)width() / (float)height();
    m_camera.setProjectionMatrix(aspect, 0.1f, 100.f, glm::radians(45.f));

    m_elapsedTimer.start();
    startTimer(1000/60);
}

// ----------------------------------------------------------------
// DESIGNING THE "NEON SLITHER" ARENA
// ----------------------------------------------------------------
void Realtime::buildNeonScene() {
    m_props.clear();
    m_lights.clear();

    // --- SETTINGS ---
    int arenaSize = 20; // 40x40 Grid
    int numObstacles = 45;
    glm::vec3 cFloor(0.05f, 0.05f, 0.08f); // Deep space floor

    // Helper: Rainbow function
    auto getRainbowColor = [](float t) {
        return glm::vec3(
            0.5f + 0.5f * std::sin(t * 0.2f + 0.0f),
            0.5f + 0.5f * std::sin(t * 0.2f + 2.0f),
            0.5f + 0.5f * std::sin(t * 0.2f + 4.0f)
            );
    };

    // 1. THE FLOOR
    for(int x = -arenaSize + 1; x < arenaSize; x++) {
        for(int z = -arenaSize + 1; z < arenaSize; z++) {
            float brightness = ((x + z) % 2 == 0) ? 1.0f : 0.8f;
            m_props.push_back({
                glm::vec3(x, -0.6f, z),
                glm::vec3(0.95f, 0.1f, 0.95f), // Gaps for grid lines
                cFloor * brightness,
                0.0f // No glow
            });
        }
    }

    // 2. RAINBOW WALLS (Tall & Glowing)
    float wallHeight = 3.0f;
    for(int i = -arenaSize; i <= arenaSize; i++) {
        glm::vec3 color = getRainbowColor((float)i);
        // Top/Bottom
        m_props.push_back({ glm::vec3(i, 0.5f, -arenaSize), glm::vec3(1, wallHeight, 1), color, 2.0f });
        m_props.push_back({ glm::vec3(i, 0.5f,  arenaSize), glm::vec3(1, wallHeight, 1), color, 2.0f });
        // Left/Right
        m_props.push_back({ glm::vec3(-arenaSize, 0.5f, i), glm::vec3(1, wallHeight, 1), color, 2.0f });
        m_props.push_back({ glm::vec3( arenaSize, 0.5f, i), glm::vec3(1, wallHeight, 1), color, 2.0f });
    }

    // 3. RANDOM NEON OBSTACLES
    srand(1234);
    for(int i = 0; i < numObstacles; i++) {
        int rx = (rand() % (2 * arenaSize - 4)) - (arenaSize - 2);
        int rz = (rand() % (2 * arenaSize - 4)) - (arenaSize - 2);

        // Avoid center spawn
        if (abs(rx) < 4 && abs(rz) < 4) continue;

        float h = 1.0f + (rand() % 40) / 10.0f; // Height 1.0 - 5.0
        glm::vec3 obsColor = getRainbowColor((float)(rx * rz));

        m_props.push_back({
            glm::vec3(rx, h/2.0f - 0.5f, rz),
            glm::vec3(0.8f, h, 0.8f),
            obsColor,
            1.5f
        });

        // Add lights to some obstacles
        if (i % 5 == 0) {
            m_lights.push_back({ glm::vec3(rx, 1.0f, rz), obsColor, 4.0f });
        }
    }

    // 4. THE SNAKE (Static Demo)
    glm::vec3 cSnake(0.1f, 1.0f, 0.8f); // Cyan
    std::vector<glm::vec3> snakePos = {{0,0,0}, {-1,0,0}, {-2,0,0}, {-3,0,0}};
    for(const auto& pos : snakePos) {
        m_props.push_back({ pos, glm::vec3(0.9f), cSnake, 3.0f }); // Bright Glow
        m_lights.push_back({ pos + glm::vec3(0, 0.5f, 0), cSnake, 3.0f });
    }

    // 5. THE APPLE
    glm::vec3 applePos(5, 0, 5);
    glm::vec3 cApple(1.0f, 0.2f, 0.5f); // Hot Pink
    m_props.push_back({ applePos, glm::vec3(0.7f), cApple, 5.0f }); // SUPER GLOW
    m_lights.push_back({ applePos + glm::vec3(0, 0.5f, 0), cApple, 5.0f });
}

// ----------------------------------------------------------------
// RENDER LOOP
// ----------------------------------------------------------------
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
            glBindVertexArray(0);
        } else {
            glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // Fallback Pink
            glClear(GL_COLOR_BUFFER_BIT);
        }
        return;
    }

    // --- PHASE 1: GEOMETRY PASS ---
    m_gbuffer.bindForWriting();
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(m_gbufferShader);
    glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "view"), 1, GL_FALSE, &m_camera.getViewMatrix()[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "proj"), 1, GL_FALSE, &m_camera.getProjMatrix()[0][0]);
    glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 0);

    glBindVertexArray(m_cubeVAO);
    for (const auto& prop : m_props) {
        glm::mat4 model = glm::translate(glm::mat4(1.f), prop.pos) * glm::scale(glm::mat4(1.f), prop.scale);
        glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "albedoColor"), 1, &prop.color[0]);
        glm::vec3 emissive = prop.color * prop.emissiveStrength;
        glUniform3fv(glGetUniformLocation(m_gbufferShader, "emissiveColor"), 1, &emissive[0]);
        glDrawArrays(GL_TRIANGLES, 0, m_cubeNumVerts);
    }
    glBindVertexArray(0);
    GL_CHECK();

    // --- PHASE 2: LIGHTING PASS ---
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
    int numLights = std::min((int)m_lights.size(), 8);
    glUniform1i(glGetUniformLocation(m_deferredShader, "numLights"), numLights);
    for(int i=0; i<numLights; i++) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform1i(glGetUniformLocation(m_deferredShader, (base + ".type").c_str()), 0);
        glUniform3fv(glGetUniformLocation(m_deferredShader, (base + ".pos").c_str()), 1, &m_lights[i].pos[0]);
        glUniform3fv(glGetUniformLocation(m_deferredShader, (base + ".color").c_str()), 1, &m_lights[i].color[0]);
        glUniform3f(glGetUniformLocation(m_deferredShader, (base + ".atten").c_str()), 0.5f, 0.5f, 0.1f);
    }
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL_CHECK();

    // --- PHASE 3: BLUR PASS ---
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

    // Increased exposure for "Slither" glow
    glUniform1f(glGetUniformLocation(m_compositeShader, "exposure"), 1.2f);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    GL_CHECK();

    glEnable(GL_DEPTH_TEST);
}

// ----------------------------------------------------------------
// BOILERPLATE
// ----------------------------------------------------------------
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
    if(m_gameState==START_SCREEN && e->key()==Qt::Key_Space) { m_gameState=PLAYING; update(); return; }
    m_keyMap[e->key()] = true; update();
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
void Realtime::sceneChanged(){} void Realtime::settingsChanged(){} void Realtime::saveViewportImage(const std::string&){} void Realtime::timerEvent(QTimerEvent*){} void Realtime::initTerrain(){}





// #include "realtime.h"
// #include <QMouseEvent>
// #include <QKeyEvent>
// #include <iostream>
// #include <algorithm>
// #include <glm/gtc/matrix_transform.hpp>
// #include "utils/debug.h"

// // --- DEBUG HELPER ---
// void checkFramebufferStatus() {
//     GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//     if (status != GL_FRAMEBUFFER_COMPLETE) {
//         std::cerr << "!!! FBO ERROR: 0x" << std::hex << status << std::dec << std::endl;
//     }
// }

// Realtime::Realtime(QWidget *parent)
//     : QOpenGLWidget(parent), m_mouseDown(false),
//     m_camPos(0.f, 25.f, 20.f), m_camLook(0.f, -0.9f, -1.f)
// {
//     setFocusPolicy(Qt::StrongFocus);
// }

// void Realtime::finish() {
//     makeCurrent();
//     glDeleteVertexArrays(1, &m_cubeVAO);
//     glDeleteBuffers(1, &m_cubeVBO);
//     glDeleteVertexArrays(1, &m_quadVAO);
//     glDeleteBuffers(1, &m_quadVBO);
//     glDeleteProgram(m_gbufferShader);
//     glDeleteProgram(m_deferredShader);
//     glDeleteProgram(m_blurShader);
//     glDeleteProgram(m_compositeShader);
//     glDeleteFramebuffers(1, &m_lightingFBO);
//     glDeleteTextures(1, &m_lightingTexture);
//     glDeleteFramebuffers(2, m_pingpongFBO);
//     glDeleteTextures(2, m_pingpongColorbuffers);
//     doneCurrent();
// }

// void Realtime::initializeGL() {
//     glewInit();
//     glEnable(GL_DEPTH_TEST);
//     glEnable(GL_CULL_FACE);
//     glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

//     int w = size().width() * devicePixelRatio();
//     int h = size().height() * devicePixelRatio();

//     // 1. Init G-Buffer
//     m_gbuffer.init(w, h);

//     // 2. Load Shaders
//     m_gbufferShader = ShaderLoader::createShaderProgram("resources/shaders/gbuffer.vert", "resources/shaders/gbuffer.frag");
//     m_deferredShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/deferredLighting.frag");
//     m_blurShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/blur.frag");
//     m_compositeShader = ShaderLoader::createShaderProgram("resources/shaders/fullscreen_quad.vert", "resources/shaders/composite.frag");

//     // 3. Init Lighting FBO (High Precision)
//     glGenFramebuffers(1, &m_lightingFBO);
//     glBindFramebuffer(GL_FRAMEBUFFER, m_lightingFBO);
//     glGenTextures(1, &m_lightingTexture);
//     glBindTexture(GL_TEXTURE_2D, m_lightingTexture);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_lightingTexture, 0);
//     checkFramebufferStatus();

//     // 4. Init Ping-Pong FBOs (Blur)
//     glGenFramebuffers(2, m_pingpongFBO);
//     glGenTextures(2, m_pingpongColorbuffers);
//     for (unsigned int i = 0; i < 2; i++) {
//         glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
//         glBindTexture(GL_TEXTURE_2D, m_pingpongColorbuffers[i]);
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//         glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpongColorbuffers[i], 0);
//     }
//     glBindFramebuffer(GL_FRAMEBUFFER, 0);

//     // 5. Init Geometry & Logic
//     initCube();
//     initQuad();
//     initTerrain(); // Stub

//     m_grassDiffuseTex = loadTexture2D("resources/textures/grass_color.jpg");
//     m_startTexture = loadTexture2D("resources/textures/start_screen.jpg");

//     buildNeonScene();
//     m_snake.init();

//     // Camera
//     m_camera.setViewMatrix(m_camPos, m_camLook, glm::vec3(0,1,0));
//     float aspect = (float)width() / (float)height();
//     m_camera.setProjectionMatrix(aspect, 0.1f, 100.f, glm::radians(45.f));

//     m_elapsedTimer.start();
//     startTimer(1000/60);
// }

// // ----------------------------------------------------------------
// // SCENE SETUP
// // ----------------------------------------------------------------
// void Realtime::buildNeonScene() {
//     m_props.clear();
//     m_lights.clear();

//     // Colors
//     glm::vec3 cDarkFloor(0.1f, 0.1f, 0.15f);
//     glm::vec3 cNeonGreen(0.1f, 1.0f, 0.1f);
//     glm::vec3 cNeonRed(1.0f, 0.1f, 0.1f);

//     // Floor
//     for(int x = -7; x <= 7; x++) {
//         for(int z = -7; z <= 7; z++) {
//             float brightness = ((x + z) % 2 == 0) ? 1.0f : 0.8f;
//             m_props.push_back({glm::vec3(x, -0.6f, z), glm::vec3(0.95f, 0.1f, 0.95f), cDarkFloor * brightness, 0.0f});
//         }
//     }
//     // Walls
//     int border = 8;
//     for(int x = -border; x <= border; x++) {
//         m_props.push_back({ glm::vec3(x, 0, -border), glm::vec3(1), cNeonGreen, 0.5f });
//         m_props.push_back({ glm::vec3(x, 0,  border), glm::vec3(1), cNeonGreen, 0.5f });
//     }
//     for(int z = -border + 1; z <= border - 1; z++) {
//         m_props.push_back({ glm::vec3(-border, 0, z), glm::vec3(1), cNeonGreen, 0.5f });
//         m_props.push_back({ glm::vec3( border, 0, z), glm::vec3(1), cNeonGreen, 0.5f });
//     }
//     // Snake
//     std::vector<glm::vec3> snakePos = {{0,0,0}, {-1,0,0}, {-2,0,0}, {-2,0,1}, {-2,0,2}};
//     for(const auto& pos : snakePos) {
//         m_props.push_back({ pos, glm::vec3(0.8f), cNeonGreen, 2.0f });
//         m_lights.push_back({ pos + glm::vec3(0, 0.5f, 0), cNeonGreen, 3.0f });
//     }
//     // Apple
//     glm::vec3 applePos(3, 0, 2);
//     m_props.push_back({ applePos, glm::vec3(0.6f), cNeonRed, 5.0f });
//     m_lights.push_back({ applePos + glm::vec3(0, 0.5f, 0), cNeonRed, 5.0f });
// }

// // ----------------------------------------------------------------
// // RENDER LOOP
// // ----------------------------------------------------------------
// void Realtime::paintGL() {
//     // *** FIX 1: UPDATE DEFAULT FBO ID EVERY FRAME ***
//     m_defaultFBO = defaultFramebufferObject();

//     // Clear previous errors
//     while (glGetError() != GL_NO_ERROR);

//     int w = size().width() * devicePixelRatio();
//     int h = size().height() * devicePixelRatio();

//     // A. START SCREEN
//     if (m_gameState == START_SCREEN) {
//         glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
//         glViewport(0, 0, w, h);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         if (m_startTexture != 0) {
//             glUseProgram(m_compositeShader);
//             glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_startTexture);
//             glUniform1i(glGetUniformLocation(m_compositeShader, "scene"), 0);
//             glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_startTexture);
//             glUniform1i(glGetUniformLocation(m_compositeShader, "bloomBlur"), 1);
//             glUniform1f(glGetUniformLocation(m_compositeShader, "exposure"), 1.0f);

//             glBindVertexArray(m_quadVAO);
//             glDrawArrays(GL_TRIANGLES, 0, 6);
//             glBindVertexArray(0);
//         } else {
//             glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // Fallback Pink
//             glClear(GL_COLOR_BUFFER_BIT);
//         }
//         return;
//     }

//     // --- PHASE 1: GEOMETRY PASS ---
//     m_gbuffer.bindForWriting();
//     glViewport(0, 0, w, h);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glEnable(GL_DEPTH_TEST);

//     glUseProgram(m_gbufferShader);
//     glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "view"), 1, GL_FALSE, &m_camera.getViewMatrix()[0][0]);
//     glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "proj"), 1, GL_FALSE, &m_camera.getProjMatrix()[0][0]);
//     glUniform1i(glGetUniformLocation(m_gbufferShader, "useTexture"), 0);

//     glBindVertexArray(m_cubeVAO);
//     for (const auto& prop : m_props) {
//         glm::mat4 model = glm::translate(glm::mat4(1.f), prop.pos) * glm::scale(glm::mat4(1.f), prop.scale);
//         glUniformMatrix4fv(glGetUniformLocation(m_gbufferShader, "model"), 1, GL_FALSE, &model[0][0]);
//         glUniform3fv(glGetUniformLocation(m_gbufferShader, "albedoColor"), 1, &prop.color[0]);
//         glm::vec3 emissive = prop.color * prop.emissiveStrength;
//         glUniform3fv(glGetUniformLocation(m_gbufferShader, "emissiveColor"), 1, &emissive[0]);
//         glDrawArrays(GL_TRIANGLES, 0, m_cubeNumVerts);
//     }
//     glBindVertexArray(0);
//     GL_CHECK();

//     // --- PHASE 2: LIGHTING PASS ---
//     glDisable(GL_DEPTH_TEST); // No depth for quads

//     glBindFramebuffer(GL_FRAMEBUFFER, m_lightingFBO);
//     glViewport(0, 0, w, h);
//     glClear(GL_COLOR_BUFFER_BIT);

//     glUseProgram(m_deferredShader);
//     glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getPositionTex());
//     glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getNormalTex());
//     glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getAlbedoTex());
//     glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m_gbuffer.getEmissiveTex());
//     glUniform1i(glGetUniformLocation(m_deferredShader, "gPosition"), 0);
//     glUniform1i(glGetUniformLocation(m_deferredShader, "gNormal"), 1);
//     glUniform1i(glGetUniformLocation(m_deferredShader, "gAlbedo"), 2);
//     glUniform1i(glGetUniformLocation(m_deferredShader, "gEmissive"), 3);

//     glUniform3fv(glGetUniformLocation(m_deferredShader, "camPos"), 1, &m_camera.getPosition()[0]);

//     int numLights = std::min((int)m_lights.size(), 8);
//     glUniform1i(glGetUniformLocation(m_deferredShader, "numLights"), numLights);
//     for(int i=0; i<numLights; i++) {
//         std::string base = "lights[" + std::to_string(i) + "]";
//         glUniform1i(glGetUniformLocation(m_deferredShader, (base + ".type").c_str()), 0);
//         glUniform3fv(glGetUniformLocation(m_deferredShader, (base + ".pos").c_str()), 1, &m_lights[i].pos[0]);
//         glUniform3fv(glGetUniformLocation(m_deferredShader, (base + ".color").c_str()), 1, &m_lights[i].color[0]);
//         glUniform3f(glGetUniformLocation(m_deferredShader, (base + ".atten").c_str()), 0.5f, 0.5f, 0.1f);
//     }
//     glBindVertexArray(m_quadVAO);
//     glDrawArrays(GL_TRIANGLES, 0, 6);
//     GL_CHECK();

//     // --- PHASE 3: BLUR PASS ---
//     bool horizontal = true;
//     glUseProgram(m_blurShader);
//     for (int i = 0; i < 10; i++) {
//         glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
//         glUniform1i(glGetUniformLocation(m_blurShader, "horizontal"), horizontal);
//         glActiveTexture(GL_TEXTURE0);
//         glBindTexture(GL_TEXTURE_2D, (i==0) ? m_gbuffer.getEmissiveTex() : m_pingpongColorbuffers[!horizontal]);
//         glDrawArrays(GL_TRIANGLES, 0, 6);
//         horizontal = !horizontal;
//     }
//     GL_CHECK();

//     // --- PHASE 4: COMPOSITE ---
//     // *** FIX 2: BIND DEFAULT FBO ID (FROM START OF FRAME) ***
//     glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
//     glViewport(0, 0, w, h);
//     glClear(GL_COLOR_BUFFER_BIT);

//     glUseProgram(m_compositeShader);
//     glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_lightingTexture);
//     glUniform1i(glGetUniformLocation(m_compositeShader, "scene"), 0);
//     glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_pingpongColorbuffers[!horizontal]);
//     glUniform1i(glGetUniformLocation(m_compositeShader, "bloomBlur"), 1);
//     glUniform1f(glGetUniformLocation(m_compositeShader, "exposure"), 1.0f);

//     // *** FIX 3: BIND VAO BEFORE FINAL DRAW ***
//     glBindVertexArray(m_quadVAO);
//     glDrawArrays(GL_TRIANGLES, 0, 6);
//     glBindVertexArray(0);

//     GL_CHECK(); // This should now pass!

//     glEnable(GL_DEPTH_TEST);
// }

// // ----------------------------------------------------------------
// // BOILERPLATE
// // ----------------------------------------------------------------
// void Realtime::resizeGL(int w, int h) {
//     if(w<=0||h<=0) return;
//     glViewport(0,0,w,h);
//     int w_dpi = w*devicePixelRatio();
//     int h_dpi = h*devicePixelRatio();

//     m_gbuffer.resize(w_dpi, h_dpi);

//     glBindTexture(GL_TEXTURE_2D, m_lightingTexture);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w_dpi, h_dpi, 0, GL_RGBA, GL_FLOAT, NULL);
//     for(int i=0; i<2; i++) {
//         glBindTexture(GL_TEXTURE_2D, m_pingpongColorbuffers[i]);
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w_dpi, h_dpi, 0, GL_RGBA, GL_FLOAT, NULL);
//     }

//     float aspect = (float)w / (float)h;
//     m_camera.setProjectionMatrix(aspect, 0.1f, 100.f, glm::radians(45.f));
// }

// void Realtime::initCube() {
//     Cube cube; cube.updateParams(1, 1);
//     std::vector<float> data = cube.generateShape();
//     m_cubeNumVerts = data.size() / 6;
//     glGenVertexArrays(1, &m_cubeVAO); glBindVertexArray(m_cubeVAO);
//     glGenBuffers(1, &m_cubeVBO); glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
//     glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_STATIC_DRAW);
//     glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
//     glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
//     glBindVertexArray(0);
// }

// void Realtime::initQuad() {
//     float verts[] = {-1,-1,0,0, 1,-1,1,0, -1,1,0,1, 1,1,1,1, -1,1,0,1, 1,-1,1,0};
//     glGenVertexArrays(1, &m_quadVAO); glBindVertexArray(m_quadVAO);
//     glGenBuffers(1, &m_quadVBO); glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
//     glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
//     glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
//     glBindVertexArray(0);
// }

// GLuint Realtime::loadTexture2D(const std::string &path) {
//     QImage img(QString::fromStdString(path));
//     if (img.isNull()) return 0;
//     img = img.convertToFormat(QImage::Format_RGBA8888).mirrored();
//     GLuint tex;
//     glGenTextures(1, &tex);
//     glBindTexture(GL_TEXTURE_2D, tex);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
//     glGenerateMipmap(GL_TEXTURE_2D);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     return tex;
// }

// void Realtime::keyPressEvent(QKeyEvent *e) {
//     if(m_gameState==START_SCREEN && e->key()==Qt::Key_Space) { m_gameState=PLAYING; update(); return; }
//     m_keyMap[e->key()] = true; update();
// }
// void Realtime::mousePressEvent(QMouseEvent *e) { if(e->button()==Qt::LeftButton) { m_mouseDown=true; m_prevMousePos=glm::vec2(e->pos().x(),e->pos().y()); }}
// void Realtime::mouseReleaseEvent(QMouseEvent *e) { m_mouseDown=false; }
// void Realtime::mouseMoveEvent(QMouseEvent *e) {
//     if(!m_mouseDown) return;
//     glm::vec2 cur(e->pos().x(), e->pos().y());
//     glm::vec2 d = cur - m_prevMousePos; m_prevMousePos = cur;
//     m_camera.rotateAroundUp(-d.x * 0.005f); m_camera.rotateAroundRight(-d.y * 0.005f);
//     update();
// }
// // Stubs
// void Realtime::sceneChanged(){} void Realtime::settingsChanged(){} void Realtime::saveViewportImage(const std::string&){} void Realtime::timerEvent(QTimerEvent*){} void Realtime::initTerrain(){}


