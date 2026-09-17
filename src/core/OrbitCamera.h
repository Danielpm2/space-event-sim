#pragma once

#include <glm/glm.hpp>

namespace core {

// Camera orbiting a target point. Pure math, no GL.
class OrbitCamera {
public:
    float yaw = 0.6f;       // radians around +Y
    float pitch = 0.25f;    // radians above the XZ plane
    float distance = 12.f;
    float minDistance = 2.f;
    float maxDistance = 80.f;
    float fovY = 45.f;      // degrees
    glm::vec3 target{0.f};

    void orbit(float dYaw, float dPitch);
    void zoom(float factor); // factor > 1 moves away

    glm::vec3 position() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
};

} // namespace core
