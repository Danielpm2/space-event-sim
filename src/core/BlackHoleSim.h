#pragma once

#include "core/Simulation.h"

namespace core {

// Rotating black hole with an equatorial accretion disk (G = c = 1, lengths
// in scene units where the mass M sets the scale; disk lies in the XZ plane).
class BlackHoleSim : public Simulation {
public:
    BlackHoleSim();

    void update(double dt) override;
    std::vector<Param> params() override;
    std::string name() const override { return "Black Hole"; }

    float schwarzschildRadius() const { return 2.f * mass; }
    // Outer event horizon of a Kerr hole with spin parameter a/M.
    float horizonRadius() const;
    // Innermost stable circular orbit for a prograde disk.
    float iscoRadius() const;
    float diskOuterRadius() const { return diskOuterM * mass; }
    // Accumulated disk animation time (scene time units).
    float diskTime() const { return m_diskTime; }

    bool supportsApproach() const override { return true; }
    ApproachSpec approachSpec() const override;
    float approachLethalRadius() const override { return 1.3f * horizonRadius(); }
    std::vector<ApproachShape> approachShapes() const override;
    ApproachReadout approachReadout() const override;
    float approachTimeDilation(float distance) const override;

    float mass = 1.f;          // scale of the hole
    float spin = 0.6f;         // a/M, 0..0.99
    float diskBrightness = 1.f;
    float diskOuterM = 14.f;   // outer disk radius in units of M
    float timeSpeed = 1.f;

private:
    float m_diskTime = 0.f;
};

} // namespace core
