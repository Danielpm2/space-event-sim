#pragma once

#include "core/OrbitCamera.h"
#include "render/Primitives.h"
#include "render/Shader.h"

#include <glm/glm.hpp>

#include <vector>

namespace render {

// Streaks of dust that wrap around the camera. Parallax against them is what
// makes flying toward a distant event read as speed.
class ApproachFx {
public:
    ApproachFx();
    // `intensity` 0 draws nothing. Call inside the scene pass, after the event.
    void draw(const core::OrbitCamera& cam, const glm::vec3& velocity, float aspect, float intensity);

private:
    Shader m_shader;
    StreamBuffer4 m_buffer;
    std::vector<glm::vec3> m_points;
    std::vector<float> m_verts;
};

} // namespace render
