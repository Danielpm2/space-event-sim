# Space Event Simulator

Watch the universe's most violent events happen in real time. A small OpenGL 3.3 core / C++17 app with five interactive simulations: orbit the camera, tweak the physics with sliders, and watch what changes.

![Black hole with a lensed Milky Way behind it](docs/images/black_hole.jpg)

## The simulations

### Black Hole
A fragment-shader raymarcher bends light rays through a Schwarzschild field, so the glowing accretion disk appears wrapped over and under the hole. The approaching side of the disk is Doppler-beamed brighter, and the real Milky Way behind it is lensed too.

### Pulsar
A neutron star spins with its magnetic axis tilted away from the rotation axis. Two radiation beams sweep around like a lighthouse, and the whole screen flares when one points at you.

![Pulsar with two sweeping beams](docs/images/pulsar.jpg)

### Supernova
A massive star swells, then collapses. A blinding flash at core bounce launches a clumpy, volumetric shock shell that expands and fades before the cycle loops.

![Supernova shock shell](docs/images/supernova.jpg)

### Neutron Star Merger
Two neutron stars spiral together on a curved spacetime grid that carries gravitational-wave ripples. Then comes the merger flash, relativistic jets and a kilonova cloud.

| Inspiral | Aftermath |
| --- | --- |
| ![Two neutron stars in the inspiral phase](docs/images/merger_inspiral.jpg) | ![Kilonova after the merger](docs/images/merger_kilonova.jpg) |

### Magnetar
Twisted dipole field lines build up stress until a starquake releases it as a giant flare.

![Magnetar field lines](docs/images/magnetar.jpg)

The math behind each event is listed with sources in [docs/REFERENCES.md](docs/REFERENCES.md).

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
Dev aids: `--warp <seconds>` fast-forwards the simulation, `--screenshot <file.ppm>` saves a frame and exits, and `--no-ui` hides all interface elements (used for the images in `docs/images/`).

## Controls

| Input | Action |
| --- | --- |
| Left-drag, arrow keys | Orbit camera |
| Scroll, W / S | Zoom |
| Sliders (top-left panel) | Change simulation parameters |
| Esc | Back to menu (quit from the menu) |
| F11 | Toggle fullscreen |
| Space | Pause / resume |
| H | Hide / show the interface |
| F1 | Controls help overlay |
| 1-5 (menu) | Launch a simulation |
| B | Toggle bloom (settings under "Post-processing" in the panel) |

## Layout

```
src/core/     simulation state, physics, camera math (GLM only; no OpenGL/ImGui)
src/render/   window, shader loader, renderers, ImGui UI, registry
shaders/      .glsl files loaded at runtime (copied next to the binary on build)
assets/       images loaded at runtime via stb_image (Milky Way sky map; see assets/CREDITS.txt)
external/     vendored GLAD (OpenGL 4.6 core loader, used with a 3.3 context)
```

## Adding a simulation

1. Add a class deriving `core::Simulation` in `src/core/` and list it in `sim_core` in `CMakeLists.txt`.
2. Add a `render::SimRenderer` subclass in `src/render/` and its shaders in `shaders/`.
3. Add one entry to `registry()` in `src/render/registry.cpp`.
