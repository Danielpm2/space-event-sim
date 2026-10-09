#include "render/ApproachFx.h"

#include <glad/glad.h>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace render {

namespace {
constexpr int kCount = 1800;
constexpr float kExtent = 14.f;      // side of the cube of dust around the camera
constexpr float kStreakSeconds = 0.05f;
} // namespace

ApproachFx::ApproachFx() : m_shader("dust.vert.glsl", "dust.frag.glsl") {
    std::mt19937 rng(9001u);
    std::uniform_real_distribution<float> u(0.f, kExtent);
    m_points.reserve(kCount);
    for (int i = 0; i < kCount; ++i)
        m_points.emplace_back(u(rng), u(rng), u(rng));
    m_verts.reserve(static_cast<size_t>(kCount) * 8);
}

void ApproachFx::draw(const core::OrbitCamera& cam, const glm::vec3& velocity, float aspect, float intensity) {
    if (intensity <= 0.001f)
        return;

    const glm::vec3 camPos = cam.position();
    const glm::vec3 streak = velocity * kStreakSeconds;
    const float half = kExtent * 0.5f;

    m_verts.clear();
    for (const glm::vec3& p : m_points) {
        // Wrap each mote into the cube centred on the camera so the field never runs out.
        const glm::vec3 rel = (glm::fract((p - camPos) / kExtent + 0.5f) - 0.5f) * kExtent;
        const float m = std::max({std::abs(rel.x), std::abs(rel.y), std::abs(rel.z)}) / half;
        const float fade = (1.f - glm::smoothstep(0.7f, 1.f, m)) * glm::smoothstep(0.03f, 0.12f, glm::length(rel) / half);
        if (fade <= 0.f)
            continue;
        const glm::vec3 head = camPos + rel;
        const glm::vec3 tail = head + streak;
        m_verts.insert(m_verts.end(), {head.x, head.y, head.z, fade * intensity});
        m_verts.insert(m_verts.end(), {tail.x, tail.y, tail.z, 0.f});
    }
    if (m_verts.empty())
        return;
    m_buffer.upload(m_verts);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    m_shader.use();
    m_shader.set("uViewProj", cam.projection(aspect) * cam.view());
    m_shader.set("uColor", glm::vec3(0.75f, 0.82f, 1.f));
    m_buffer.draw(GL_LINES);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

} // namespace render
