#include "render/MagnetarRenderer.h"

#include "core/MagnetarSim.h"
#include "render/CameraUniforms.h"
#include "render/SkyTexture.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace render {

MagnetarRenderer::MagnetarRenderer()
    : m_sky("fullscreen.vert.glsl", "sky.frag.glsl"),
      m_star("pulsar.vert.glsl", "pulsar.frag.glsl"),
      m_lines("fieldline.vert.glsl", "fieldline.frag.glsl"),
      m_shell("body.vert.glsl", "shell.frag.glsl"),
      m_halo("halo.vert.glsl", "halo.frag.glsl"),
      m_particles("particles.vert.glsl", "particles.frag.glsl") {}

void MagnetarRenderer::draw(const core::Simulation& base, int width, int height) {
    const auto& sim = static_cast<const core::MagnetarSim&>(base);
    const core::OrbitCamera& cam = sim.camera;
    const glm::mat4 viewProj = cam.projection(static_cast<float>(width) / height) * cam.view();
    const glm::vec3 camPos = cam.position();
    const float flash = sim.flash();

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
    m_star.set("uRadius", core::MagnetarSim::kStarRadius);
    m_star.set("uCamPos", camPos);
    m_star.set("uMagAxis", sim.magneticAxis());
    m_star.set("uPhase", sim.phase());
    m_star.set("uGlow", 1.f);
    m_star.set("uHotspot", sim.hotspotDirection());
    m_star.set("uHotspotStrength", sim.hotspotStrength());
    m_sphere.draw();

    // Additive light from here on.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);

    // Field lines
    std::vector<float> verts;
    const auto lines = sim.fieldLines();
    if (!lines.empty())
        verts.reserve(lines.size() * (lines[0].size() - 1) * 8);
    for (size_t id = 0; id < lines.size(); ++id) {
        const auto& pts = lines[id];
        for (size_t i = 0; i + 1 < pts.size(); ++i) {
            const float u0 = static_cast<float>(i) / (pts.size() - 1);
            const float u1 = static_cast<float>(i + 1) / (pts.size() - 1);
            verts.insert(verts.end(), {pts[i].x, pts[i].y, pts[i].z, static_cast<float>(id) + u0 * 0.999f});
            verts.insert(verts.end(), {pts[i + 1].x, pts[i + 1].y, pts[i + 1].z, static_cast<float>(id) + u1 * 0.999f});
        }
    }
    m_lineBuffer.upload(verts);
    m_lines.use();
    m_lines.set("uViewProj", viewProj);
    m_lines.set("uTime", sim.time());
    m_lines.set("uFlare", std::min(flash, 1.f));
    m_lines.set("uBrightness", 0.6f + 0.05f * sim.bField + 1.5f * flash);
    m_lineBuffer.draw(GL_LINES);

    // Flare front
    if (sim.shellStrength() > 0.01f) {
        const glm::mat4 model = glm::scale(glm::mat4(1.f), glm::vec3(sim.shellRadius()));
        m_shell.use();
        m_shell.set("uViewProj", viewProj);
        m_shell.set("uModel", model);
        m_shell.set("uNormalMat", glm::inverseTranspose(glm::mat3(model)));
        m_shell.set("uCamPos", camPos);
        m_shell.set("uColor", glm::vec3(1.f, 0.6f, 0.3f));
        m_shell.set("uStrength", 0.7f * sim.shellStrength());
        m_sphere.draw();
    }

    // Flare ejecta
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
        m_particles.set("uPointSize", 0.12f);
        m_particles.set("uScreenHeight", static_cast<float>(height));
        m_particles.set("uGlow", 1.2f);
        m_particles.set("uTintNew", glm::vec3(1.f, 0.9f, 0.7f));
        m_particles.set("uTintOld", glm::vec3(0.9f, 0.25f, 0.1f));
        m_particleBuffer.draw(GL_POINTS);
    }

    // Halos
    glDisable(GL_DEPTH_TEST);
    m_halo.use();
    m_halo.set("uViewProj", viewProj);
    m_halo.set("uCenter", glm::vec3(0.f));
    m_halo.set("uRight", cam.right());
    m_halo.set("uUp", cam.up());
    m_halo.set("uSize", 4.f);
    m_halo.set("uColor", glm::vec3(0.35f, 0.45f, 1.f) * 0.45f);
    m_quad.draw();
    if (flash > 0.01f) {
        m_halo.set("uSize", 12.f);
        m_halo.set("uColor", glm::vec3(1.f, 0.85f, 0.65f) * (0.9f * flash));
        m_quad.draw();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

} // namespace render
