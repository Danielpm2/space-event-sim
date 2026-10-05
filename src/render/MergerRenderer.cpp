#include "render/MergerRenderer.h"

#include "core/MergerSim.h"
#include "render/CameraUniforms.h"
#include "render/SkyTexture.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace render {

namespace {

constexpr int kGridLines = 49;
constexpr int kGridSamples = 97;

void buildGrid(const core::MergerSim& sim, float fade, std::vector<float>& out) {
    const float ext = core::MergerSim::kExtent;
    out.clear();
    out.reserve(static_cast<size_t>(kGridLines) * 2 * (kGridSamples - 1) * 2 * 4);

    auto vertex = [&](float x, float z) {
        const float r = std::sqrt(x * x + z * z) / ext;
        const float edge = std::clamp((1.f - r) / 0.45f, 0.f, 1.f);
        return std::array<float, 4>{x, sim.spacetimeHeight(x, z), z, edge * edge * 0.75f * fade};
    };

    for (int axis = 0; axis < 2; ++axis) {
        for (int i = 0; i < kGridLines; ++i) {
            const float c = -ext + 2.f * ext * i / (kGridLines - 1);
            std::array<float, 4> prev{};
            for (int j = 0; j < kGridSamples; ++j) {
                const float s = -ext + 2.f * ext * j / (kGridSamples - 1);
                const auto cur = axis == 0 ? vertex(s, c) : vertex(c, s);
                if (j > 0) {
                    out.insert(out.end(), prev.begin(), prev.end());
                    out.insert(out.end(), cur.begin(), cur.end());
                }
                prev = cur;
            }
        }
    }
}

} // namespace

MergerRenderer::MergerRenderer()
    : m_sky("fullscreen.vert.glsl", "sky.frag.glsl"),
      m_body("body.vert.glsl", "body.frag.glsl"),
      m_grid("grid.vert.glsl", "grid.frag.glsl"),
      m_beam("beam.vert.glsl", "beam.frag.glsl"),
      m_halo("halo.vert.glsl", "halo.frag.glsl"),
      m_particles("particles.vert.glsl", "particles.frag.glsl") {}

