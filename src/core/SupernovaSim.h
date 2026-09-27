#pragma once

#include "core/Simulation.h"

namespace core {

// Core-collapse supernova that loops: collapse -> flash -> expanding shock
// shell -> fade, leaving a neutron star behind.
class SupernovaSim : public Simulation {
public:
    static constexpr float kCollapseTime = 3.5f; // seconds (before time scale)
    static constexpr float kCycleTime = 28.f;

    SupernovaSim();

    void update(double dt) override;
    std::vector<Param> params() override;
    std::string name() const override { return "Supernova"; }

    float time() const { return m_t; }
    bool exploded() const { return m_t >= kCollapseTime; }
    float sinceBlast() const { return m_t > kCollapseTime ? m_t - kCollapseTime : 0.f; }

    float progenitorRadius() const; // contracts during collapse
    float starBrightness() const;   // brightens during collapse
    float flash() const;            // bright burst at core bounce
    float shockRadius() const;
    float shellThickness() const;
    float shellBrightness() const;
    float fade() const;             // smooth loop in/out
    float remnantGlow() const { return exploded() ? 1.f : 0.f; }

    float progenitorMass = 20.f; // solar masses
    float energy = 1.f;          // relative explosion energy
    float clumpiness = 0.6f;
    float timeScale = 1.f;

private:
    float m_t = 0.f;
};

} // namespace core
