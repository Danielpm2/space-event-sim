#include "render/App.h"

#include "render/MainMenu.h"
#include "render/Screenshot.h"
#include "render/registry.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
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
        m_defaultCamera = m_sim->camera;
        m_activeIndex = index;
        m_paused = false;
        m_uiVisible = true;
    } catch (const std::exception& e) {
        m_error = e.what();
        std::fprintf(stderr, "%s\n", m_error.c_str());
        m_sim.reset();
        m_renderer.reset();
    }
}

void App::warpSimulation(double seconds) {
    if (!m_sim)
        return;
    for (; seconds > 0.0; seconds -= 0.05)
        m_sim->update(std::min(seconds, 0.05));
}

void App::backToMenu() {
    m_renderer.reset();
    m_sim.reset();
    m_paused = false;
    m_uiVisible = true;
    m_fade = 1.f;
}

void App::drawSimulationUi() {
    if (!m_uiVisible)
        return;

    const SimEntry& entry = registry().at(m_activeIndex);
    ImGui::SetNextWindowPos(ImVec2(14.f, 14.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400.f, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.78f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin(entry.name.c_str(), nullptr, flags)) {
        ImGui::PushTextWrapPos(0.f);
        ImGui::TextDisabled("%s", entry.description.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.52f);
        if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& p : m_sim->params()) {
                ImGui::SliderFloat(p.name.c_str(), p.value, p.min, p.max, "%.2f");
                ImGui::SetItemTooltip("Ctrl+click to type a value");
            }
        }
        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::SliderFloat("Field of view", &m_sim->camera.fovY, 25.f, 90.f, "%.0f deg");
            if (ImGui::Button("Reset camera", ImVec2(-1.f, 0.f)))
                m_sim->camera = m_defaultCamera;
        }
        if (ImGui::CollapsingHeader("Post-processing")) {
            ImGui::Checkbox("Bloom (B)", &m_fx.bloomEnabled);
            ImGui::BeginDisabled(!m_fx.bloomEnabled);
            ImGui::SliderFloat("Threshold", &m_fx.threshold, 0.f, 5.f);
            ImGui::SliderFloat("Intensity", &m_fx.intensity, 0.f, 2.f);
            ImGui::SliderInt("Blur passes", &m_fx.blurPasses, 1, 12);
            ImGui::EndDisabled();
            ImGui::SliderFloat("Exposure", &m_fx.exposure, 0.1f, 4.f);
            if (ImGui::Button("Reset post-processing", ImVec2(-1.f, 0.f)))
                m_fx = core::PostFxSettings{};
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();
        const float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (ImGui::Button(m_paused ? "Resume (Space)" : "Pause (Space)", ImVec2(half, 0.f)))
            m_paused = !m_paused;
        ImGui::SameLine();
        if (ImGui::Button("Menu (Esc)", ImVec2(half, 0.f)))
            backToMenu();
        ImGui::TextDisabled("F1 help   H hide UI   F11 fullscreen");
    }
    ImGui::End();
}

void App::drawHelp() {
    if (!m_showHelp)
        return;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.92f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Controls", &m_showHelp, flags)) {
        static const char* rows[][2] = {
            {"Left-drag", "Orbit the camera"},
            {"Scroll  /  W S", "Zoom in and out"},
            {"Arrow keys", "Orbit the camera"},
            {"Space", "Pause / resume the simulation"},
            {"B", "Toggle bloom"},
            {"H", "Hide / show the interface"},
            {"F11", "Toggle fullscreen"},
            {"F1", "Show / hide this help"},
            {"Esc", "Back to the menu"},
        };
        if (ImGui::BeginTable("help", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
            for (const auto& r : rows) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive), "%s", r[0]);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(r[1]);
            }
            ImGui::EndTable();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("Ctrl+click a slider to type an exact value.");
    }
    ImGui::End();
}

void App::drawOverlays() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    const float base = ImGui::GetFontSize();

    if (m_sim && m_paused) {
        const char* text = "PAUSED";
        const ImVec2 ts = font->CalcTextSizeA(base * 1.4f, FLT_MAX, 0.f, text);
        dl->AddText(font, base * 1.4f, ImVec2(vp->Pos.x + (vp->Size.x - ts.x) * 0.5f, vp->Pos.y + 18.f),
                    IM_COL32(255, 255, 255, 200), text);
    }
    if (m_sim && !m_uiVisible) {
        const char* text = "H: show interface";
        const ImVec2 ts = font->CalcTextSizeA(base * 0.9f, FLT_MAX, 0.f, text);
        dl->AddText(font, base * 0.9f, ImVec2(vp->Pos.x + 14.f, vp->Pos.y + vp->Size.y - ts.y - 12.f),
                    IM_COL32(255, 255, 255, 90), text);
    }
    if (m_fade > 0.f) {
        const int alpha = static_cast<int>(255.f * m_fade * m_fade);
        dl->AddRectFilled(vp->Pos, ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y), IM_COL32(0, 0, 0, alpha));
    }
}

