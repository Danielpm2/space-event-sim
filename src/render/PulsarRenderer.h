#pragma once

#include "render/FullscreenTriangle.h"
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
    void buildSphere();
    void buildCone();

    Shader m_sky;
    Shader m_star;
    Shader m_beam;
    Shader m_halo;
    Shader m_particles;
    FullscreenTriangle m_fullscreen;

    GLuint m_sphereVao = 0, m_sphereVbo = 0, m_sphereEbo = 0;
    GLsizei m_sphereIndexCount = 0;
    GLuint m_coneVao = 0, m_coneVbo = 0, m_coneEbo = 0;
    GLsizei m_coneIndexCount = 0;
    GLuint m_particleVao = 0, m_particleVbo = 0;
    GLuint m_emptyVao = 0;
};

} // namespace render
