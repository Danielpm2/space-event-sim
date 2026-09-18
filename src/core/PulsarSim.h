#pragma once

#include "core/Simulation.h"

#include <glm/glm.hpp>

#include <random>
#include <vector>

namespace core {

// Neutron star spinning about +Y with a magnetic axis tilted away from it.
// Two beams leave along +/- the magnetic axis and sweep like a lighthouse.
class PulsarSim : public Simulation {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float age;
        float lifetime;
    };

    static constexpr float kStarRadius = 1.f;
    static constexpr float kBeamLength = 10.f;
    static constexpr size_t kMaxParticles = 6000;

    PulsarSim();

    void update(double dt) override;
    std::vector<Param> params() override;
    std::string name() const override { return "Pulsar"; }

    // Current rotation angle of the star about +Y, radians.
    float phase() const { return m_phase; }
    // Unit vector along the (north) magnetic pole.
    glm::vec3 magneticAxis() const;
    // 0..1 brightness of the beam flash seen by an observer in direction
    // `toObserver` (unit vector from the star).
    float pulseIntensity(const glm::vec3& toObserver) const;
    float beamHalfAngle() const; // radians

    const std::vector<Particle>& particles() const { return m_particles; }

    float spinRate = 0.5f;      // revolutions per second
    float tiltDeg = 35.f;       // angle between magnetic and rotation axes
    float beamWidthDeg = 10.f;  // beam half-angle
    float particleRate = 250.f; // emitted per second
    float glow = 1.f;           // visual brightness multiplier

private:
    void emit(int count);

    float m_phase = 0.f;
    float m_emitAccum = 0.f;
    std::vector<Particle> m_particles;
    std::mt19937 m_rng{1234u};
};

} // namespace core
