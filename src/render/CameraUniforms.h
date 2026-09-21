#pragma once

#include "core/OrbitCamera.h"
#include "render/Shader.h"

#include <cmath>

namespace render {

// Uniforms declared in shaders/common/camera.glsl.
inline void setCameraUniforms(const Shader& s, const core::OrbitCamera& cam, int width, int height) {
    s.set("uCamPos", cam.position());
    s.set("uCamRight", cam.right());
    s.set("uCamUp", cam.up());
    s.set("uCamForward", cam.forward());
    s.set("uTanHalfFov", std::tan(glm::radians(cam.fovY) * 0.5f));
    s.set("uResolution", glm::vec2(static_cast<float>(width), static_cast<float>(height)));
}

} // namespace render
