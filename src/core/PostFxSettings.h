#pragma once

namespace core {

// Post-processing state shared by all simulations; the render layer reads it.
struct PostFxSettings {
    bool bloomEnabled = true;
    float threshold = 1.0f;
    float intensity = 0.6f;
    float exposure = 1.0f;
    int blurPasses = 5;
    float vignette = 0.f;   // 0..1 edge darkening
    float aberration = 0.f; // radial colour fringing, fraction of the screen radius
};

} // namespace core
