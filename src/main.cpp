#include "render/App.h"
#include "render/registry.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // namespace

// Usage: spacesim [--sim <name>] [--screenshot <file.ppm>]
int main(int argc, char** argv) {
    try {
        render::App app;
        for (int i = 1; i + 1 < argc; ++i) {
            if (std::strcmp(argv[i], "--screenshot") == 0)
                app.captureAndExit(argv[i + 1], 90);
            if (std::strcmp(argv[i], "--sim") != 0)
                continue;
            const auto& entries = render::registry();
            for (size_t k = 0; k < entries.size(); ++k)
                if (lower(entries[k].name) == lower(argv[i + 1]))
                    app.launchSimulation(k);
        }
        return app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
}
