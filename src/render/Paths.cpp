#include "render/Paths.h"

#include <iterator>
#include <string>
#include <system_error>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#include <vector>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace render {

namespace {

fs::path executableDir() {
#if defined(_WIN32)
    wchar_t buf[32768];
    const DWORD n = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(std::size(buf)));
    if (n > 0 && n < std::size(buf))
        return fs::path(std::wstring(buf, n)).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buf(size);
    if (_NSGetExecutablePath(buf.data(), &size) == 0) {
        std::error_code ec;
        const fs::path p = fs::canonical(buf.data(), ec);
        return (ec ? fs::path(buf.data()) : p).parent_path();
    }
#elif defined(__linux__)
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

fs::path assetDir() {
    static const fs::path dir = [] {
        fs::path candidates[] = {
            executableDir() / "assets",
#ifdef ASSET_SOURCE_DIR
            fs::path(ASSET_SOURCE_DIR),
#endif
            fs::path("assets"),
        };
        for (const auto& c : candidates)
            if (isDir(c))
                return c;
        return fs::path("assets");
    }();
    return dir;
}

} // namespace render
