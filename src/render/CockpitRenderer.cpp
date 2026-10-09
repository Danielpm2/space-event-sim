#include "render/CockpitRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace render {

namespace {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    float mat; // 0 hull, 1..3 instrument screens, 4 emissive strip
};

constexpr float kFovDeg = 60.f; // fixed, so the frame does not change with the scene's field of view

void addQuad(std::vector<Vertex>& out, const glm::vec3& c, const glm::vec3& u, const glm::vec3& v,
             const glm::vec3& n, float mat) {
    const Vertex corners[4] = {
        {c - u - v, n, {0.f, 0.f}, mat}, {c + u - v, n, {1.f, 0.f}, mat},
        {c + u + v, n, {1.f, 1.f}, mat}, {c - u + v, n, {0.f, 1.f}, mat},
    };
    for (int i : {0, 1, 2, 0, 2, 3})
        out.push_back(corners[i]);
}

void addBox(std::vector<Vertex>& out, const glm::vec3& c, const glm::vec3& half, const glm::mat3& rot, float mat) {
    for (int axis = 0; axis < 3; ++axis)
        for (float sign : {-1.f, 1.f}) {
            glm::vec3 n(0.f), u(0.f), v(0.f);
            n[axis] = sign;
            u[(axis + 1) % 3] = 1.f;
            v[(axis + 2) % 3] = 1.f;
            addQuad(out, c + rot * (n * half[axis]), rot * (u * half[(axis + 1) % 3]),
                    rot * (v * half[(axis + 2) % 3]), rot * n, mat);
        }
}

glm::mat3 rotZ(float a) { return glm::mat3(glm::rotate(glm::mat4(1.f), a, glm::vec3(0.f, 0.f, 1.f))); }
glm::mat3 rotX(float a) { return glm::mat3(glm::rotate(glm::mat4(1.f), a, glm::vec3(1.f, 0.f, 0.f))); }

std::vector<Vertex> buildCockpit() {
    std::vector<Vertex> v;
    const glm::mat3 I(1.f);

    // Dashboard and canopy frame (metres, camera at the origin looking down -Z).
    addBox(v, {0.f, -0.95f, -1.15f}, {2.2f, 0.4f, 0.6f}, I, 0.f);
    addBox(v, {-1.38f, 0.05f, -1.45f}, {0.1f, 1.0f, 0.07f}, rotZ(-0.28f), 0.f);
    addBox(v, {1.38f, 0.05f, -1.45f}, {0.1f, 1.0f, 0.07f}, rotZ(0.28f), 0.f);
    addBox(v, {0.f, 0.64f, -1.4f}, {1.6f, 0.08f, 0.1f}, I, 0.f);
    addBox(v, {0.f, -0.545f, -1.72f}, {1.9f, 0.008f, 0.012f}, I, 4.f);
    addBox(v, {0.f, 0.555f, -1.38f}, {1.55f, 0.006f, 0.012f}, I, 4.f);

    // Three tilted instrument panels standing on the dashboard.
    const glm::mat3 tilt = rotX(-0.7f);
    for (int i = 0; i < 3; ++i) {
        const glm::vec3 c(-0.95f + 0.95f * static_cast<float>(i), -0.5f, -1.5f);
        addBox(v, c, {0.41f, 0.24f, 0.02f}, tilt, 0.f);
        addQuad(v, c + tilt * glm::vec3(0.f, 0.f, 0.021f), tilt * glm::vec3(0.38f, 0.f, 0.f),
                tilt * glm::vec3(0.f, 0.21f, 0.f), tilt * glm::vec3(0.f, 0.f, 1.f), 1.f + static_cast<float>(i));
    }
    return v;
}

} // namespace

CockpitRenderer::CockpitRenderer() : m_shader("cockpit.vert.glsl", "cockpit.frag.glsl") {
    const std::vector<Vertex> verts = buildCockpit();
    m_count = static_cast<GLsizei>(verts.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, mat)));
    glBindVertexArray(0);
}

CockpitRenderer::~CockpitRenderer() {
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void CockpitRenderer::draw(const CockpitState& s, float aspect) {
    // Keep the scene's colour but start a fresh depth range so the frame always wins.
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glm::mat4 model(1.f);
    model = glm::rotate(model, s.vibration.x, glm::vec3(1.f, 0.f, 0.f));
    model = glm::rotate(model, s.vibration.y, glm::vec3(0.f, 1.f, 0.f));
    model = glm::rotate(model, s.vibration.z, glm::vec3(0.f, 0.f, 1.f));

    m_shader.use();
    m_shader.set("uProj", glm::perspective(glm::radians(kFovDeg), aspect, 0.05f, 20.f));
    m_shader.set("uModel", model);
    m_shader.set("uXScale", std::clamp(aspect / (16.f / 9.f), 0.7f, 1.6f));
    m_shader.set("uEventDir", s.eventDir);
    m_shader.set("uGlow", s.glow);
    m_shader.set("uSpeed", s.speed);
    m_shader.set("uShake", s.shake);
    m_shader.set("uProx", s.proximity);
    m_shader.set("uTime", s.time);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_count);
    glBindVertexArray(0);
}

} // namespace render
