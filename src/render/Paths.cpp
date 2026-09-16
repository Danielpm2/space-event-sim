#include "render/Paths.h"

#include <system_error>

#if defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace render {

namespace {

fs::path executableDir() {
#if defined(__linux__)
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return fs::path(buf).parent_path();
    }
#endif
    return {};
}

bool isDir(const fs::path& p) {
    std::error_code ec;
    return !p.empty() && fs::is_directory(p, ec);
}

} // namespace

fs::path shaderDir() {
    static const fs::path dir = [] {
        fs::path candidates[] = {
            executableDir() / "shaders",
#ifdef SHADER_SOURCE_DIR
            fs::path(SHADER_SOURCE_DIR),
#endif
            fs::path("shaders"),
        };
        for (const auto& c : candidates)
            if (isDir(c))
                return c;
        return fs::path("shaders");
    }();
    return dir;
}

} // namespace render
