#pragma once

namespace core {
class Simulation;
}

namespace render {

// Draws one simulation type. Constructed with a current GL context, so the
// constructor loads shaders/buffers; it only reads core state in draw().
class SimRenderer {
public:
    virtual ~SimRenderer() = default;
    virtual void draw(const core::Simulation& sim, int width, int height) = 0;
};

} // namespace render
