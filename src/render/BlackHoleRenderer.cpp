#include "render/BlackHoleRenderer.h"

#include "core/BlackHoleSim.h"
#include "render/CameraUniforms.h"

#include <glad/glad.h>

#include <algorithm>

namespace render {

BlackHoleRenderer::BlackHoleRenderer() : m_shader("fullscreen.vert.glsl", "black_hole.frag.glsl") {}

void BlackHoleRenderer::draw(const core::Simulation& base, int width, int height) {
    const auto& sim = static_cast<const core::BlackHoleSim&>(base);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_shader.use();
    setCameraUniforms(m_shader, sim.camera, width, height);
    m_shader.set("uMass", sim.mass);
    m_shader.set("uRs", sim.schwarzschildRadius());
    m_shader.set("uHorizon", sim.horizonRadius());
    m_shader.set("uIsco", sim.iscoRadius());
    m_shader.set("uDiskOuter", sim.diskOuterRadius());
    m_shader.set("uSpin", sim.spin);
    m_shader.set("uDiskTime", sim.diskTime());
    m_shader.set("uDiskBrightness", sim.diskBrightness);
    m_shader.set("uEscape", std::max(sim.camera.distance * 1.1f, 60.f));
    m_triangle.draw();
}

} // namespace render