void MergerRenderer::draw(const core::Simulation& base, int width, int height) {
    const auto& sim = static_cast<const core::MergerSim&>(base);
    const core::OrbitCamera& cam = sim.camera;
    const glm::mat4 viewProj = cam.projection(static_cast<float>(width) / height) * cam.view();
    const glm::vec3 camPos = cam.position();
    const float fade = sim.fade();

    // Sky
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    m_sky.use();
    setCameraUniforms(m_sky, cam, width, height);
    bindSky(m_sky, height, cam.fovY);
    m_fullscreen.draw();

    // Bodies (opaque)
    glEnable(GL_DEPTH_TEST);
    m_body.use();
    m_body.set("uViewProj", viewProj);
    m_body.set("uCamPos", camPos);
    if (!sim.merged()) {
        const float stretch = sim.tidalStretch();
        for (int i = 0; i < 2; ++i) {
            const float r = sim.bodyRadius(i);
            glm::mat4 model = glm::translate(glm::mat4(1.f), sim.bodyPosition(i));
            model = glm::rotate(model, -sim.phase(), glm::vec3(0.f, 1.f, 0.f));
            model = glm::scale(model, glm::vec3(r * stretch, r, r / std::sqrt(stretch)));
            m_body.set("uModel", model);
            m_body.set("uNormalMat", glm::inverseTranspose(glm::mat3(model)));
            m_body.set("uColor", i == 0 ? glm::vec3(0.45f, 0.65f, 1.f) : glm::vec3(0.6f, 0.75f, 1.f));
            m_body.set("uBrightness", 0.75f * fade);
            m_sphere.draw();
        }
    } else {
        const float glow = sim.remnantGlow();
        const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.f), sim.bodyPosition(0)), glm::vec3(0.95f));
        m_body.set("uModel", model);
        m_body.set("uNormalMat", glm::inverseTranspose(glm::mat3(model)));
        m_body.set("uColor", glm::vec3(0.7f, 0.8f, 1.f));
        m_body.set("uBrightness", fade * (0.1f + 0.7f * glow));
        m_sphere.draw();
    }

    // Everything below is additive light.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);

    // Spacetime grid
    std::vector<float> lines;
    buildGrid(sim, fade, lines);
    m_gridBuffer.upload(lines);
    m_grid.use();
    m_grid.set("uViewProj", viewProj);
    m_grid.set("uBaseY", core::MergerSim::kGridY);
    m_grid.set("uIntensity", 1.f);
    m_gridBuffer.draw(GL_LINES);

    // Jets
    const float jet = sim.jetStrength();
    if (jet > 0.f) {
        m_beam.use();
        m_beam.set("uViewProj", viewProj);
        m_beam.set("uCamPos", camPos);
        m_beam.set("uLength", 12.f);
        m_beam.set("uOrigin", sim.bodyPosition(0));
        m_beam.set("uU", glm::vec3(1.f, 0.f, 0.f));
        m_beam.set("uV", glm::vec3(0.f, 0.f, 1.f));
        for (float sign : {1.f, -1.f}) {
            m_beam.set("uAxis", glm::vec3(0.f, sign, 0.f));
            for (int layer = 0; layer < 2; ++layer) {
                const bool inner = layer == 1;
                m_beam.set("uTanHalf", 0.12f * (inner ? 0.4f : 1.f));
                m_beam.set("uColor", inner ? glm::vec3(0.85f, 0.92f, 1.f) : glm::vec3(0.4f, 0.6f, 1.f));
                m_beam.set("uIntensity", (inner ? 1.0f : 0.45f) * jet * fade);
                m_beam.set("uFlash", 0.f);
                m_cone.draw();
            }
        }
    }

    // Kilonova ejecta
    const auto& ps = sim.ejecta();
    if (!ps.empty()) {
        std::vector<float> buf;
        buf.reserve(ps.size() * 4);
        for (const auto& p : ps) {
            const glm::vec3 pos = p.position + glm::vec3(0.f, sim.bodyPosition(0).y, 0.f);
            buf.insert(buf.end(), {pos.x, pos.y, pos.z, std::pow(1.f - p.age / p.lifetime, 1.6f)});
        }
        m_particleBuffer.upload(buf);
        glEnable(GL_PROGRAM_POINT_SIZE);
        m_particles.use();
        m_particles.set("uViewProj", viewProj);
        m_particles.set("uPointSize", 0.16f);
        m_particles.set("uScreenHeight", static_cast<float>(height));
        m_particles.set("uGlow", 1.2f * fade);
        m_particles.set("uTintNew", glm::vec3(0.75f, 0.85f, 1.f));
        m_particles.set("uTintOld", glm::vec3(1.f, 0.3f, 0.1f));
        m_particleBuffer.draw(GL_POINTS);
    }

    // Halos: around each star, plus the merger flash.
    glDisable(GL_DEPTH_TEST);
    m_halo.use();
    m_halo.set("uViewProj", viewProj);
    m_halo.set("uRight", cam.right());
    m_halo.set("uUp", cam.up());
    if (!sim.merged()) {
        for (int i = 0; i < 2; ++i) {
            m_halo.set("uCenter", sim.bodyPosition(i));
            m_halo.set("uSize", 2.4f);
            m_halo.set("uColor", glm::vec3(0.25f, 0.5f, 1.f) * (0.3f * fade));
            m_quad.draw();
        }
    } else {
        m_halo.set("uCenter", sim.bodyPosition(0));
        m_halo.set("uSize", 3.f);
        m_halo.set("uColor", glm::vec3(0.4f, 0.6f, 1.f) * (0.6f * sim.remnantGlow() * fade));
        m_quad.draw();
        m_halo.set("uSize", 14.f);
        m_halo.set("uColor", glm::vec3(0.9f, 0.92f, 1.f) * (0.9f * sim.flash() * fade));
        m_quad.draw();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

} // namespace render
