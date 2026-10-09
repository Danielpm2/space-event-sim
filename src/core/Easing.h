#pragma once

#include <algorithm>
#include <cmath>

namespace core {

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Constant-ratio interpolation: equal time steps shrink the distance by equal factors.
inline float logLerp(float a, float b, float t) { return a * std::pow(b / a, t); }

inline float smooth01(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

} // namespace core
