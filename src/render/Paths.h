#pragma once

#include <filesystem>

namespace render {

// Directory holding the runtime-loaded .glsl files. Looks next to the
// executable first, then at the source tree, then ./shaders.
std::filesystem::path shaderDir();

} // namespace render
