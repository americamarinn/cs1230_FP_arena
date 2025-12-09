#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>

class GBuffer {
public:
    GBuffer();
    ~GBuffer();

    void init(int width, int height);
    void resize(int width, int height);
    void bindForWriting();

    // Getters for textures
    GLuint getPositionTex() const { return m_positionTex; }
    GLuint getNormalTex()   const { return m_normalTex; }
    GLuint getAlbedoTex()   const { return m_albedoTex; }
    GLuint getEmissiveTex() const { return m_emissiveTex; }
    GLuint getDepthTex()    const { return m_depthTex; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void createTextures(int width, int height);
    void createDepth(int width, int height);
    void destroy(); // Helper to clean up

    GLuint m_fbo = 0;
    GLuint m_positionTex = 0;
    GLuint m_normalTex   = 0;
    GLuint m_albedoTex   = 0;
    GLuint m_emissiveTex = 0;
    GLuint m_depthTex    = 0;

    int m_width = 0;
    int m_height = 0;
};
