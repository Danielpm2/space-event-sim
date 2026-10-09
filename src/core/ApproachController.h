#pragma once

#include "core/OrbitCamera.h"
#include "core/Simulation.h"

#include <glm/glm.hpp>

namespace core {

// Flies the ship past a Simulation's event in first person and writes its camera.
// The path is a straight line along -Z at a lateral offset set on the aim grid;
// the pilot's gaze tracks the event. Pure math, no GL.
class ApproachController {
public:
    float duration = 75.f; // seconds; taken from the sim's spec on start()

    // Aim point on the grid, each axis -1..1 (x: left..right, y: below..above).
    // Only takes effect at the next start().
    void setAim(glm::vec2 aim);
    glm::vec2 aim() const { return m_aim; }

    // Ship position along the flyby at progress u, for a lateral offset in scene units.
    static glm::vec3 flightPosition(float startDistance, glm::vec2 offset, float u);

    void start(Simulation& sim);
    // Restores the camera the user had before start().
    void stop(Simulation& sim);
    // Drops state without touching a simulation (it may already be gone).
    void reset();
    void setProgress(float u);
    // Advances the flight and writes the camera. dt = 0 holds still (paused).
    void update(Simulation& sim, double dt);
    // Glance away from the event; eases back to tracking it.
    void look(float dYaw, float dPitch);

    bool active() const { return m_active; }
    bool dead() const { return m_active && m_dead; }
    bool finished() const { return m_active && (m_u >= 1.f || m_dead); }
    float progress() const { return m_u; }
    float elapsed() const { return m_clock; }
    // 0 far away .. 1 at the closest the ship gets.
    float proximity() const { return m_proximity; }
    // 0..1 rattle intensity; also what audio should follow.
    float shake() const { return m_shake; }
    // 0..1 black-out as the ship nears a lethal distance.
    float deathFade() const { return m_deathFade; }
    // Smoothed ship velocity in scene units per second (zero while paused).
    const glm::vec3& velocity() const { return m_velocity; }
    float speed() const { return m_speed; }

private:
    glm::vec2 m_aim{-0.45f, 0.15f};
    bool m_active = false;
    bool m_dead = false;
    float m_u = 0.f;
    float m_clock = 0.f;
    float m_lookYaw = 0.f;
    float m_lookPitch = 0.f;
    float m_gazeYaw = 0.f;
    float m_gazePitch = 0.f;
    bool m_gazeSnap = true;
    float m_bank = 0.f;
    float m_proximity = 0.f;
    float m_shake = 0.f;
    float m_deathFade = 0.f;
    float m_speed = 0.f;
    glm::vec3 m_velocity{0.f};
    glm::vec3 m_prevPos{0.f};
    bool m_havePrev = false;
    OrbitCamera m_saved;
};

} // namespace core
