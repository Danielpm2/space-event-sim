#pragma once

#include <filesystem>

namespace render {

// Directory holding the runtime-loaded .glsl files. Looks next to the
// executable first, then at the source tree, then ./shaders.
std::filesystem::path shaderDir();

// Directory holding runtime-loaded images/models; same lookup order as shaderDir().
std::filesystem::path assetDir();

} // namespace render
