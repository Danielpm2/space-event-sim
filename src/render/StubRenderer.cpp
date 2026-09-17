#include "render/StubRenderer.h"

#include "core/StubSim.h"

namespace render {

StubRenderer::StubRenderer() : m_shader("fullscreen.vert.glsl", "stub.frag.glsl") {}

void StubRenderer::draw(const core::Simulation& sim, int width, int height) {
    const auto& stub = static_cast<const core::StubSim&>(sim);
    m_shader.use();
    m_shader.set("uResolution", glm::vec2(width, height));
    m_shader.set("uTime", static_cast<float>(stub.time()));
    m_shader.set("uHue", stub.hue);
    m_triangle.draw();
}

} // namespace render
