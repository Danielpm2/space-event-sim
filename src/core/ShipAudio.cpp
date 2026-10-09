#include "core/ShipAudio.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

constexpr double kTwoPi = 6.283185307179586;

// One-pole coefficient for a cutoff far below the sample rate.
float onePole(float cutoffHz, float fs) { return std::min(1.f, static_cast<float>(kTwoPi) * cutoffHz / fs); }

} // namespace

ShipAudio::ShipAudio(float sampleRate) : m_fs(sampleRate) {
    m_kFast = 1.f - std::exp(-1.f / (0.03f * m_fs));
    m_kSlow = 1.f - std::exp(-1.f / (1.0f * m_fs));
    m_chirpWait = static_cast<int>(1.5f * m_fs);
}

void ShipAudio::setParams(const ShipAudioParams& p) {
    m_in.active.store(p.active, std::memory_order_relaxed);
    m_in.shake.store(p.shake, std::memory_order_relaxed);
    m_in.speed.store(p.speed, std::memory_order_relaxed);
    m_in.proximity.store(p.proximity, std::memory_order_relaxed);
    m_in.impulse.store(p.impulse, std::memory_order_relaxed);
    m_in.deathFade.store(p.deathFade, std::memory_order_relaxed);
    m_in.volume.store(p.volume, std::memory_order_relaxed);
}

float ShipAudio::noise() {
    m_rng ^= m_rng << 13;
    m_rng ^= m_rng >> 17;
    m_rng ^= m_rng << 5;
    return static_cast<float>(m_rng) * (2.f / 4294967295.f) - 1.f;
}

