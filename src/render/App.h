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

private:
    void backToMenu();
    void drawSimulationUi(int width, int height);
    bool keyPressedOnce(int key);

    Window m_window;
    ImGuiLayer m_imgui;
    Shader m_menuBg;
    FullscreenTriangle m_triangle;

    std::unique_ptr<core::Simulation> m_sim;
    std::unique_ptr<SimRenderer> m_renderer;
    std::string m_error;
    bool m_keyDown[512] = {};
};

} // namespace render
