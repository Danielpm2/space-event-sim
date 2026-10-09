#pragma once

#include <glm/glm.hpp>

namespace core {

// Everything a scripted camera move needs to place the camera.
// With firstPerson set, the camera sits at `eye` looking along `heading` and
// yaw/pitch/distance are ignored (distance is kept as |eye - target|).
struct CameraPose {
    float yaw = 0.f;
    float pitch = 0.f;
    float distance = 10.f;
    float roll = 0.f;
    float fovY = 45.f;
    glm::vec3 target{0.f};
    bool firstPerson = false;
    glm::vec3 eye{0.f};
    glm::vec3 heading{0.f, 0.f, -1.f};
};

// Camera orbiting a target point. Pure math, no GL.
class OrbitCamera {
public:
    float yaw = 0.6f;       // radians around +Y
    float pitch = 0.25f;    // radians above the XZ plane
    float roll = 0.f;       // radians about the view axis
    float distance = 12.f;
    float minDistance = 2.f;
    float maxDistance = 80.f;
    float fovY = 45.f;      // degrees
    glm::vec3 target{0.f};

    bool firstPerson = false;
    glm::vec3 eye{0.f};
    glm::vec3 heading{0.f, 0.f, -1.f};

    void orbit(float dYaw, float dPitch);
    void zoom(float factor); // factor > 1 moves away
    // Scripted moves bypass the min/max distance limits that zoom() enforces.
    CameraPose pose() const;
    void setPose(const CameraPose& p);

    glm::vec3 position() const;
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
};

} // namespace core