void ShipAudio::render(float* out, int frames) {
    const float tActive = std::clamp(m_in.active.load(std::memory_order_relaxed), 0.f, 1.f);
    const float tShake = std::clamp(m_in.shake.load(std::memory_order_relaxed), 0.f, 1.f);
    const float tSpeed = std::clamp(m_in.speed.load(std::memory_order_relaxed), 0.f, 1.f);
    const float tProx = std::clamp(m_in.proximity.load(std::memory_order_relaxed), 0.f, 1.f);
    const float tDeath = std::clamp(m_in.deathFade.load(std::memory_order_relaxed), 0.f, 1.f);
    const float tVol = std::clamp(m_in.volume.load(std::memory_order_relaxed), 0.f, 1.f);
    const float impulse = m_in.impulse.load(std::memory_order_relaxed);

    if (impulse >= 0.6f && m_prevImpulse < 0.6f) {
        m_boom = 1.f;
        m_boomTime = 0.f;
    }
    m_prevImpulse = impulse;
    if (tDeath >= 0.5f && m_prevDeath < 0.5f)
        m_static = 1.f;
    m_prevDeath = tDeath;

    const float boomDecay = std::exp(-1.f / (1.2f * m_fs));
    const float staticDecay = std::exp(-1.f / (0.35f * m_fs));
    const float chirpDecay = std::exp(-1.f / (0.035f * m_fs));
    const float invFs = 1.f / m_fs;

    for (int i = 0; i < frames; ++i) {
        m_active += (tActive - m_active) * m_kSlow;
        m_shake += (tShake - m_shake) * m_kFast;
        m_speed += (tSpeed - m_speed) * m_kFast;
        m_prox += (tProx - m_prox) * m_kFast;
        m_death += (tDeath - m_death) * m_kFast;
        m_vol += (tVol - m_vol) * m_kFast;

        const double t = m_time;
        m_time += invFs;

        // Engine: a detuned pair of harmonic stacks whose tone opens with speed and closeness.
        const double f = 46.0 + 30.0 * m_speed + 8.0 * m_prox;
        m_enginePhase += f * invFs;
        m_enginePhase2 += f * 1.004 * invFs;
        m_enginePhase -= std::floor(m_enginePhase);
        m_enginePhase2 -= std::floor(m_enginePhase2);
        const float stack = static_cast<float>(
            std::sin(kTwoPi * m_enginePhase) + 0.5 * std::sin(2.0 * kTwoPi * m_enginePhase) +
            0.25 * std::sin(3.0 * kTwoPi * m_enginePhase2) + 0.5 * std::sin(kTwoPi * m_enginePhase2));
        m_engineLp += onePole(180.f + 700.f * m_prox + 400.f * m_speed, m_fs) * (stack - m_engineLp);
        const float engine =
            m_engineLp * 0.2f * (0.6f + 0.4f * m_speed) * (1.f + 0.1f * static_cast<float>(std::sin(kTwoPi * 0.17 * t)));

        // Pad: slow stacked fifths, slightly different per ear.
        static constexpr double padFreq[6] = {110.2, 164.9, 220.6, 109.8, 164.5, 219.4};
        float pad[2] = {0.f, 0.f};
        for (int k = 0; k < 6; ++k) {
            m_padPhase[k] += padFreq[k] * invFs;
            m_padPhase[k] -= std::floor(m_padPhase[k]);
            pad[k / 3] += static_cast<float>(std::sin(kTwoPi * m_padPhase[k]));
        }
        const float tremolo = 0.7f + 0.3f * static_cast<float>(std::sin(kTwoPi * 0.09 * t));
        pad[0] *= 0.012f * tremolo;
        pad[1] *= 0.012f * tremolo;

        // Ventilation and air rush: band-limited noise, independent per ear.
        float hiss[2];
        const float hissAmp = 0.018f + 0.1f * m_speed;
        for (int c = 0; c < 2; ++c) {
            const float n = noise();
            m_hissLow[c] += onePole(600.f, m_fs) * (n - m_hissLow[c]);
            m_hissBand[c] += onePole(2500.f, m_fs) * ((n - m_hissLow[c]) - m_hissBand[c]);
            hiss[c] = m_hissBand[c] * hissAmp;
        }

        // Hull rumble and rattle follow the shake.
        m_brown = m_brown * 0.995f + noise() * 0.06f;
        m_rumbleLp += onePole(120.f, m_fs) * (m_brown - m_rumbleLp);
        const float shake13 = std::pow(m_shake, 1.3f);
        const float rn = noise();
        m_rattleLow += onePole(120.f, m_fs) * (rn - m_rattleLow);
        m_rattleBand += onePole(400.f, m_fs) * ((rn - m_rattleLow) - m_rattleBand);
        const float am = 0.5f + 0.5f * static_cast<float>(std::sin(kTwoPi * 11.0 * t));
        const float rumble = m_rumbleLp * 1.2f * shake13 + m_rattleBand * am * 0.8f * m_shake * m_shake;

        // Occasional soft console chirps.
        if (--m_chirpWait <= 0) {
            m_chirp = 1.f;
            m_chirpFreq = 1200.0 + 1200.0 * (0.5 + 0.5 * noise());
            m_chirpPan = 0.5f + 0.5f * noise();
            m_chirpWait = static_cast<int>((2.5f + 4.f * (0.5f + 0.5f * noise())) * m_fs);
        }
        float chirp = 0.f;
        if (m_chirp > 1e-4f) {
            m_chirpPhase += m_chirpFreq * invFs;
            m_chirpPhase -= std::floor(m_chirpPhase);
            chirp = static_cast<float>(std::sin(kTwoPi * m_chirpPhase)) * m_chirp * 0.012f;
            m_chirp *= chirpDecay;
        }

        // Shock-wave boom: a sine gliding down plus a muffled thump.
        float boom = 0.f;
        if (m_boom > 1e-4f) {
            m_boomTime += invFs;
            m_boomPhase += (30.0 + 40.0 * std::exp(-m_boomTime / 0.6)) * invFs;
            m_boomPhase -= std::floor(m_boomPhase);
            m_boomNoiseLp += onePole(160.f, m_fs) * (noise() - m_boomNoiseLp);
            boom = (static_cast<float>(std::sin(kTwoPi * m_boomPhase)) * 0.55f + m_boomNoiseLp * 0.9f) * m_boom;
            m_boom *= boomDecay;
        }

        // Losing the ship: the mix collapses, with a burst of static on the way down.
        const float deathGain = (1.f - m_death) * (1.f - m_death);
        float staticNoise = 0.f;
        if (m_static > 1e-4f) {
            staticNoise = noise() * m_static * 0.3f;
            m_static *= staticDecay;
        }

        const float common = engine + rumble + boom;
        const float master = m_active * m_vol;
        const float left = (common + pad[0] + hiss[0] + chirp * (1.f - m_chirpPan)) * deathGain + staticNoise;
        const float right = (common + pad[1] + hiss[1] + chirp * m_chirpPan) * deathGain + staticNoise;
        out[2 * i] = std::tanh(left * master);
        out[2 * i + 1] = std::tanh(right * master);
    }
}

} // namespace core
