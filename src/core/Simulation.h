#pragma once

#include "core/OrbitCamera.h"
#include "core/Param.h"

#include <string>
#include <vector>

namespace core {

// What the approach HUD reports about the thing the ship is flying toward.
struct ApproachReadout {
    const char* subject = "";  // e.g. "Blast wave"
    float radius = 1.f;        // scene units
    float kmPerUnit = 1.f;     // physical length of one scene unit
    bool enclosing = false;    // true if the ship can fly inside `radius`
};

// Base class for every space-event simulation. Holds state only; it knows
// nothing about OpenGL or the UI.
class Simulation {
public:
    virtual ~Simulation() = default;

    virtual void update(double dt) = 0;
    virtual std::vector<Param> params() = 0;
    virtual std::string name() const = 0;

    // Cinematic approach: opt in by overriding supportsApproach() and approachPose().
    virtual bool supportsApproach() const { return false; }
    // Camera placement for progress u in [0, 1]; u = 0 is far away, u = 1 is closest.
    virtual CameraPose approachPose(float /*u*/) const { return camera.pose(); }
    virtual void beginApproach() {}
    // Multiplier on the simulation clock while approaching.
    virtual float approachClockScale() const { return 1.f; }
    // 0..1 burst of event energy that should rattle the ship.
    virtual float approachImpulse() const { return 0.f; }
    virtual ApproachReadout approachReadout() const { return {}; }
    // Clock rate at `distance` relative to a distant observer (1 = unaffected).
    virtual float approachTimeDilation(float /*distance*/) const { return 1.f; }

    OrbitCamera camera;
};

} // namespace core
