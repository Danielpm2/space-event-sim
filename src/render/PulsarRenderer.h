#pragma once

#include "render/FullscreenTriangle.h"
#include "render/Primitives.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"

#include <glad/glad.h>

namespace render {

class PulsarRenderer : public SimRenderer {
public:
    PulsarRenderer();
    ~PulsarRenderer() override;
    PulsarRenderer(const PulsarRenderer&) = delete;
    PulsarRenderer& operator=(const PulsarRenderer&) = delete;

    void draw(const core::Simulation& sim, int width, int height) override;

private:
    void buildCone();

    Shader m_sky;
    Shader m_star;
    Shader m_beam;
    Shader m_halo;
    Shader m_particles;
    FullscreenTriangle m_fullscreen;
    SphereMesh m_sphere;
    StreamBuffer4 m_particleBuffer;
    QuadStrip m_quad;

    GLuint m_coneVao = 0, m_coneVbo = 0, m_coneEbo = 0;
    GLsizei m_coneIndexCount = 0;
};

} // namespace render
