#pragma once

#include "render/Shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace render {

// Everything the cockpit reacts to, gathered by the caller each frame.
struct CockpitState {
    glm::vec3 eventDir{0.f, 0.f, -1.f}; // view-space unit vector to the event
    glm::vec3 glow{0.f};                // event light on the interior
    glm::vec3 vibration{0.f};           // rattle relative to the pilot's head, radians
    float speed = 0.f;                  // 0..1
    float shake = 0.f;                  // 0..1
    float proximity = 0.f;              // 0..1
    float time = 0.f;
};

// The ship's cockpit frame and instrument panels, drawn in view space over the scene.
class CockpitRenderer {
public:
    CockpitRenderer();
    ~CockpitRenderer();
    CockpitRenderer(const CockpitRenderer&) = delete;
    CockpitRenderer& operator=(const CockpitRenderer&) = delete;

    // Call inside the HDR scene pass, after the event has been drawn.
    void draw(const CockpitState& state, float aspect);

private:
    Shader m_shader;
    GLuint m_vao = 0, m_vbo = 0;
    GLsizei m_count = 0;
};

} // namespace render
