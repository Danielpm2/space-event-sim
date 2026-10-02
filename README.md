# Space Event Simulator

A small OpenGL 3.3 core / C++17 app with a menu of space-event simulations.

- **Black Hole**: fragment-shader raymarched gravitational lensing, Doppler-beamed accretion disk, lensed starfield.
- **Pulsar**: tilted magnetic axis, two lighthouse beams, particle stream, flare when a beam sweeps the viewer.
- **Supernova**: core collapse, flash, and an expanding clumpy shock shell (volumetric raymarch); loops.
- **Neutron Star Merger**: inspiral on a curved spacetime grid with gravitational-wave ripples, merger flash, jets and kilonova ejecta; loops.
- **Magnetar**: twisted dipole field lines that build up stress until a starquake flare releases it.

## Build (Linux)

Requirements: CMake >= 3.20, a C++17 compiler, git, network access (GLFW, GLM and Dear ImGui are fetched by CMake), and X11 development packages for GLFW.

```sh
# Debian / Ubuntu / Pop!_OS
sudo apt install build-essential cmake git libgl1-mesa-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

cmake -S . -B build
cmake --build build -j
./build/spacesim
```

GLFW's Wayland backend is built only if `libwayland-dev`, `libxkbcommon-dev`, `wayland-protocols` and `wayland-scanner` are installed; otherwise the X11 backend is used (XWayland on Wayland sessions).

Options: `./build/spacesim --sim pulsar` (or `"black hole"`, `supernova`, `"neutron star merger"`, `magnetar`) skips the menu.
Dev aids: `--warp <seconds>` fast-forwards the simulation and `--screenshot <file.ppm>` saves a frame and exits, `--no-bloom` starts with bloom off.

## Controls

| Input | Action |
| --- | --- |
| Left-drag, arrow keys | Orbit camera |
| Scroll, W / S | Zoom |
| Sliders (top-left panel) | Change simulation parameters |
| Esc | Back to menu (quit from the menu) |
| F11 | Toggle fullscreen |

## Layout

```
src/core/     simulation state, physics, camera math (GLM only; no OpenGL/ImGui)
src/render/   window, shader loader, renderers, ImGui UI, registry
shaders/      .glsl files loaded at runtime (copied next to the binary on build)
external/     vendored GLAD (OpenGL 4.6 core loader, used with a 3.3 context)
```

## Adding a simulation

1. Add a class deriving `core::Simulation` in `src/core/` and list it in `sim_core` in `CMakeLists.txt`.
2. Add a `render::SimRenderer` subclass in `src/render/` and its shaders in `shaders/`.
3. Add one entry to `registry()` in `src/render/registry.cpp`.
