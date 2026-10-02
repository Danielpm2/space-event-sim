#include "render/PostProcess.h"

#include "render/GlCheck.h"

#include <cstdio>

namespace render {

PostProcess::PostProcess() : m_composite("fullscreen.vert.glsl", "composite.frag.glsl") {}

PostProcess::~PostProcess() { destroyTarget(m_scene); }

void PostProcess::destroyTarget(Target& t) {
    if (t.fbo)
        glDeleteFramebuffers(1, &t.fbo);
    if (t.tex)
        glDeleteTextures(1, &t.tex);
    if (t.depth)
        glDeleteRenderbuffers(1, &t.depth);
    t = {};
}

bool PostProcess::createTarget(Target& t, int w, int h, bool withDepth, const char* name) {
    destroyTarget(t);
    t.w = w;
    t.h = h;

    glGenTextures(1, &t.tex);
    glBindTexture(GL_TEXTURE_2D, t.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &t.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);

    if (withDepth) {
        glGenRenderbuffers(1, &t.depth);
        glBindRenderbuffer(GL_RENDERBUFFER, t.depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t.depth);
    }

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    const bool glError = checkGl(name);
    if (status != GL_FRAMEBUFFER_COMPLETE || glError) {
        std::fprintf(stderr, "[PostProcess] framebuffer '%s' %dx%d failed: %s\n", name, w, h,
                     framebufferStatusName(status));
        destroyTarget(t);
        return false;
    }
    return true;
}

bool PostProcess::beginScene(int width, int height) {
    m_active = false;
    if (width <= 0 || height <= 0)
        return false;
    if (m_failed)
        return false;

    if (m_scene.fbo == 0 || m_scene.w != width || m_scene.h != height) {
        if (!createTarget(m_scene, width, height, true, "scene")) {
            std::fprintf(stderr, "[PostProcess] disabled; rendering directly to the window\n");
            m_failed = true;
            return false;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_scene.fbo);
    glViewport(0, 0, width, height);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_active = true;
    return true;
}

void PostProcess::apply(const core::PostFxSettings& settings, int width, int height) {
    if (!m_active)
        return;
    m_active = false;

    checkGl("scene pass");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_composite.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scene.tex);
    m_composite.set("uScene", 0);
    m_composite.set("uExposure", settings.exposure);
    m_triangle.draw();

    checkGl("PostProcess::apply");
}

} // namespace render
