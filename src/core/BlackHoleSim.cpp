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
