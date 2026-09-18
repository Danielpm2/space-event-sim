#include "core/PulsarSim.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {
constexpr float kTwoPi = 6.2831853f;
constexpr float kDeg = 0.017453292f;
constexpr float kParticleLife = 2.2f;
} // namespace

PulsarSim::PulsarSim() {
    m_particles.reserve(kMaxParticles);
    camera.distance = 16.f;
    camera.minDistance = 3.f;
    camera.maxDistance = 60.f;
}

float PulsarSim::beamHalfAngle() const { return beamWidthDeg * kDeg; }

glm::vec3 PulsarSim::magneticAxis() const {
    const float t = tiltDeg * kDeg;
    return {std::sin(t) * std::cos(m_phase), std::cos(t), std::sin(t) * std::sin(m_phase)};
}

float PulsarSim::pulseIntensity(const glm::vec3& toObserver) const {
    const glm::vec3 m = magneticAxis();
    const float c = std::clamp(std::max(glm::dot(m, toObserver), glm::dot(-m, toObserver)), -1.f, 1.f);
    const float angle = std::acos(c);
    const float sigma = std::max(beamHalfAngle(), 1e-3f);
    return std::exp(-(angle * angle) / (sigma * sigma));
}

void PulsarSim::emit(int count) {
    std::uniform_real_distribution<float> u01(0.f, 1.f);
    const glm::vec3 m = magneticAxis();
    const float cosMax = std::cos(beamHalfAngle() * 0.6f);

    // Orthonormal basis around the magnetic axis for cone sampling.
    const glm::vec3 helper = std::abs(m.y) < 0.9f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
    const glm::vec3 b1 = glm::normalize(glm::cross(m, helper));
    const glm::vec3 b2 = glm::cross(m, b1);

    for (int i = 0; i < count && m_particles.size() < kMaxParticles; ++i) {
        const float sign = u01(m_rng) < 0.5f ? 1.f : -1.f;
        const float cosT = 1.f - u01(m_rng) * (1.f - cosMax);
        const float sinT = std::sqrt(std::max(0.f, 1.f - cosT * cosT));
        const float az = u01(m_rng) * kTwoPi;
        const glm::vec3 dir = sign * (m * cosT + (b1 * std::cos(az) + b2 * std::sin(az)) * sinT);

        Particle p;
        p.position = dir * (kStarRadius * 1.05f);
        p.velocity = dir * (kBeamLength / kParticleLife) * (0.85f + 0.3f * u01(m_rng));
        p.age = 0.f;
        p.lifetime = kParticleLife * (0.8f + 0.4f * u01(m_rng));
        m_particles.push_back(p);
    }
}

void PulsarSim::update(double dtd) {
    const float dt = static_cast<float>(dtd);
    m_phase = std::fmod(m_phase + kTwoPi * spinRate * dt, kTwoPi);

    for (size_t i = 0; i < m_particles.size();) {
        Particle& p = m_particles[i];
        p.age += dt;
        p.position += p.velocity * dt;
        if (p.age >= p.lifetime) {
            p = m_particles.back();
            m_particles.pop_back();
        } else {
            ++i;
        }
    }

    m_emitAccum += particleRate * dt;
    const int n = static_cast<int>(m_emitAccum);
    m_emitAccum -= static_cast<float>(n);
    emit(n);
}

std::vector<Param> PulsarSim::params() {
    return {
        {"Spin rate (rev/s)", &spinRate, 0.05f, 3.f},
        {"Magnetic tilt (deg)", &tiltDeg, 0.f, 90.f},
        {"Beam width (deg)", &beamWidthDeg, 2.f, 30.f},
        {"Particle rate", &particleRate, 0.f, 800.f},
        {"Glow", &glow, 0.2f, 2.f},
    };
}

} // namespace core
