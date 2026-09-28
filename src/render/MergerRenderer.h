#pragma once

#include "render/FullscreenTriangle.h"
#include "render/Primitives.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"

namespace render {

class MergerRenderer : public SimRenderer {
public:
    MergerRenderer();
    void draw(const core::Simulation& sim, int width, int height) override;

private:
    Shader m_sky;
    Shader m_body;
    Shader m_grid;
    Shader m_beam;
    Shader m_halo;
    Shader m_particles;
    FullscreenTriangle m_fullscreen;
    SphereMesh m_sphere;
    ConeMesh m_cone;
    StreamBuffer4 m_gridBuffer;
    StreamBuffer4 m_particleBuffer;
    QuadStrip m_quad;
};

} // namespace render
