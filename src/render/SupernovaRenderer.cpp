#include "render/SupernovaRenderer.h"

#include "core/SupernovaSim.h"
#include "render/CameraUniforms.h"
#include "render/SkyTexture.h"

#include <glad/glad.h>

namespace render {

SupernovaRenderer::SupernovaRenderer() : m_shader("fullscreen.vert.glsl", "supernova.frag.glsl") {}

void SupernovaRenderer::draw(const core::Simulation& base, int width, int height) {
    const auto& sim = static_cast<const core::SupernovaSim&>(base);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_shader.use();
    setCameraUniforms(m_shader, sim.camera, width, height);
    bindSky(m_shader, height, sim.camera.fovY);
    m_shader.set("uTime", sim.time());
    m_shader.set("uStarRadius", sim.progenitorRadius());
    m_shader.set("uStarBright", sim.starBrightness());
    m_shader.set("uFlash", sim.flash());
    m_shader.set("uShockR", sim.shockRadius());
    m_shader.set("uThick", sim.shellThickness());
    m_shader.set("uShellBright", sim.shellBrightness());
    m_shader.set("uClump", sim.clumpiness);
    m_shader.set("uFade", sim.fade());
    m_shader.set("uRemnant", sim.remnantGlow());
    m_triangle.draw();
}

} // namespace render
