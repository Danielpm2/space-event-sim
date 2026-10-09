#pragma once

#include <atomic>
#include <cstdint>

namespace core {

// What the ship's sound should follow, all 0..1 except `volume`.
struct ShipAudioParams {
    float active = 0.f;    // ship is flying; fades the whole mix in and out
    float shake = 0.f;     // hull rattle
    float speed = 0.f;     // engine pitch and air rush
    float proximity = 0.f; // opens the engine's tone as the event looms
    float impulse = 0.f;   // a rising edge through 0.6 fires a shock-wave boom
    float deathFade = 0.f; // silences the mix; a rising edge through 0.5 fires a static burst
    float volume = 0.7f;
};

// Synthesised ambient ship interior: engine drone, pad, ventilation, console
// chirps, plus rumble, boom and static driven by ShipAudioParams.
// setParams() is called from the main thread and render() from the audio thread.
class ShipAudio {
public:
    explicit ShipAudio(float sampleRate = 48000.f);

    void setParams(const ShipAudioParams& p);
    // Fills `frames` interleaved stereo samples in [-1, 1].
    void render(float* stereo, int frames);

private:
    float noise(); // white, [-1, 1]

    struct Input {
        std::atomic<float> active{0.f}, shake{0.f}, speed{0.f}, proximity{0.f}, impulse{0.f}, deathFade{0.f},
            volume{0.7f};
    } m_in;

    float m_fs;
    float m_kFast, m_kSlow;
    uint32_t m_rng = 0x9e3779b9u;

    // Smoothed copies of the inputs.
    float m_active = 0.f, m_shake = 0.f, m_speed = 0.f, m_prox = 0.f, m_death = 0.f, m_vol = 0.7f;
    float m_prevImpulse = 0.f, m_prevDeath = 0.f;

    double m_time = 0.0;
    double m_enginePhase = 0.0, m_enginePhase2 = 0.0;
    double m_padPhase[6] = {};
    float m_engineLp = 0.f;
    float m_hissLow[2] = {}, m_hissBand[2] = {};
    float m_brown = 0.f, m_rumbleLp = 0.f;
    float m_rattleLow = 0.f, m_rattleBand = 0.f;
    float m_boom = 0.f, m_boomTime = 0.f, m_boomNoiseLp = 0.f;
    double m_boomPhase = 0.0;
    float m_static = 0.f;
    float m_chirp = 0.f, m_chirpPan = 0.5f;
    double m_chirpPhase = 0.0, m_chirpFreq = 1500.0;
    int m_chirpWait = 0;
};

} // namespace core
