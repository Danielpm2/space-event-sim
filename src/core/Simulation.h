#pragma once

#include "core/OrbitCamera.h"
#include "core/Param.h"

#include <string>
#include <vector>

namespace core {

// Base class for every space-event simulation. Holds state only; it knows
// nothing about OpenGL or the UI.
class Simulation {
public:
    virtual ~Simulation() = default;

    virtual void update(double dt) = 0;
    virtual std::vector<Param> params() = 0;
    virtual std::string name() const = 0;

    OrbitCamera camera;
};

} // namespace core
