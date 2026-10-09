#pragma once

#include "core/OrbitCamera.h"
#include "core/Simulation.h"

#include <glm/glm.hpp>

namespace core {

// Drives a Simulation's camera along its approachPose() path. Pure math, no GL.
class ApproachController {
public:
    float duration = 60.f; // seconds from first to last pose

    void start(Simulation& sim);
    // Restores the camera the user had before start().
    void stop(Simulation& sim);
    // Drops state without touching a simulation (it may already be gone).
    void reset();
    void setProgress(float u);
    // Advances the move and writes the camera. dt = 0 holds still (paused).
    void update(Simulation& sim, double dt);
    // User look-around, layered on top of the path and easing back to centre.
    void look(float dYaw, float dPitch);

    bool active() const { return m_active; }
    bool finished() const { return m_active && m_u >= 1.f; }
    float progress() const { return m_u; }
    float elapsed() const { return m_clock; }
    // 0 far away .. 1 at the closest pose.
    float proximity() const { return m_proximity; }
    // 0..1 rattle intensity; also what audio should follow.
    float shake() const { return m_shake; }
    // Smoothed ship velocity in scene units per second (zero while paused).
    const glm::vec3& velocity() const { return m_velocity; }
    float speed() const { return m_speed; }

private:
    bool m_active = false;
    float m_u = 0.f;
    float m_clock = 0.f;
    float m_lookYaw = 0.f;
    float m_lookPitch = 0.f;
    float m_proximity = 0.f;
    float m_shake = 0.f;
    float m_speed = 0.f;
    glm::vec3 m_velocity{0.f};
    glm::vec3 m_prevPos{0.f};
    bool m_havePrev = false;
    OrbitCamera m_saved;
};

} // namespace core
