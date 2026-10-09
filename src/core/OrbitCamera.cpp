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

glm::vec3 OrbitCamera::forward() const { return glm::normalize(target - position()); }

CameraPose OrbitCamera::pose() const { return {yaw, pitch, distance, roll, fovY, target}; }

void OrbitCamera::setPose(const CameraPose& p) {
    yaw = p.yaw;
    pitch = p.pitch;
    distance = p.distance;
    roll = p.roll;
    fovY = p.fovY;
    target = p.target;
}

glm::vec3 OrbitCamera::right() const {
    const glm::vec3 f = forward();
    const glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0.f, 1.f, 0.f)));
    const glm::vec3 u = glm::cross(r, f);
    return r * std::cos(roll) + u * std::sin(roll);
}

glm::vec3 OrbitCamera::up() const {
    const glm::vec3 f = forward();
    const glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0.f, 1.f, 0.f)));
    const glm::vec3 u = glm::cross(r, f);
    return u * std::cos(roll) - r * std::sin(roll);
}

glm::mat4 OrbitCamera::view() const {
    return glm::lookAt(position(), target, up());
}

glm::mat4 OrbitCamera::projection(float aspect) const {
    return glm::perspective(glm::radians(fovY), aspect, 0.05f, 500.f);
}

} // namespace core
