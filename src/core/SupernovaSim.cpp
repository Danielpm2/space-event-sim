#include "core/SupernovaSim.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {
float smoothstep(float a, float b, float x) {
    const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}
} // namespace

SupernovaSim::SupernovaSim() {
    camera.distance = 38.f;
    camera.minDistance = 8.f;
    camera.maxDistance = 100.f;
    camera.pitch = 0.3f;
    camera.fovY = 45.f;
}

void SupernovaSim::update(double dt) {
    m_t += static_cast<float>(dt) * timeScale;
    if (m_t >= kCycleTime)
        m_t = std::fmod(m_t, kCycleTime);
}

float SupernovaSim::progenitorRadius() const {
    const float base = 1.2f + 0.08f * progenitorMass;
    if (exploded())
        return 0.f;
    const float c = m_t / kCollapseTime;
    // Slow swelling, then runaway collapse.
    return base * (1.f + 0.06f * std::sin(m_t * 5.f)) * (1.f - 0.92f * c * c * c);
}

float SupernovaSim::starBrightness() const {
    if (exploded())
        return 0.f;
    const float c = m_t / kCollapseTime;
    return 1.f + 7.f * c * c * c * c;
}

float SupernovaSim::flash() const {
    if (!exploded())
        return 0.f;
    return (0.6f + 0.02f * progenitorMass) * std::exp(-sinceBlast() / 0.9f);
}

float SupernovaSim::shockRadius() const {
    const float tau = sinceBlast();
    if (tau <= 0.f)
        return 0.f;
    return shockRadiusAtTau(tau);
}

float SupernovaSim::shockRadiusAtTau(float tau) const {
    return 1.25f * std::sqrt(energy) * std::pow(std::max(tau, 0.f), 0.68f);
}

float SupernovaSim::shellThickness() const {
    return 0.35f + 0.07f * shockRadius() + 0.008f * progenitorMass;
}

float SupernovaSim::shellBrightness() const {
    if (!exploded())
        return 0.f;
    return (std::exp(-sinceBlast() / 8.f) + 0.1f) * (0.6f + 0.02f * progenitorMass);
}

float SupernovaSim::fade() const {
    return smoothstep(0.f, 0.6f, m_t) * (1.f - smoothstep(kCycleTime - 3.f, kCycleTime, m_t));
}

ApproachSpec SupernovaSim::approachSpec() const {
    ApproachSpec s;
    s.startDistance = 95.f;
    s.gridExtent = 30.f;
    s.nearDistance = 3.f;
    s.fovStart = 48.f;
    s.fovEnd = 68.f;
    return s;
}

std::vector<ApproachShape> SupernovaSim::approachShapes() const {
    // The sim restarts at the collapse when the approach begins, so the closest pass
    // happens at a predictable point of the blast.
    const ApproachSpec s = approachSpec();
    const float tau = 0.5f * s.duration * approachClockScale() * timeScale - kCollapseTime;
    if (tau <= 0.f)
        return {};
    return {{false, shockRadiusAtTau(tau), 0.f, "Blast wave"}};
}

ApproachReadout SupernovaSim::approachReadout() const {
    constexpr float kKmPerUnit = 1.496e8f; // one scene unit = 1 AU
    if (exploded())
        return {"Blast wave", shockRadius(), kKmPerUnit, true};
    return {"Progenitor star", std::max(progenitorRadius(), 0.01f), kKmPerUnit, false};
}

std::vector<Param> SupernovaSim::params() {
    return {
        {"Progenitor mass (Msun)", &progenitorMass, 8.f, 40.f},
        {"Explosion energy", &energy, 0.4f, 2.f},
        {"Clumpiness", &clumpiness, 0.f, 1.f},
        {"Time scale", &timeScale, 0.2f, 3.f},
    };
}

} // namespace core
