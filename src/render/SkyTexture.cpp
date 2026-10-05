#include "render/SkyTexture.h"

#include "render/Paths.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <stb_image.h>

namespace render {

namespace {

constexpr int kSkyUnit = 7;
constexpr const char* kSkyFile = "sky/milky_way_8k.jpg";

struct Sky {
    GLuint tex = 0;
    int width = 0;

    Sky() {
        const auto path = (assetDir() / kSkyFile).string();
        int w = 0, h = 0, n = 0;
        unsigned char* px = stbi_load(path.c_str(), &w, &h, &n, 3);
        if (!px) {
            std::fprintf(stderr, "[Sky] cannot load %s (%s); using procedural stars only\n", path.c_str(),
                         stbi_failure_reason());
            return;
        }
        GLint maxSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
        if (w > maxSize) {
            std::fprintf(stderr, "[Sky] %dx%d exceeds GL_MAX_TEXTURE_SIZE %d; using procedural stars only\n", w, h,
                         maxSize);
            stbi_image_free(px);
            return;
        }

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        stbi_image_free(px);
        width = w;
    }

    ~Sky() {
        if (tex)
            glDeleteTextures(1, &tex);
    }
};

} // namespace

void bindSky(const Shader& shader, int screenHeight, float fovYDegrees) {
    static Sky sky;
    shader.set("uSkyEnabled", sky.tex ? 1.f : 0.f);
    if (!sky.tex)
        return;

    // Mip level is chosen from the pixel's angular size; derivatives are unusable
    // across the equirect seam and inside the black-hole march loop.
    const float pixelAngle = glm::radians(fovYDegrees) / static_cast<float>(std::max(screenHeight, 1));
    const float texelsPerPixel = pixelAngle * sky.width / (2.f * glm::pi<float>());
    shader.set("uSkyLod", std::max(0.f, std::log2(std::max(texelsPerPixel, 1e-4f))));

    glActiveTexture(GL_TEXTURE0 + kSkyUnit);
    glBindTexture(GL_TEXTURE_2D, sky.tex);
    glActiveTexture(GL_TEXTURE0);
    shader.set("uSky", kSkyUnit);
}

} // namespace render
