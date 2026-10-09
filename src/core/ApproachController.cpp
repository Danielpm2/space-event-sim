#include "core/ApproachController.h"

#include "core/Easing.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kMaxLookYaw = kPi;
constexpr float kMaxLookPitch = 1.0f;
constexpr float kMaxPitch = 1.5f;
constexpr float kShakeAngle = 0.008f; // radians at full shake
constexpr float kCrawl = 0.12f;       // share of the opening speed kept at the closest pass
constexpr float kGazeLag = 0.35f;     // seconds the pilot takes to follow the event
constexpr float kGazeMaxRate = 1.5f;  // rad/s

// Two incommensurate sines read as irregular vibration without any state.
float wobble(float t, float f1, float f2, float phase) {
    return 0.6f * std::sin(t * f1 + phase) + 0.4f * std::sin(t * f2 + phase * 2.3f);
}

} // namespace

glm::vec3 ApproachController::flightPosition(float startDistance, glm::vec2 offset, float u) {
    // Fast when far, slow at the closest pass, then away again.
    const float w = 1.f - 2.f * std::clamp(u, 0.f, 1.f);
    const float z = startDistance * w * (kCrawl + (1.f - kCrawl) * w * w);
    return {offset.x, offset.y, z};
}

void ApproachController::setAim(glm::vec2 aim) { m_aim = glm::clamp(aim, glm::vec2(-1.f), glm::vec2(1.f)); }

void ApproachController::start(Simulation& sim) {
    m_saved = sim.camera;
    m_active = true;
    m_dead = false;
    m_u = 0.f;
    m_clock = 0.f;
    m_lookYaw = m_lookPitch = 0.f;
    m_bank = 0.f;
    m_deathFade = 0.f;
    m_gazeSnap = true;
    m_havePrev = false;
    duration = sim.approachSpec().duration;
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
    m_dead = false;
    m_speed = m_proximity = m_shake = m_deathFade = 0.f;
    m_velocity = glm::vec3(0.f);
    m_havePrev = false;
}

void ApproachController::setProgress(float u) {
    m_u = std::clamp(u, 0.f, 1.f);
    m_gazeSnap = true;
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
    const ApproachSpec spec = sim.approachSpec();
    m_clock += dt;
    if (!m_dead)
        m_u = std::min(1.f, m_u + dt / std::max(duration, 1.f));
    const float ease = std::exp(-0.5f * dt);
    m_lookYaw *= ease;
    m_lookPitch *= ease;

    const glm::vec2 offset = m_aim * spec.gridExtent;
    const glm::vec3 pos = flightPosition(spec.startDistance, offset, m_u);
    const float dist = glm::length(pos);

    const float lethal = sim.approachLethalRadius();
    if (lethal > 0.f) {
        if (dist <= lethal)
            m_dead = true;
        const float approach = std::clamp((1.6f * lethal - dist) / (0.6f * lethal), 0.f, 1.f);
        m_deathFade = m_dead ? std::min(1.f, std::max(m_deathFade, approach) + dt) : approach;
    }

    const float dNear = std::max(spec.nearDistance, 1e-3f);
    const float span = std::log(std::max(spec.startDistance, dNear * 1.01f) / dNear);
    m_proximity = std::clamp(std::log(spec.startDistance / std::max(dist, 1e-3f)) / span, 0.f, 1.f);
    m_shake = std::max(0.55f * m_proximity * m_proximity, 0.8f * std::clamp(sim.approachImpulse(), 0.f, 1.f));

    // The pilot tracks the event with a lag and a turn-rate limit, so a pass right over
    // it sweeps the view instead of snapping it.
    const float toLen = std::max(dist, 1e-3f);
    const float targetYaw = std::atan2(-pos.x, pos.z);
    const float targetPitch = std::clamp(std::asin(std::clamp(-pos.y / toLen, -1.f, 1.f)), -kMaxPitch, kMaxPitch);
    float yawRate = 0.f;
    if (m_gazeSnap) {
        m_gazeYaw = targetYaw;
        m_gazePitch = targetPitch;
        m_gazeSnap = false;
    } else if (dt > 0.f) {
        const float a = 1.f - std::exp(-dt / kGazeLag);
        const float maxStep = kGazeMaxRate * dt;
        const float dYaw = std::remainder(targetYaw - m_gazeYaw, 2.f * kPi);
        const float stepYaw = std::clamp(dYaw * a, -maxStep, maxStep);
        const float stepPitch = std::clamp((targetPitch - m_gazePitch) * a, -maxStep, maxStep);
        m_gazeYaw += stepYaw;
        m_gazePitch += stepPitch;
        yawRate = stepYaw / dt;
    }
    // Bank into the turn: a right turn drops the right side, i.e. negative roll.
    if (dt > 0.f)
        m_bank += (std::clamp(-0.6f * yawRate, -0.2f, 0.2f) - m_bank) * (1.f - std::exp(-3.f * dt));

    // Velocity comes from the path alone, so looking around or shaking never reads as motion.
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
    const float rattle = 0.0008f + 0.006f * m_shake;
    m_vibration = {rattle * wobble(m_clock, 37.f, 61.f, 0.9f), rattle * wobble(m_clock, 43.f, 71.f, 2.1f),
                   0.6f * rattle * wobble(m_clock, 29.f, 53.f, 3.7f)};
    const float yaw = m_gazeYaw + m_lookYaw + amp * wobble(m_clock, 13.1f, 29.7f, 0.3f);
    const float pitch = std::clamp(m_gazePitch + m_lookPitch + amp * wobble(m_clock, 11.3f, 23.9f, 1.7f),
                                   -kMaxPitch, kMaxPitch);
    CameraPose out;
    out.firstPerson = true;
    out.eye = pos;
    out.heading = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
    out.roll = m_bank + 1.5f * amp * wobble(m_clock, 7.9f, 17.3f, 2.9f);
    out.fovY = lerp(spec.fovStart, spec.fovEnd, smooth01(m_proximity));
    sim.camera.setPose(out);
}

} // namespace core
