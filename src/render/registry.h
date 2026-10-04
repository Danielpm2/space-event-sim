#pragma once

#include "render/SimRenderer.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace core {
class Simulation;
}

namespace render {

// One entry per simulation. This is the only place core sims and their
// renderers are paired; add a new simulation by appending to registry().
struct SimEntry {
    std::string name;
    std::string description;
    std::function<std::unique_ptr<core::Simulation>()> makeSimulation;
    std::function<std::unique_ptr<SimRenderer>()> makeRenderer;
};

const std::vector<SimEntry>& registry();

} // namespace render
