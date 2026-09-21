#pragma once

#include "render/FullscreenTriangle.h"
#include "render/ImGuiLayer.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"
#include "render/Window.h"

#include "core/Simulation.h"

#include <memory>
#include <string>

namespace render {

// Owns the window and drives the menu <-> simulation state machine.
class App {
public:
    App();
    int run();
    void launchSimulation(size_t index);
    // Dev aid: after `frames` frames write a PPM of the framebuffer and exit.
    void captureAndExit(std::string path, int frames) {
        m_capturePath = std::move(path);
        m_captureFrames = frames;
    }

private:
    void backToMenu();
    void handleCameraInput(double dt);
    void drawSimulationUi(int width, int height);
    bool keyPressedOnce(int key);

    Window m_window;
    ImGuiLayer m_imgui;
    Shader m_menuBg;
    FullscreenTriangle m_triangle;

    std::unique_ptr<core::Simulation> m_sim;
    std::unique_ptr<SimRenderer> m_renderer;
    std::string m_error;
    std::string m_capturePath;
    int m_captureFrames = 0;
    bool m_dragging = false;
    bool m_wasMouseDown = false;
    bool m_keyDown[512] = {};
};

} // namespace render