void App::drawFps() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x - 12.f, vp->Pos.y + 12.f), ImGuiCond_Always,
                            ImVec2(1.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                                   ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##fps", nullptr, flags))
        ImGui::Text("%.0f FPS (%.2f ms)", m_imgui.fps(), 1000.f / std::max(m_imgui.fps(), 1.f));
    ImGui::End();
}

void App::handleCameraInput(double dt) {
    core::OrbitCamera& cam = m_sim->camera;
    const auto mouse = m_imgui.mouse();

    if (!mouse.leftDown)
        m_dragging = false;
    else if (!m_dragging && !m_wasMouseDown && !m_imgui.wantsMouse())
        m_dragging = true;
    m_wasMouseDown = mouse.leftDown;

    if (m_dragging)
        cam.orbit(-mouse.dx * 0.005f, mouse.dy * 0.005f);
    if (mouse.scroll != 0.f && !m_imgui.wantsMouse())
        cam.zoom(std::exp(-mouse.scroll * 0.1f));

    if (!m_imgui.wantsKeyboard()) {
        GLFWwindow* win = m_window.handle();
        const float step = static_cast<float>(dt);
        const float turn = 1.5f * step;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS)  cam.orbit(-turn, 0.f);
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.orbit(turn, 0.f);
        if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)    cam.orbit(0.f, turn);
        if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS)  cam.orbit(0.f, -turn);
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)     cam.zoom(std::exp(-1.0f * step));
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)     cam.zoom(std::exp(1.0f * step));
    }
}

int App::run() {
    double last = m_window.time();
    std::vector<MenuEntry> names;
    for (const auto& e : registry())
        names.push_back({e.name, e.description});

    while (!m_window.shouldClose()) {
        m_window.pollEvents();

        // A minimized window reports a 0x0 framebuffer; idle instead of spinning.
        int w, h;
        m_window.framebufferSize(w, h);
        if (w <= 0 || h <= 0) {
            m_window.waitEvents(0.1);
            last = m_window.time();
            continue;
        }

        const double now = m_window.time();
        const double dt = std::min(now - last, 0.1);
        last = now;

        const bool esc = keyPressedOnce(GLFW_KEY_ESCAPE);
        const bool f1 = keyPressedOnce(GLFW_KEY_F1);
        const bool hKey = keyPressedOnce(GLFW_KEY_H);
        const bool space = keyPressedOnce(GLFW_KEY_SPACE);
        if (keyPressedOnce(GLFW_KEY_F11))
            m_window.toggleFullscreen();

        m_imgui.beginFrame();
        if (f1)
            m_showHelp = !m_showHelp;
        if (m_sim && !m_imgui.wantsKeyboard()) {
            if (keyPressedOnce(GLFW_KEY_B))
                m_fx.bloomEnabled = !m_fx.bloomEnabled;
            if (hKey)
                m_uiVisible = !m_uiVisible;
            if (space)
                m_paused = !m_paused;
        } else {
            keyPressedOnce(GLFW_KEY_B); // keep the edge detector in sync
        }
        m_fade = std::max(0.f, m_fade - static_cast<float>(dt) / 0.35f);
        if (m_sim)
            handleCameraInput(dt);

        glViewport(0, 0, w, h);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (esc && m_showHelp)
            m_showHelp = false;
        else if (m_sim && esc)
            backToMenu();
        else if (!m_sim && esc)
            m_window.requestClose();

        if (h > 0 && w > 0) {
            if (m_sim) {
                const bool hdr = m_post.beginScene(w, h);
                if (!m_paused)
                    m_sim->update(dt);
                m_renderer->draw(*m_sim, w, h);
                if (hdr)
                    m_post.apply(m_fx, w, h);
            } else {
                m_menuBg.use();
                m_menuBg.set("uResolution", glm::vec2(w, h));
                m_menuBg.set("uTime", static_cast<float>(now));
                m_triangle.draw();
            }
        }

        if (m_sim) {
            drawSimulationUi();
            if (m_uiVisible)
                drawFps();
        } else {
            const MenuResult r = drawMainMenu(names, m_error);
            if (r.quit) {
                m_window.requestClose();
            } else if (r.selected >= 0) {
                launchSimulation(static_cast<size_t>(r.selected));
                if (m_sim)
                    m_fade = 1.f;
            }
            drawFps();
        }
        drawHelp();
        drawOverlays();
        m_imgui.endFrame();

        if (!m_capturePath.empty() && --m_captureFrames <= 0) {
            if (!saveScreenshotPPM(m_capturePath, w, h))
                std::fprintf(stderr, "Cannot write %s\n", m_capturePath.c_str());
            m_window.requestClose();
        }

        m_window.swapBuffers();
    }
    return 0;
}

} // namespace render
