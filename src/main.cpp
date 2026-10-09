#include "render/App.h"
#include "render/registry.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // namespace

// Usage: spacesim [--sim <name>] [--warp <seconds>] [--screenshot <file.ppm>] [--no-bloom] [--no-ui]
//                 [--record <prefix> <frames>] [--orbit <rad/s>] [--approach <0..1>]
int main(int argc, char** argv) {
    try {
        render::App app;
        double warp = 0.0;
        double approach = -1.0;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--no-bloom") == 0)
                app.postFx().bloomEnabled = false;
            if (std::strcmp(argv[i], "--no-ui") == 0)
                app.hideUi();
        }
        for (int i = 1; i + 1 < argc; ++i) {
            if (std::strcmp(argv[i], "--record") == 0 && i + 2 < argc)
                app.recordAndExit(argv[i + 1], std::atoi(argv[i + 2]));
            if (std::strcmp(argv[i], "--orbit") == 0)
                app.setAutoOrbit(static_cast<float>(std::atof(argv[i + 1])));
            if (std::strcmp(argv[i], "--screenshot") == 0)
                app.captureAndExit(argv[i + 1], 90);
            if (std::strcmp(argv[i], "--warp") == 0)
                warp = std::atof(argv[i + 1]);
            if (std::strcmp(argv[i], "--approach") == 0)
                approach = std::atof(argv[i + 1]);
            if (std::strcmp(argv[i], "--sim") != 0)
                continue;
            const auto& entries = render::registry();
            for (size_t k = 0; k < entries.size(); ++k)
                if (lower(entries[k].name) == lower(argv[i + 1]))
                    app.launchSimulation(k);
        }
        if (approach >= 0.0)
            app.startApproach(static_cast<float>(approach));
        app.warpSimulation(warp);
        return app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
}
