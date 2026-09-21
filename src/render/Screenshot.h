#pragma once

#include <string>

namespace render {

// Reads the default framebuffer and writes a binary PPM. Returns false on I/O failure.
bool saveScreenshotPPM(const std::string& path, int width, int height);

} // namespace render
