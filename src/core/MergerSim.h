#pragma once

#include "core/Simulation.h"

#include <glm/glm.hpp>

#include <random>
#include <vector>

namespace core {

// Two neutron stars spiral together (inspiral -> merger -> kilonova ejecta and
// short-GRB jets), with a spacetime grid that curves around the masses and
// carries gravitational-wave ripples. Loops after the aftermath.
class MergerSim : public Simulation {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float age;
        float lifetime;
    };

    static constexpr float kGridY = -2.2f;
    static constexpr float kAftermath = 16.f; // seconds after merger before looping
    static constexpr float kExtent = 15.f;    // half-size of the drawn grid

    MergerSim();

    void update(double dt) override;
    std::vector<Param> params() override;
    std::string name() const override { return "Neutron Star Merger"; }

    bool merged() const { return m_merged; }
    float sinceMerge() const { return m_sinceMerge; }
    float separation() const { return m_a; }
    float phase() const { return m_phase; }

    // Index 0/1. After the merger both collapse into one remnant at the centre.
    glm::vec3 bodyPosition(int i) const;
    float bodyRadius(int i) const;
    float tidalStretch() const; // >= 1, elongation toward the companion
    float flash() const;
    float jetStrength() const;
    float remnantGlow() const;
    float gwAmplitude() const;
    float fade() const;

    // Height of the spacetime grid at (x, z): curvature wells plus GW ripples.
    float spacetimeHeight(float x, float z) const;

    const std::vector<Particle>& ejecta() const { return m_ejecta; }

    float mass1 = 1.4f;
    float mass2 = 1.4f;
    float startSeparation = 8.f;
    float timeScale = 1.f;

private:
    float wellDepth(float x, float z) const;
    float orbitalOmega() const;
    void merge();
    void restart();

    float m_a = 8.f;
    float m_phase = 0.f;
    float m_omegaAtMerge = 0.f;
    float m_ampAtMerge = 0.f;
    float m_cycleTime = 0.f;
    bool m_merged = false;
    float m_sinceMerge = 0.f;
    std::vector<Particle> m_ejecta;
    std::mt19937 m_rng{4242u};
};

} // namespace core
