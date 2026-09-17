#include "render/registry.h"

#include "core/StubSim.h"
#include "render/StubRenderer.h"

namespace render {

const std::vector<SimEntry>& registry() {
    static const std::vector<SimEntry> entries = {
        {"Black Hole",
         [] { return std::make_unique<core::StubSim>("Black Hole", 0.08f); },
         [] { return std::make_unique<StubRenderer>(); }},
        {"Pulsar",
         [] { return std::make_unique<core::StubSim>("Pulsar", 0.6f); },
         [] { return std::make_unique<StubRenderer>(); }},
    };
    return entries;
}

} // namespace render
