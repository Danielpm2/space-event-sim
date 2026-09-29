#include "core/MagnetarSim.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {
constexpr float kTwoPi = 6.2831853f;
constexpr float kDeg = 0.017453292f;
constexpr int kLinePoints = 48;
constexpr size_t kMaxParticles = 900;

// Rotates a star-frame vector: tilt about Z, then spin about Y.
glm::vec3 toWorld(const glm::vec3& v, float tilt, float phase) {
    const float ct = std::cos(tilt), st = std::sin(tilt);
    const glm::vec3 t(v.x * ct - v.y * st, v.x * st + v.y * ct, v.z);
    const float cp = std::cos(phase), sp = std::sin(phase);
    return {t.x * cp + t.z * sp, t.y, -t.x * sp + t.z * cp};
}
} // namespace

MagnetarSim::MagnetarSim() {
    m_interval = flareInterval;
    m_sinceFlare = 0.5f * m_interval;
    m_twist = 0.5f * maxTwist();
    m_particles.reserve(kMaxParticles);
    camera.distance = 17.f;
    camera.minDistance = 4.f;
    camera.maxDistance = 60.f;
    camera.pitch = 0.3f;
}

glm::vec3 MagnetarSim::magneticAxis() const {
    return toWorld({0.f, 1.f, 0.f}, tiltDeg * kDeg, m_phase);
}

float MagnetarSim::flash() const { return flareEnergy * std::exp(-m_flareAge / 0.45f); }

float MagnetarSim::shellRadius() const { return 1.1f + 11.f * (1.f - std::exp(-m_flareAge / 1.2f)); }

float MagnetarSim::shellStrength() const { return flareEnergy * std::exp(-m_flareAge / 1.3f); }

float MagnetarSim::hotspotStrength() const {
    return flareEnergy * std::exp(-m_flareAge / 2.5f) * (m_flareAge > 0.f ? 1.f : 0.f);
}

glm::vec3 MagnetarSim::hotspotDirection() const {
    return toWorld(m_hotspotLocal, 0.f, m_phase);
}

std::vector<std::vector<glm::vec3>> MagnetarSim::fieldLines() const {
    const int shells = 4 + static_cast<int>(bField * 0.5f);
    const int azimuths = 8;
    const float tilt = tiltDeg * kDeg;
    const float shake = 0.12f * std::exp(-m_flareAge / 0.5f) * flareEnergy;

    std::vector<std::vector<glm::vec3>> lines;
    lines.reserve(static_cast<size_t>(shells) * azimuths);
    for (int s = 0; s < shells; ++s) {
        const float L = 1.9f + 4.6f * static_cast<float>(s) / std::max(shells - 1, 1);
        const float thetaMin = std::asin(std::sqrt(1.f / L));
        for (int a = 0; a < azimuths; ++a) {
            const float phi0 = kTwoPi * (a + 0.5f * (s & 1)) / azimuths;
            std::vector<glm::vec3> pts;
            pts.reserve(kLinePoints);
            for (int i = 0; i < kLinePoints; ++i) {
                const float u = static_cast<float>(i) / (kLinePoints - 1);
                const float theta = thetaMin + (3.14159265f - 2.f * thetaMin) * u;
                const float r = L * std::sin(theta) * std::sin(theta);
                // Twist winds the line around the axis, more on outer shells.
                const float phi = phi0 + m_twist * std::cos(theta) * (0.4f + 0.6f * (L / 6.5f));
                glm::vec3 p(r * std::sin(theta) * std::cos(phi), r * std::cos(theta), r * std::sin(theta) * std::sin(phi));
                p = toWorld(p, tilt, m_phase);
                if (shake > 0.f) {
                    const float k = 3.f * r + m_time * 40.f + static_cast<float>(s * 7 + a);
                    p += glm::vec3(std::sin(k), std::sin(k * 1.3f + 1.f), std::sin(k * 0.7f + 2.f)) * shake * (r / L);
                }
                pts.push_back(p);
            }
            lines.push_back(std::move(pts));
        }
    }
    return lines;
}

void MagnetarSim::triggerFlare() {
    std::uniform_real_distribution<float> u(0.f, 1.f);
    m_flareAge = 0.f;
    m_sinceFlare = 0.f;
    m_interval = flareInterval * (0.75f + 0.5f * u(m_rng));
    m_twist *= 0.15f;

    // Crust fracture site somewhere on the surface.
    const float az = u(m_rng) * kTwoPi;
    const float cy = 2.f * u(m_rng) - 1.f;
    const float sy = std::sqrt(1.f - cy * cy);
    m_hotspotLocal = {sy * std::cos(az), cy, sy * std::sin(az)};

    const glm::vec3 origin = toWorld(m_hotspotLocal, 0.f, m_phase);
    const glm::vec3 helper = std::abs(origin.y) < 0.9f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
    const glm::vec3 b1 = glm::normalize(glm::cross(origin, helper));
    const glm::vec3 b2 = glm::cross(origin, b1);
    const int count = static_cast<int>(500 * flareEnergy);
    for (int i = 0; i < count && m_particles.size() < kMaxParticles; ++i) {
        const float cosT = 1.f - u(m_rng) * 0.6f;
        const float sinT = std::sqrt(1.f - cosT * cosT);
        const float a = u(m_rng) * kTwoPi;
        const glm::vec3 dir = origin * cosT + (b1 * std::cos(a) + b2 * std::sin(a)) * sinT;
        Particle p;
        p.position = dir * (kStarRadius * 1.02f);
        p.velocity = dir * (3.f + 5.f * u(m_rng));
        p.age = 0.f;
        p.lifetime = 2.f + 1.5f * u(m_rng);
        m_particles.push_back(p);
    }
}

void MagnetarSim::update(double dtd) {
    const float dt = static_cast<float>(dtd);
    m_time += dt;
    m_phase = std::fmod(m_phase + kTwoPi * spinRate * dt, kTwoPi);
    m_flareAge += dt;
    m_sinceFlare += dt;

    const float ratio = std::clamp(m_sinceFlare / std::max(m_interval, 0.1f), 0.f, 1.f);
    const float target = maxTwist() * (0.1f + 0.9f * std::pow(ratio, 1.5f));
    m_twist += (target - m_twist) * (1.f - std::exp(-dt * 1.2f));

    if (m_sinceFlare >= m_interval)
        triggerFlare();

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
}

std::vector<Param> MagnetarSim::params() {
    return {
        {"Field strength (1e14 G)", &bField, 1.f, 10.f},
        {"Spin rate (rev/s)", &spinRate, 0.02f, 0.6f},
        {"Magnetic tilt (deg)", &tiltDeg, 0.f, 90.f},
        {"Flare interval (s)", &flareInterval, 2.f, 14.f},
        {"Flare energy", &flareEnergy, 0.3f, 2.f},
    };
}

} // namespace core
