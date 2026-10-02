#pragma once

namespace core {

// Post-processing state shared by all simulations; the render layer reads it.
struct PostFxSettings {
    bool bloomEnabled = true;
    float threshold = 1.0f;
    float intensity = 0.6f;
    float exposure = 1.0f;
    int blurPasses = 5;
};

} // namespace core
