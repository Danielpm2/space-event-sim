#include "render/PulsarRenderer.h"

#include "core/PulsarSim.h"
#include "render/CameraUniforms.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace render {

PulsarRenderer::PulsarRenderer()
    : m_sky("fullscreen.vert.glsl", "sky.frag.glsl"),
      m_star("pulsar.vert.glsl", "pulsar.frag.glsl"),
      m_beam("beam.vert.glsl", "beam.frag.glsl"),
      m_halo("halo.vert.glsl", "halo.frag.glsl"),
      m_particles("particles.vert.glsl", "particles.frag.glsl") {
    buildSphere();
    buildCone();

    glGenVertexArrays(1, &m_particleVao);
    glGenBuffers(1, &m_particleVbo);
    glBindVertexArray(m_particleVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    glGenVertexArrays(1, &m_emptyVao);
    glBindVertexArray(0);
}

PulsarRenderer::~PulsarRenderer() {
    glDeleteVertexArrays(1, &m_sphereVao);
    glDeleteBuffers(1, &m_sphereVbo);
    glDeleteBuffers(1, &m_sphereEbo);
    glDeleteVertexArrays(1, &m_coneVao);
    glDeleteBuffers(1, &m_coneVbo);
    glDeleteBuffers(1, &m_coneEbo);
    glDeleteVertexArrays(1, &m_particleVao);
    glDeleteBuffers(1, &m_particleVbo);
    glDeleteVertexArrays(1, &m_emptyVao);
}

void PulsarRenderer::buildSphere() {
    const int stacks = 32, sectors = 64;
    std::vector<float> v;
    std::vector<unsigned> idx;
    for (int i = 0; i <= stacks; ++i) {
        const float phi = glm::pi<float>() * i / stacks;
        for (int j = 0; j <= sectors; ++j) {
            const float th = 2.f * glm::pi<float>() * j / sectors;
            v.push_back(std::sin(phi) * std::cos(th));
            v.push_back(std::cos(phi));
            v.push_back(std::sin(phi) * std::sin(th));
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            const unsigned a = i * (sectors + 1) + j, b = a + sectors + 1;
            idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    m_sphereIndexCount = static_cast<GLsizei>(idx.size());

    glGenVertexArrays(1, &m_sphereVao);
    glGenBuffers(1, &m_sphereVbo);
    glGenBuffers(1, &m_sphereEbo);
    glBindVertexArray(m_sphereVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

void PulsarRenderer::buildCone() {
    const int around = 48, along = 16;
    std::vector<float> v;
    std::vector<unsigned> idx;
    for (int i = 0; i <= along; ++i) {
        for (int j = 0; j <= around; ++j) {
            v.push_back(2.f * glm::pi<float>() * j / around);
            v.push_back(static_cast<float>(i) / along);
        }
    }
    for (int i = 0; i < along; ++i) {
        for (int j = 0; j < around; ++j) {
            const unsigned a = i * (around + 1) + j, b = a + around + 1;
            idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    m_coneIndexCount = static_cast<GLsizei>(idx.size());

    glGenVertexArrays(1, &m_coneVao);
    glGenBuffers(1, &m_coneVbo);
    glGenBuffers(1, &m_coneEbo);
    glBindVertexArray(m_coneVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_coneVbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_coneEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

void PulsarRenderer::draw(const core::Simulation& base, int width, int height) {
    const auto& sim = static_cast<const core::PulsarSim&>(base);
    const core::OrbitCamera& cam = sim.camera;
    const glm::mat4 viewProj = cam.projection(static_cast<float>(width) / height) * cam.view();
    const glm::vec3 magAxis = sim.magneticAxis();
    const glm::vec3 camPos = cam.position();
    const float flash = sim.pulseIntensity(glm::normalize(camPos));

    // Sky
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    m_sky.use();
    setCameraUniforms(m_sky, cam, width, height);
    m_fullscreen.draw();

    // Star
    glEnable(GL_DEPTH_TEST);
    m_star.use();
    m_star.set("uViewProj", viewProj);
    m_star.set("uRadius", core::PulsarSim::kStarRadius);
    m_star.set("uCamPos", camPos);
    m_star.set("uMagAxis", magAxis);
    m_star.set("uPhase", sim.phase());
    m_star.set("uGlow", sim.glow);
    glBindVertexArray(m_sphereVao);
    glDrawElements(GL_TRIANGLES, m_sphereIndexCount, GL_UNSIGNED_INT, nullptr);

    // Everything below is additive light.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);

    // Beams: two nested cones per pole (wide dim shell + narrow bright core).
    m_beam.use();
    m_beam.set("uViewProj", viewProj);
    m_beam.set("uCamPos", camPos);
    m_beam.set("uLength", core::PulsarSim::kBeamLength);
    glBindVertexArray(m_coneVao);
    const float tanHalf = std::tan(sim.beamHalfAngle());
    const glm::vec3 helper = std::abs(magAxis.y) < 0.9f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
    for (float sign : {1.f, -1.f}) {
        const glm::vec3 axis = sign * magAxis;
        const glm::vec3 u = glm::normalize(glm::cross(axis, helper));
        m_beam.set("uAxis", axis);
        m_beam.set("uU", u);
        m_beam.set("uV", glm::cross(axis, u));
        for (int layer = 0; layer < 2; ++layer) {
            const bool inner = layer == 1;
            m_beam.set("uTanHalf", tanHalf * (inner ? 0.4f : 1.f));
            m_beam.set("uColor", inner ? glm::vec3(0.75f, 0.9f, 1.f) : glm::vec3(0.3f, 0.55f, 1.f));
            m_beam.set("uIntensity", (inner ? 0.7f : 0.3f) * sim.glow);
            m_beam.set("uFlash", flash * (inner ? 0.5f : 0.2f));
            glDrawElements(GL_TRIANGLES, m_coneIndexCount, GL_UNSIGNED_INT, nullptr);
        }
    }

    // Particles
    const auto& ps = sim.particles();
    if (!ps.empty()) {
        std::vector<float> buf;
        buf.reserve(ps.size() * 4);
        for (const auto& p : ps) {
            buf.insert(buf.end(), {p.position.x, p.position.y, p.position.z, 1.f - p.age / p.lifetime});
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_particleVbo);
        glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(float), buf.data(), GL_STREAM_DRAW);

        glEnable(GL_PROGRAM_POINT_SIZE);
        m_particles.use();
        m_particles.set("uViewProj", viewProj);
        m_particles.set("uPointSize", 0.1f);
        m_particles.set("uScreenHeight", static_cast<float>(height));
        m_particles.set("uGlow", sim.glow);
        glBindVertexArray(m_particleVao);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(ps.size()));
    }

    // Halo around the star, flaring when a beam sweeps across the viewer.
    glDisable(GL_DEPTH_TEST);
    m_halo.use();
    m_halo.set("uViewProj", viewProj);
    m_halo.set("uCenter", glm::vec3(0.f));
    m_halo.set("uRight", cam.right());
    m_halo.set("uUp", cam.up());
    glBindVertexArray(m_emptyVao);
    m_halo.set("uSize", 5.f);
    m_halo.set("uColor", glm::vec3(0.25f, 0.5f, 1.f) * (0.5f * sim.glow));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_halo.set("uSize", 11.f);
    m_halo.set("uColor", glm::vec3(0.7f, 0.85f, 1.f) * (1.4f * flash * sim.glow));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

} // namespace render
