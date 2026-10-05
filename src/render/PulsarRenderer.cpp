#include "render/PulsarRenderer.h"

#include "core/PulsarSim.h"
#include "render/CameraUniforms.h"
#include "render/SkyTexture.h"

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
      m_particles("particles.vert.glsl", "particles.frag.glsl") {}

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
    bindSky(m_sky, height, cam.fovY);
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
    m_sphere.draw();

    // Everything below is additive light.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);

    // Beams: two nested cones per pole (wide dim shell + narrow bright core).
    m_beam.use();
    m_beam.set("uViewProj", viewProj);
    m_beam.set("uCamPos", camPos);
    m_beam.set("uLength", core::PulsarSim::kBeamLength);
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
            m_cone.draw();
        }
    }

    // Particles
    const auto& ps = sim.particles();
    if (!ps.empty()) {
        std::vector<float> buf;
        buf.reserve(ps.size() * 4);
        for (const auto& p : ps)
            buf.insert(buf.end(), {p.position.x, p.position.y, p.position.z, 1.f - p.age / p.lifetime});
        m_particleBuffer.upload(buf);

        glEnable(GL_PROGRAM_POINT_SIZE);
        m_particles.use();
        m_particles.set("uViewProj", viewProj);
        m_particles.set("uPointSize", 0.1f);
        m_particles.set("uScreenHeight", static_cast<float>(height));
        m_particles.set("uGlow", sim.glow);
        m_particles.set("uTintNew", glm::vec3(0.8f, 0.95f, 1.f));
        m_particles.set("uTintOld", glm::vec3(0.25f, 0.45f, 1.f));
        m_particleBuffer.draw(GL_POINTS);
    }

    // Halo around the star, flaring when a beam sweeps across the viewer.
    glDisable(GL_DEPTH_TEST);
    m_halo.use();
    m_halo.set("uViewProj", viewProj);
    m_halo.set("uCenter", glm::vec3(0.f));
    m_halo.set("uRight", cam.right());
    m_halo.set("uUp", cam.up());
    m_halo.set("uSize", 5.f);
    m_halo.set("uColor", glm::vec3(0.25f, 0.5f, 1.f) * (0.5f * sim.glow));
    m_quad.draw();
    m_halo.set("uSize", 11.f);
    m_halo.set("uColor", glm::vec3(0.7f, 0.85f, 1.f) * (1.4f * flash * sim.glow));
    m_quad.draw();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

} // namespace render
