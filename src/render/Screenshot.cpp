#include "render/Screenshot.h"

#include <glad/glad.h>

#include <cstdio>
#include <vector>

namespace render {

bool saveScreenshotPPM(const std::string& path, int width, int height) {
    std::vector<unsigned char> px(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, px.data());

    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;
    std::fprintf(f, "P6\n%d %d\n255\n", width, height);
    // GL rows start at the bottom; PPM rows start at the top.
    for (int y = height - 1; y >= 0; --y)
        std::fwrite(&px[static_cast<size_t>(y) * width * 3], 1, static_cast<size_t>(width) * 3, f);
    std::fclose(f);
    return true;
}

} // namespace render
