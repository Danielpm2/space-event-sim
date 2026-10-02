#pragma once

#include "core/PostFxSettings.h"
#include "render/FullscreenTriangle.h"
#include "render/Shader.h"

#include <glad/glad.h>

namespace render {

// Owns the HDR scene target and the composite pass. Simulations render
// between beginScene() and apply(); ImGui is drawn afterwards by the caller.
class PostProcess {
public:
    PostProcess();
    ~PostProcess();
    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    // Binds the HDR target and clears it. Returns false (and leaves the default
    // framebuffer bound) if post-processing is unavailable this frame.
    bool beginScene(int width, int height);
    // Tone-maps the scene into the default framebuffer.
    void apply(const core::PostFxSettings& settings, int width, int height);

private:
    struct Target {
        GLuint fbo = 0, tex = 0, depth = 0;
        int w = 0, h = 0;
    };

    bool createTarget(Target& t, int w, int h, bool withDepth, const char* name);
    void destroyTarget(Target& t);
    // Bright pass + blur into m_ping[0]; returns false if unavailable.
    bool runBloom(const core::PostFxSettings& settings);

    Shader m_composite;
    Shader m_bright;
    Shader m_blur;
    FullscreenTriangle m_triangle;
    Target m_scene;
    Target m_ping[2]; // half resolution
    bool m_failed = false;
    bool m_bloomFailed = false;
    bool m_active = false;
};

} // namespace render
