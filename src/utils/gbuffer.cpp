#include "gbuffer.h"
#include <iostream>

GBuffer::GBuffer() {
}

GBuffer::~GBuffer() {
    destroy();
}

void GBuffer::init(int width, int height) {
    if (width <= 0 || height <= 0) return;

    m_width = width;
    m_height = height;

    destroy();

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    createTextures(width, height);
    createDepth(width, height);

    GLenum attachments[4] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(4, attachments);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "❌ GBuffer Incomplete: 0x" << std::hex << status << std::dec << std::endl;
    } else {
        std::cout << "✅ GBuffer Initialized (Safe Mode: GL_RGBA)" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (m_width == width && m_height == height) return;
    init(width, height);
}

void GBuffer::bindForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void GBuffer::createTextures(int width, int height) {
    // SAFE MODE: Use GL_RGBA (Standard 8-bit) instead of GL_RGBA16F
    // This fixes the "Unloadable Texture" error on some Macs

    // Position
    glGenTextures(1, &m_positionTex);
    glBindTexture(GL_TEXTURE_2D, m_positionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_positionTex, 0);

    // Normal
    glGenTextures(1, &m_normalTex);
    glBindTexture(GL_TEXTURE_2D, m_normalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normalTex, 0);

    // Albedo
    glGenTextures(1, &m_albedoTex);
    glBindTexture(GL_TEXTURE_2D, m_albedoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_albedoTex, 0);

    // Emissive
    glGenTextures(1, &m_emissiveTex);
    glBindTexture(GL_TEXTURE_2D, m_emissiveTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_emissiveTex, 0);
}

void GBuffer::createDepth(int width, int height) {
    glGenTextures(1, &m_depthTex);
    glBindTexture(GL_TEXTURE_2D, m_depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);
}

void GBuffer::destroy() {
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_positionTex) glDeleteTextures(1, &m_positionTex);
    if (m_normalTex) glDeleteTextures(1, &m_normalTex);
    if (m_albedoTex) glDeleteTextures(1, &m_albedoTex);
    if (m_emissiveTex) glDeleteTextures(1, &m_emissiveTex);
    if (m_depthTex) glDeleteTextures(1, &m_depthTex);

    m_fbo = 0; m_positionTex = 0; m_normalTex = 0;
    m_albedoTex = 0; m_emissiveTex = 0; m_depthTex = 0;
}

// #include "gbuffer.h"
// #include <iostream>

// GBuffer::GBuffer() {
// }

// GBuffer::~GBuffer() {
//     destroy();
// }

// void GBuffer::init(int width, int height) {
//     // 1. Store dimensions
//     m_width = width;
//     m_height = height;

//     // 2. Create the Framebuffer
//     glGenFramebuffers(1, &m_fbo);
//     glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

//     // 3. Create and Attach Textures
//     createTextures(width, height);
//     createDepth(width, height);

//     // 4. Tell OpenGL we will draw to these 4 attachments
//     GLenum attachments[4] = {
//         GL_COLOR_ATTACHMENT0, // Position
//         GL_COLOR_ATTACHMENT1, // Normal
//         GL_COLOR_ATTACHMENT2, // Albedo
//         GL_COLOR_ATTACHMENT3  // Emissive
//     };
//     glDrawBuffers(4, attachments);

//     // 5. Verify Completeness
//     GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//     if (status != GL_FRAMEBUFFER_COMPLETE) {
//         std::cerr << "❌ GBuffer FBO Incomplete! Status: 0x" << std::hex << status << std::dec << std::endl;
//     }

//     // 6. Unbind
//     glBindFramebuffer(GL_FRAMEBUFFER, 0);
// }

// void GBuffer::resize(int width, int height) {
//     // Avoid resizing if dimensions haven't changed (or are invalid)
//     if (m_width == width && m_height == height) return;
//     if (width <= 0 || height <= 0) return;

//     // Full cleanup and re-initialization
//     destroy();
//     init(width, height);
// }

// void GBuffer::destroy() {
//     if (m_fbo) {
//         glDeleteFramebuffers(1, &m_fbo);
//         m_fbo = 0;
//     }
//     if (m_positionTex) {
//         glDeleteTextures(1, &m_positionTex);
//         m_positionTex = 0;
//     }
//     if (m_normalTex) {
//         glDeleteTextures(1, &m_normalTex);
//         m_normalTex = 0;
//     }
//     if (m_albedoTex) {
//         glDeleteTextures(1, &m_albedoTex);
//         m_albedoTex = 0;
//     }
//     if (m_emissiveTex) {
//         glDeleteTextures(1, &m_emissiveTex);
//         m_emissiveTex = 0;
//     }
//     if (m_depthTex) {
//         glDeleteTextures(1, &m_depthTex);
//         m_depthTex = 0;
//     }
// }

// void GBuffer::bindForWriting() {
//     glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
// }

// void GBuffer::createTextures(int width, int height) {
//     // --- Position (High Precision) ---
//     glGenTextures(1, &m_positionTex);
//     glBindTexture(GL_TEXTURE_2D, m_positionTex);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_positionTex, 0);

//     // --- Normal (High Precision) ---
//     glGenTextures(1, &m_normalTex);
//     glBindTexture(GL_TEXTURE_2D, m_normalTex);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normalTex, 0);

//     // --- Albedo (Standard Precision is usually fine, but 16F is safer for math) ---
//     glGenTextures(1, &m_albedoTex);
//     glBindTexture(GL_TEXTURE_2D, m_albedoTex);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_albedoTex, 0);

//     // --- Emissive ---
//     glGenTextures(1, &m_emissiveTex);
//     glBindTexture(GL_TEXTURE_2D, m_emissiveTex);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_emissiveTex, 0);
// }

// void GBuffer::createDepth(int width, int height) {
//     glGenTextures(1, &m_depthTex);
//     glBindTexture(GL_TEXTURE_2D, m_depthTex);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);
// }
