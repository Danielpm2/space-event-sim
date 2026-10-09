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

// Shape drawn on the aim grid, seen along the flight direction (event at the centre).
struct ApproachShape {
    bool bar = false;      // false: disc of `radius`; true: horizontal bar, half-length `radius`
    float radius = 1.f;    // scene units
    float thickness = 0.f; // bar half-height, scene units
    const char* label = "";
};

// Flight parameters for the first-person flyby. The event sits at the origin;
// the ship flies along -Z at a lateral offset chosen on the aim grid.
struct ApproachSpec {
    float duration = 75.f;       // seconds for the whole pass
    float startDistance = 95.f;  // scene units from the event at the start (and end)
    float gridExtent = 30.f;     // scene units from the grid centre to its edge
    float nearDistance = 3.f;    // distance at which shake and vignette peak
    float fovStart = 50.f;       // degrees, far away
    float fovEnd = 60.f;         // degrees, near the event
};

// Base class for every space-event simulation. Holds state only; it knows
// nothing about OpenGL or the UI.
class Simulation {
public:
    virtual ~Simulation() = default;

    virtual void update(double dt) = 0;
    virtual std::vector<Param> params() = 0;
    virtual std::string name() const = 0;

    // First-person flyby: opt in by overriding supportsApproach() and approachSpec().
    virtual bool supportsApproach() const { return false; }
    virtual ApproachSpec approachSpec() const { return {}; }
    // Ship is destroyed inside this distance from the event; 0 means never.
    virtual float approachLethalRadius() const { return 0.f; }
    // Shapes for the aim grid, as they will be at the moment of closest pass.
    virtual std::vector<ApproachShape> approachShapes() const { return {}; }
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
