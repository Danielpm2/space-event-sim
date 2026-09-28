#include "render/registry.h"

#include "core/BlackHoleSim.h"
#include "core/MergerSim.h"
#include "core/PulsarSim.h"
#include "core/SupernovaSim.h"
#include "render/BlackHoleRenderer.h"
#include "render/MergerRenderer.h"
#include "render/PulsarRenderer.h"
#include "render/SupernovaRenderer.h"

namespace render {

const std::vector<SimEntry>& registry() {
    static const std::vector<SimEntry> entries = {
        {"Black Hole",
         [] { return std::make_unique<core::BlackHoleSim>(); },
         [] { return std::make_unique<BlackHoleRenderer>(); }},
        {"Pulsar",
         [] { return std::make_unique<core::PulsarSim>(); },
         [] { return std::make_unique<PulsarRenderer>(); }},
        {"Supernova",
         [] { return std::make_unique<core::SupernovaSim>(); },
         [] { return std::make_unique<SupernovaRenderer>(); }},
        {"Neutron Star Merger",
         [] { return std::make_unique<core::MergerSim>(); },
         [] { return std::make_unique<MergerRenderer>(); }},
    };
    return entries;
}

} // namespace render
