#include "core/BlackHoleSim.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {
constexpr float kTimeUnitsPerSecond = 12.f; // orbital motion is slow in raw G=c=1 units
}

BlackHoleSim::BlackHoleSim() {
    camera.distance = 34.f;
    camera.minDistance = 10.f;
    camera.maxDistance = 120.f;
    camera.pitch = 0.18f;
    camera.yaw = 0.f;
    camera.fovY = 50.f;
}

void BlackHoleSim::update(double dt) {
    m_diskTime += static_cast<float>(dt) * timeSpeed * kTimeUnitsPerSecond;
}

float BlackHoleSim::horizonRadius() const {
    const float a = std::clamp(spin, 0.f, 0.998f);
    return mass * (1.f + std::sqrt(1.f - a * a));
}

float BlackHoleSim::iscoRadius() const {
    const float a = std::clamp(spin, 0.f, 0.998f);
    const float z1 = 1.f + std::cbrt(1.f - a * a) * (std::cbrt(1.f + a) + std::cbrt(1.f - a));
    const float z2 = std::sqrt(3.f * a * a + z1 * z1);
    return mass * (3.f + z2 - std::sqrt((3.f - z1) * (3.f + z1 + 2.f * z2)));
}

ApproachSpec BlackHoleSim::approachSpec() const {
    ApproachSpec s;
    s.startDistance = 110.f;
    s.gridExtent = std::max(40.f, 1.3f * diskOuterRadius());
    s.nearDistance = 5.5f * mass;
    s.fovStart = 50.f;
    s.fovEnd = 62.f;
    return s;
}

std::vector<ApproachShape> BlackHoleSim::approachShapes() const {
    // The disk lies in the XZ plane, so it is edge-on to a ship flying along -Z.
    return {
        {true, diskOuterRadius(), 0.25f * mass, "Accretion disk"},
        {false, 1.5f * schwarzschildRadius(), 0.f, "Photon sphere"},
        {false, horizonRadius(), 0.f, "Event horizon"},
    };
}

ApproachReadout BlackHoleSim::approachReadout() const {
    constexpr float kMassKm = 5.9e6f; // GM/c^2 of a 4-million-solar-mass hole
    return {"Accretion disk", diskOuterRadius(), kMassKm / mass, false};
}

float BlackHoleSim::approachTimeDilation(float distance) const {
    return std::sqrt(std::max(1.f - schwarzschildRadius() / std::max(distance, 1e-3f), 0.f));
}

std::vector<Param> BlackHoleSim::params() {
    return {
        {"Mass", &mass, 0.5f, 2.f},
        {"Spin (a/M)", &spin, 0.f, 0.99f},
        {"Disk brightness", &diskBrightness, 0.2f, 3.f},
        {"Disk radius (M)", &diskOuterM, 8.f, 30.f},
        {"Time speed", &timeSpeed, 0.f, 4.f},
    };
}

} // namespace core
