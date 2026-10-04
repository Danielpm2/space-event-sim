#include "render/registry.h"

#include "core/BlackHoleSim.h"
#include "core/MagnetarSim.h"
#include "core/MergerSim.h"
#include "core/PulsarSim.h"
#include "core/SupernovaSim.h"
#include "render/BlackHoleRenderer.h"
#include "render/MagnetarRenderer.h"
#include "render/MergerRenderer.h"
#include "render/PulsarRenderer.h"
#include "render/SupernovaRenderer.h"

namespace render {

const std::vector<SimEntry>& registry() {
    static const std::vector<SimEntry> entries = {
        {"Black Hole",
         "Gravitational lensing, a glowing accretion disk and Doppler beaming.",
         [] { return std::make_unique<core::BlackHoleSim>(); },
         [] { return std::make_unique<BlackHoleRenderer>(); }},
        {"Pulsar",
         "A spinning neutron star sweeping twin radiation beams like a lighthouse.",
         [] { return std::make_unique<core::PulsarSim>(); },
         [] { return std::make_unique<PulsarRenderer>(); }},
        {"Supernova",
         "Core collapse, a blinding flash and an expanding shock shell.",
         [] { return std::make_unique<core::SupernovaSim>(); },
         [] { return std::make_unique<SupernovaRenderer>(); }},
        {"Neutron Star Merger",
         "Two neutron stars spiral together, ripple spacetime and ignite a kilonova.",
         [] { return std::make_unique<core::MergerSim>(); },
         [] { return std::make_unique<MergerRenderer>(); }},
        {"Magnetar",
         "Twisted magnetic field lines build stress until a starquake flare.",
         [] { return std::make_unique<core::MagnetarSim>(); },
         [] { return std::make_unique<MagnetarRenderer>(); }},
    };
    return entries;
}

} // namespace render
