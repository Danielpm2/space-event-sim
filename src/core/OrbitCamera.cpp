#include "core/OrbitCamera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace core {

void OrbitCamera::orbit(float dYaw, float dPitch) {
    constexpr float limit = 1.553343f; // ~89 degrees
    yaw = std::fmod(yaw + dYaw, 6.2831853f);
    pitch = std::clamp(pitch + dPitch, -limit, limit);
}

void OrbitCamera::zoom(float factor) {
    distance = std::clamp(distance * factor, minDistance, maxDistance);
}

glm::vec3 OrbitCamera::position() const {
    return target + distance * glm::vec3(std::cos(pitch) * std::sin(yaw),
                                         std::sin(pitch),
                                         std::cos(pitch) * std::cos(yaw));
}

glm::mat4 OrbitCamera::view() const {
    return glm::lookAt(position(), target, glm::vec3(0.f, 1.f, 0.f));
}

glm::mat4 OrbitCamera::projection(float aspect) const {
    return glm::perspective(glm::radians(fovY), aspect, 0.05f, 500.f);
}

} // namespace core
