#include "core/MergerSim.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {
constexpr float kInspiralRate = 11.7f; // tunes the inspiral to ~16 s at defaults
constexpr float kOmegaScale = 6.f;     // visual speed-up of the orbit
constexpr float kWaveSpeed = 8.f;      // visual speed of GW ripples
constexpr float kPi = 3.14159265f;
constexpr int kEjectaCount = 2600;

float smoothstep(float a, float b, float x) {
    const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}
} // namespace

MergerSim::MergerSim() {
    m_a = startSeparation;
    m_ejecta.reserve(kEjectaCount);
    camera.distance = 26.f;
    camera.minDistance = 6.f;
    camera.maxDistance = 70.f;
    camera.pitch = 0.42f;
    camera.yaw = 0.5f;
    camera.target = glm::vec3(0.f, -1.5f, 0.f);
}

float MergerSim::bodyRadius(int i) const { return 0.3f + 0.12f * (i == 0 ? mass1 : mass2); }

float MergerSim::orbitalOmega() const {
    return kOmegaScale * std::sqrt(mass1 + mass2) / std::pow(std::max(m_a, 0.5f), 1.5f);
}

glm::vec3 MergerSim::bodyPosition(int i) const {
    if (m_merged)
        return {0.f, kGridY + wellDepth(0.f, 0.f) + 0.6f, 0.f};
    const float M = mass1 + mass2;
    const float share = i == 0 ? mass2 / M : mass1 / M;
    const float sign = i == 0 ? 1.f : -1.f;
    const float x = sign * share * m_a * std::cos(m_phase);
    const float z = sign * share * m_a * std::sin(m_phase);
    return {x, kGridY + wellDepth(x, z) + bodyRadius(i) * 0.9f, z};
}

float MergerSim::tidalStretch() const {
    if (m_merged)
        return 1.f;
    return 1.f + 0.7f * smoothstep(4.f, 1.3f, m_a);
}

float MergerSim::flash() const {
    return m_merged ? 2.2f * std::exp(-m_sinceMerge / 0.6f) : 0.f;
}

float MergerSim::jetStrength() const {
    if (!m_merged)
        return 0.f;
    return smoothstep(0.6f, 1.2f, m_sinceMerge) * (1.f - smoothstep(3.f, 6.f, m_sinceMerge));
}

float MergerSim::remnantGlow() const {
    return m_merged ? std::exp(-m_sinceMerge / 6.f) : 0.f;
}

float MergerSim::gwAmplitude() const {
    if (m_merged)
        return m_ampAtMerge * std::exp(-m_sinceMerge / 1.8f);
    return 0.22f * mass1 * mass2 * 6.f / (m_a + 1.f);
}

float MergerSim::fade() const {
    const float inT = smoothstep(0.f, 0.8f, m_cycleTime);
    const float outT = m_merged ? 1.f - smoothstep(kAftermath - 2.5f, kAftermath, m_sinceMerge) : 1.f;
    return inT * outT;
}

float MergerSim::wellDepth(float x, float z) const {
    constexpr float soft = 0.8f;
    if (m_merged) {
        const float M = mass1 + mass2;
        return -1.5f * M * (0.75f + 0.25f * std::exp(-m_sinceMerge / 4.f)) / std::sqrt(x * x + z * z + soft);
    }
    float h = 0.f;
    for (int i = 0; i < 2; ++i) {
        const float m = i == 0 ? mass1 : mass2;
        const float M = mass1 + mass2;
        const float share = i == 0 ? mass2 / M : mass1 / M;
        const float sign = i == 0 ? 1.f : -1.f;
        const float bx = sign * share * m_a * std::cos(m_phase);
        const float bz = sign * share * m_a * std::sin(m_phase);
        const float dx = x - bx, dz = z - bz;
        h -= 1.5f * m / std::sqrt(dx * dx + dz * dz + soft);
    }
    return h;
}

float MergerSim::spacetimeHeight(float x, float z) const {
    const float r = std::sqrt(x * x + z * z);
    const float omega = m_merged ? m_omegaAtMerge * (0.4f + 0.6f * std::exp(-m_sinceMerge / 1.5f)) : orbitalOmega();
    const float k = 2.f * omega / kWaveSpeed;
    const float ripple = 1.0f * gwAmplitude() * std::sin(2.f * m_phase - k * r) / std::sqrt(1.f + 0.5f * r);
    return kGridY + wellDepth(x, z) + ripple;
}

void MergerSim::merge() {
    m_merged = true;
    m_sinceMerge = 0.f;
    m_omegaAtMerge = orbitalOmega();
    m_ampAtMerge = gwAmplitude() * 1.6f;

    std::uniform_real_distribution<float> u(0.f, 1.f);
    m_ejecta.clear();
    for (int i = 0; i < kEjectaCount; ++i) {
        const bool polar = u(m_rng) < 0.15f;
        const float az = u(m_rng) * 2.f * kPi;
        const float elev = polar ? (0.9f + 0.6f * u(m_rng)) : (u(m_rng) - 0.5f) * 0.7f;
        const float sgn = u(m_rng) < 0.5f ? 1.f : -1.f;
        const glm::vec3 dir = glm::normalize(
            glm::vec3(std::cos(az) * std::cos(elev), sgn * std::sin(elev), std::sin(az) * std::cos(elev)));
        const float speed = polar ? 4.f + 3.f * u(m_rng) : 1.2f + 2.6f * u(m_rng);
        Particle p;
        p.position = dir * 0.6f;
        p.velocity = dir * speed;
        p.age = 0.f;
        p.lifetime = 10.f + 5.f * u(m_rng);
        m_ejecta.push_back(p);
    }
}

void MergerSim::restart() {
    m_merged = false;
    m_a = startSeparation;
    m_sinceMerge = 0.f;
    m_cycleTime = 0.f;
    m_ejecta.clear();
}

void MergerSim::update(double dtd) {
    const float dt = static_cast<float>(dtd) * timeScale;
    m_cycleTime += dt;

    if (!m_merged) {
        const float contact = bodyRadius(0) + bodyRadius(1);
        const int steps = std::max(1, static_cast<int>(std::ceil(dt / 0.004f)));
        const float h = dt / steps;
        const float M = mass1 + mass2;
        for (int i = 0; i < steps && !m_merged; ++i) {
            m_phase = std::fmod(m_phase + orbitalOmega() * h, 2.f * kPi);
            m_a -= kInspiralRate * mass1 * mass2 * M / (m_a * m_a * m_a) * h;
            if (m_a <= contact)
                merge();
        }
        return;
    }

    m_sinceMerge += dt;
    const float spin = m_omegaAtMerge * (0.4f + 0.6f * std::exp(-m_sinceMerge / 1.5f));
    m_phase = std::fmod(m_phase + spin * dt, 2.f * kPi);

    for (size_t i = 0; i < m_ejecta.size();) {
        Particle& p = m_ejecta[i];
        p.age += dt;
        p.position += p.velocity * dt;
        p.velocity *= std::exp(-0.05f * dt);
        if (p.age >= p.lifetime) {
            p = m_ejecta.back();
            m_ejecta.pop_back();
        } else {
            ++i;
        }
    }

    if (m_sinceMerge >= kAftermath)
        restart();
}

std::vector<Param> MergerSim::params() {
    return {
        {"Mass 1 (Msun)", &mass1, 1.0f, 2.5f},
        {"Mass 2 (Msun)", &mass2, 1.0f, 2.5f},
        {"Initial separation (next loop)", &startSeparation, 5.f, 9.f},
        {"Time scale", &timeScale, 0.2f, 3.f},
    };
}

} // namespace core
