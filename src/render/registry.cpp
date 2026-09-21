#include "render/registry.h"

#include "core/BlackHoleSim.h"
#include "core/PulsarSim.h"
#include "render/BlackHoleRenderer.h"
#include "render/PulsarRenderer.h"

namespace render {

const std::vector<SimEntry>& registry() {
    static const std::vector<SimEntry> entries = {
        {"Black Hole",
         [] { return std::make_unique<core::BlackHoleSim>(); },
         [] { return std::make_unique<BlackHoleRenderer>(); }},
        {"Pulsar",
         [] { return std::make_unique<core::PulsarSim>(); },
         [] { return std::make_unique<PulsarRenderer>(); }},
    };
    return entries;
}

} // namespace render
