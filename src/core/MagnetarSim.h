#pragma once

#include "core/Simulation.h"

#include <glm/glm.hpp>

#include <random>
#include <vector>

namespace core {

// Slowly rotating neutron star with an ultra-strong, twisted dipole field.
// Stress builds in the twist until a starquake releases it as a giant flare.
class MagnetarSim : public Simulation {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float age;
        float lifetime;
    };

    static constexpr float kStarRadius = 1.f;

    MagnetarSim();

    void update(double dt) override;
    std::vector<Param> params() override;
    std::string name() const override { return "Magnetar"; }

    float phase() const { return m_phase; }
    float time() const { return m_time; }
    glm::vec3 magneticAxis() const;
    // World-space polylines of the field lines; each entry is one line.
    std::vector<std::vector<glm::vec3>> fieldLines() const;

    float twist() const { return m_twist; }
    float flash() const;          // burst of light at the quake
    float shellRadius() const;    // expanding flare front
    float shellStrength() const;
    float hotspotStrength() const; // glowing crust fracture
    glm::vec3 hotspotDirection() const; // world-space unit vector

    const std::vector<Particle>& particles() const { return m_particles; }

    float bField = 6.f;          // in 1e14 gauss
    float spinRate = 0.12f;      // revolutions per second
    float tiltDeg = 40.f;
    float flareInterval = 7.f;   // seconds between giant flares
    float flareEnergy = 1.f;

private:
    float maxTwist() const { return 0.5f + 0.18f * bField; }
    void triggerFlare();

    float m_phase = 0.f;
    float m_time = 0.f;
    float m_twist = 0.f;
    float m_sinceFlare = 0.f;
    float m_interval = 7.f;
    float m_flareAge = 1000.f;
    glm::vec3 m_hotspotLocal{0.f, 0.f, 1.f};
    std::vector<Particle> m_particles;
    std::mt19937 m_rng{777u};
};

} // namespace core
