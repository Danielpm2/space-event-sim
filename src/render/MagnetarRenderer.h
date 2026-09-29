#pragma once

#include "render/FullscreenTriangle.h"
#include "render/Primitives.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"

namespace render {

class MagnetarRenderer : public SimRenderer {
public:
    MagnetarRenderer();
    void draw(const core::Simulation& sim, int width, int height) override;

private:
    Shader m_sky;
    Shader m_star;
    Shader m_lines;
    Shader m_shell;
    Shader m_halo;
    Shader m_particles;
    FullscreenTriangle m_fullscreen;
    SphereMesh m_sphere;
    StreamBuffer4 m_lineBuffer;
    StreamBuffer4 m_particleBuffer;
    QuadStrip m_quad;
};

} // namespace render
