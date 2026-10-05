#pragma once

#include "render/Shader.h"

namespace render {

// Binds the equirectangular Milky Way image to the sampler declared in
// shaders/common/stars.glsl. Loaded on first use; if the asset is missing the
// shaders fall back to the procedural starfield alone.
void bindSky(const Shader& shader, int screenHeight, float fovYDegrees);

} // namespace render
