#include "render/App.h"

#include "render/MainMenu.h"
#include "render/registry.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <vector>

namespace render {

App::App()
    : m_window(1280, 720, "Space Event Simulator"),
      m_imgui(m_window.handle()),
      m_menuBg("fullscreen.vert.glsl", "menu_bg.frag.glsl") {}

bool App::keyPressedOnce(int key) {
    const bool down = glfwGetKey(m_window.handle(), key) == GLFW_PRESS;
    bool& was = m_keyDown[key];
    const bool fired = down && !was;
    was = down;
    return fired;
}

void App::launchSimulation(size_t index) {
    m_error.clear();
    try {
        const SimEntry& entry = registry().at(index);
        auto renderer = entry.makeRenderer();
        m_sim = entry.makeSimulation();
        m_renderer = std::move(renderer);
    } catch (const std::exception& e) {
        m_error = e.what();
        std::fprintf(stderr, "%s\n", m_error.c_str());
        m_sim.reset();
        m_renderer.reset();
    }
}

void App::backToMenu() {
    m_renderer.reset();
    m_sim.reset();
}

void App::drawSimulationUi(int, int) {
    ImGui::SetNextWindowPos(ImVec2(12.f, 12.f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin(m_sim->name().c_str(), nullptr, flags)) {
        for (auto& p : m_sim->params())
            ImGui::SliderFloat(p.name.c_str(), p.value, p.min, p.max);
        ImGui::Separator();
        if (ImGui::Button("Back to menu (Esc)"))
            backToMenu();
    }
    ImGui::End();
}

int App::run() {
    double last = m_window.time();
    std::vector<std::string> names;
    for (const auto& e : registry())
        names.push_back(e.name);

    while (!m_window.shouldClose()) {
        m_window.pollEvents();

        const double now = m_window.time();
        const double dt = std::min(now - last, 0.1);
        last = now;

        const bool esc = keyPressedOnce(GLFW_KEY_ESCAPE);
        if (keyPressedOnce(GLFW_KEY_F11))
            m_window.toggleFullscreen();

        int w, h;
        m_window.framebufferSize(w, h);
        glViewport(0, 0, w, h);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_sim && esc)
            backToMenu();
        else if (!m_sim && esc)
            m_window.requestClose();

        if (h > 0 && w > 0) {
            if (m_sim) {
                m_sim->update(dt);
                m_renderer->draw(*m_sim, w, h);
            } else {
                m_menuBg.use();
                m_menuBg.set("uResolution", glm::vec2(w, h));
                m_menuBg.set("uTime", static_cast<float>(now));
                m_triangle.draw();
            }
        }

        m_imgui.beginFrame();
        if (m_sim) {
            drawSimulationUi(w, h);
        } else {
            const MenuResult r = drawMainMenu(names, m_error);
            if (r.quit)
                m_window.requestClose();
            else if (r.selected >= 0)
                launchSimulation(static_cast<size_t>(r.selected));
        }
        m_imgui.endFrame();

        m_window.swapBuffers();
    }
    return 0;
}

} // namespace render
