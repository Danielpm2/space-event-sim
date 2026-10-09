#include "core/ApproachController.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

constexpr float kMaxLookYaw = 1.2f;
constexpr float kMaxLookPitch = 0.7f;
constexpr float kShakeAngle = 0.008f; // radians at full shake

// Two incommensurate sines read as irregular vibration without any state.
float wobble(float t, float f1, float f2, float phase) {
    return 0.6f * std::sin(t * f1 + phase) + 0.4f * std::sin(t * f2 + phase * 2.3f);
}

} // namespace

void ApproachController::start(Simulation& sim) {
    m_saved = sim.camera;
    m_active = true;
    m_u = 0.f;
    m_clock = 0.f;
    m_lookYaw = m_lookPitch = 0.f;
    m_havePrev = false;
    sim.beginApproach();
    update(sim, 0.0);
}

void ApproachController::stop(Simulation& sim) {
    if (!m_active)
        return;
    sim.camera = m_saved;
    reset();
}

void ApproachController::reset() {
    m_active = false;
    m_speed = m_proximity = m_shake = 0.f;
    m_velocity = glm::vec3(0.f);
    m_havePrev = false;
}

void ApproachController::setProgress(float u) {
    m_u = std::clamp(u, 0.f, 1.f);
    m_havePrev = false;
}

void ApproachController::look(float dYaw, float dPitch) {
    m_lookYaw = std::clamp(m_lookYaw + dYaw, -kMaxLookYaw, kMaxLookYaw);
    m_lookPitch = std::clamp(m_lookPitch + dPitch, -kMaxLookPitch, kMaxLookPitch);
}

void ApproachController::update(Simulation& sim, double dtd) {
    if (!m_active)
        return;
    const float dt = static_cast<float>(dtd);
    m_clock += dt;
    m_u = std::min(1.f, m_u + dt / std::max(duration, 1.f));
    const float ease = std::exp(-0.5f * dt);
    m_lookYaw *= ease;
    m_lookPitch *= ease;

    const CameraPose pose = sim.approachPose(m_u);
    const float dFar = sim.approachPose(0.f).distance;
    const float dNear = sim.approachPose(1.f).distance;
    const float span = std::log(dFar) - std::log(dNear);
    m_proximity = span > 1e-4f ? std::clamp((std::log(dFar) - std::log(pose.distance)) / span, 0.f, 1.f) : 1.f;
    m_shake = std::max(0.55f * m_proximity * m_proximity, 0.8f * std::clamp(sim.approachImpulse(), 0.f, 1.f));

    // Velocity comes from the path alone, so looking around or shaking never reads as motion.
    OrbitCamera path;
    path.setPose(pose);
    const glm::vec3 pos = path.position();
    if (m_havePrev && dt > 1e-4f) {
        const float a = 1.f - std::exp(-6.f * dt);
        m_velocity += ((pos - m_prevPos) / dt - m_velocity) * a;
    } else {
        m_velocity = glm::vec3(0.f);
    }
    m_speed = glm::length(m_velocity);
    m_prevPos = pos;
    m_havePrev = true;

    const float amp = kShakeAngle * m_shake;
    CameraPose out = pose;
    out.yaw += m_lookYaw + amp * wobble(m_clock, 13.1f, 29.7f, 0.3f);
    out.pitch = std::clamp(pose.pitch + m_lookPitch + amp * wobble(m_clock, 11.3f, 23.9f, 1.7f), -1.5f, 1.5f);
    out.roll += 1.5f * amp * wobble(m_clock, 7.9f, 17.3f, 2.9f);
    sim.camera.setPose(out);
}

} // namespace core
