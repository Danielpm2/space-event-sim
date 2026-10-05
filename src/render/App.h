#pragma once

#include "render/FullscreenTriangle.h"
#include "render/ImGuiLayer.h"
#include "render/PostProcess.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"
#include "render/Window.h"

#include "core/PostFxSettings.h"
#include "core/Simulation.h"

#include <memory>
#include <string>

namespace render {

// Owns the window and drives the menu <-> simulation state machine.
class App {
public:
    App();
    int run();
    core::PostFxSettings& postFx() { return m_fx; }
    void launchSimulation(size_t index);
    // Dev aid: fast-forward the running simulation by `seconds`.
    void warpSimulation(double seconds);
    // Dev aid: skip all ImGui drawing so screenshots show only the scene.
    void hideUi() { m_noUi = true; }
    // Dev aid: write `frames` PPMs named <prefix>NNNN.ppm at a fixed 30 fps timestep, then exit.
    void recordAndExit(std::string prefix, int frames) {
        m_recordPrefix = std::move(prefix);
        m_recordFrames = frames;
    }
    // Dev aid: turn the camera around the target at `radPerSec`.
    void setAutoOrbit(float radPerSec) { m_autoOrbit = radPerSec; }
    // Dev aid: after `frames` frames write a PPM of the framebuffer and exit.
    void captureAndExit(std::string path, int frames) {
        m_capturePath = std::move(path);
        m_captureFrames = frames;
    }

private:
    void backToMenu();
    void handleCameraInput(double dt);
    void drawSimulationUi();
    void drawHelp();
    void drawOverlays();
    void drawFps();
    bool keyPressedOnce(int key);

    Window m_window;
    ImGuiLayer m_imgui;
    Shader m_menuBg;
    FullscreenTriangle m_triangle;
    PostProcess m_post;
    core::PostFxSettings m_fx;

    std::unique_ptr<core::Simulation> m_sim;
    std::unique_ptr<SimRenderer> m_renderer;
    core::OrbitCamera m_defaultCamera;
    std::string m_error;
    std::string m_capturePath;
    int m_captureFrames = 0;
    std::string m_recordPrefix;
    int m_recordFrames = 0;
    int m_recordIndex = 0;
    float m_autoOrbit = 0.f;
    bool m_dragging = false;
    bool m_wasMouseDown = false;
    size_t m_activeIndex = 0;
    bool m_paused = false;
    bool m_uiVisible = true;
    bool m_noUi = false;
    bool m_showHelp = false;
    float m_fade = 0.f; // 1 = fully black, eases to 0 after a screen change
    bool m_keyDown[512] = {};
};

} // namespace render
