#include "render/PostProcess.h"

#include "render/GlCheck.h"

#include <algorithm>
#include <cstdio>

namespace render {

PostProcess::PostProcess()
    : m_composite("fullscreen.vert.glsl", "composite.frag.glsl"),
      m_bright("fullscreen.vert.glsl", "bright_pass.frag.glsl"),
      m_blur("fullscreen.vert.glsl", "blur.frag.glsl") {}

PostProcess::~PostProcess() {
    destroyTarget(m_scene);
    destroyTarget(m_ping[0]);
    destroyTarget(m_ping[1]);
}

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
    // After a failure, try again only when the size changes.
    if (m_failed && width == m_failedW && height == m_failedH)
        return false;
    m_failed = false;

    if (m_scene.fbo == 0 || m_scene.w != width || m_scene.h != height) {
        if (!createTarget(m_scene, width, height, true, "scene")) {
            std::fprintf(stderr, "[PostProcess] disabled at %dx%d; rendering directly to the window\n", width, height);
            m_failed = true;
            m_failedW = width;
            m_failedH = height;
            destroyTarget(m_ping[0]);
            destroyTarget(m_ping[1]);
            return false;
        }
        const int hw = std::max(1, width / 2), hh = std::max(1, height / 2);
        m_bloomFailed = !createTarget(m_ping[0], hw, hh, false, "bloom A") ||
                        !createTarget(m_ping[1], hw, hh, false, "bloom B");
        if (m_bloomFailed)
            std::fprintf(stderr, "[PostProcess] bloom disabled\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_scene.fbo);
    glViewport(0, 0, width, height);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_active = true;
    return true;
}

bool PostProcess::runBloom(const core::PostFxSettings& settings) {
    if (m_bloomFailed || m_ping[0].fbo == 0 || m_ping[1].fbo == 0)
        return false;

    const int w = m_ping[0].w, h = m_ping[0].h;
    glViewport(0, 0, w, h);
    glActiveTexture(GL_TEXTURE0);

    m_bright.use();
    m_bright.set("uScene", 0);
    m_bright.set("uThreshold", settings.threshold);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ping[0].fbo);
    glBindTexture(GL_TEXTURE_2D, m_scene.tex);
    m_triangle.draw();

    m_blur.use();
    m_blur.set("uImage", 0);
    const int passes = std::clamp(settings.blurPasses, 1, 16);
    for (int i = 0; i < passes; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ping[1].fbo);
        glBindTexture(GL_TEXTURE_2D, m_ping[0].tex);
        m_blur.set("uDirection", glm::vec2(1.f / w, 0.f));
        m_triangle.draw();

        glBindFramebuffer(GL_FRAMEBUFFER, m_ping[0].fbo);
        glBindTexture(GL_TEXTURE_2D, m_ping[1].tex);
        m_blur.set("uDirection", glm::vec2(0.f, 1.f / h));
        m_triangle.draw();
    }
    return true;
}

void PostProcess::apply(const core::PostFxSettings& settings, int width, int height) {
    if (!m_active)
        return;
    m_active = false;

    checkGl("scene pass");
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    const bool bloom = settings.bloomEnabled && runBloom(settings);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    m_composite.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scene.tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloom ? m_ping[0].tex : m_scene.tex);
    glActiveTexture(GL_TEXTURE0);
    m_composite.set("uScene", 0);
    m_composite.set("uBloom", 1);
    m_composite.set("uBloomIntensity", bloom ? settings.intensity : 0.f);
    m_composite.set("uExposure", settings.exposure);
    m_composite.set("uVignette", settings.vignette);
    m_composite.set("uAberration", settings.aberration);
    m_triangle.draw();

    checkGl("PostProcess::apply");
}

} // namespace render
