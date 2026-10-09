#include "render/App.h"

#include "core/Easing.h"
#include "render/MainMenu.h"
#include "render/Screenshot.h"
#include "render/SkyTexture.h"
#include "render/registry.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

namespace render {

namespace {

std::string formatDistance(double km) {
    constexpr double kAu = 1.495978707e8, kLightSecond = 299792.458;
    char buf[96];
    if (km >= 0.02 * kAu)
        std::snprintf(buf, sizeof(buf), "%.2f AU  (%.0f light-seconds)", km / kAu, km / kLightSecond);
    else
        std::snprintf(buf, sizeof(buf), "%.3g km", km);
    return buf;
}

std::string formatSpan(double km) {
    constexpr double kSun = 1.3927e6, kEarth = 12742.0;
    const double suns = km / kSun;
    char buf[64];
    if (suns >= 1.0)
        std::snprintf(buf, sizeof(buf), "%.*f Suns across", suns >= 10.0 ? 0 : 1, suns);
    else
        std::snprintf(buf, sizeof(buf), "%.0f Earths across", km / kEarth);
    return buf;
}

} // namespace

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
        m_approach.reset();
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

void App::startApproach(float u) {
    if (!m_sim || !m_sim->supportsApproach())
        return;
    m_approach.start(*m_sim);
    m_approach.setProgress(u);
    // Bring the event to the state it would have reached by then.
    warpSimulation(u * m_approach.duration * m_sim->approachClockScale());
}

void App::toggleApproach() {
    if (!m_sim || !m_sim->supportsApproach())
        return;
    if (m_approach.active())
        m_approach.stop(*m_sim);
    else
        m_approach.start(*m_sim);
}

void App::backToMenu() {
    m_renderer.reset();
    m_sim.reset();
    m_approach.reset();
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
        const bool canApproach = m_sim->supportsApproach();
        if (ImGui::CollapsingHeader("Camera", canApproach ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
            if (canApproach) {
                if (!m_approach.active()) {
                    if (ImGui::Button("Approach the event (A)", ImVec2(-1.f, 0.f)))
                        toggleApproach();
                } else {
                    ImGui::ProgressBar(m_approach.progress(), ImVec2(-1.f, 0.f));
                    const float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
                    if (ImGui::Button("Leave approach (A)", ImVec2(halfW, 0.f)))
                        toggleApproach();
                    ImGui::SameLine();
                    if (ImGui::Button("Restart", ImVec2(halfW, 0.f))) {
                        m_approach.stop(*m_sim);
                        m_approach.start(*m_sim);
                    }
                }
            }
            ImGui::BeginDisabled(m_approach.active());
            ImGui::SliderFloat("Field of view", &m_sim->camera.fovY, 25.f, 90.f, "%.0f deg");
            ImGui::EndDisabled();
            if (ImGui::Button("Reset camera", ImVec2(-1.f, 0.f))) {
                m_approach.stop(*m_sim);
                m_sim->camera = m_defaultCamera;
            }
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
            {"A", "Start / leave the cinematic approach"},
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

void App::drawApproachHud() {
    if (!m_sim || !m_approach.active())
        return;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float base = ImGui::GetFontSize();
    const float x0 = vp->Pos.x, y0 = vp->Pos.y, w = vp->Size.x, h = vp->Size.y;

    // Letterbox bars slide in; the lower one doubles as a progress track.
    const float bar = h * 0.07f * core::smooth01(m_approach.elapsed() / 1.5f);
    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + w, y0 + bar), IM_COL32(0, 0, 0, 255));
    dl->AddRectFilled(ImVec2(x0, y0 + h - bar), ImVec2(x0 + w, y0 + h), IM_COL32(0, 0, 0, 255));
    const float trackY = y0 + h - bar * 0.5f;
    dl->AddRectFilled(ImVec2(x0 + w * 0.04f, trackY), ImVec2(x0 + w * 0.96f, trackY + 2.f), IM_COL32(255, 255, 255, 40));
    dl->AddRectFilled(ImVec2(x0 + w * 0.04f, trackY),
                      ImVec2(x0 + w * (0.04f + 0.92f * m_approach.progress()), trackY + 2.f), IM_COL32(255, 255, 255, 170));

    if (!m_uiVisible)
        return;

    const core::ApproachReadout r = m_sim->approachReadout();
    const float d = m_sim->camera.distance;
    std::vector<std::string> lines;
    lines.push_back("DISTANCE   " + formatDistance(static_cast<double>(d) * r.kmPerUnit));
    char buf[160];
    if (r.enclosing && d < r.radius) {
        std::snprintf(buf, sizeof(buf), "INSIDE   %s", r.subject);
    } else {
        const float deg = 2.f * std::atan(r.radius / std::max(d, 1e-3f)) * 57.29578f;
        std::snprintf(buf, sizeof(buf), "%s subtends %.0f deg of sky", r.subject, deg);
    }
    lines.push_back(buf);
    lines.push_back(std::string(r.subject) + " spans " +
                    formatSpan(2.0 * static_cast<double>(r.radius) * r.kmPerUnit));
    const float dilation = m_sim->approachTimeDilation(d);
    if (dilation < 0.999f) {
        std::snprintf(buf, sizeof(buf), "TIME RUNS AT   %.2fx", dilation);
        lines.push_back(buf);
    }
    if (m_approach.finished())
        lines.push_back("A leave   |   Space pause");

    const float lineH = base * 1.35f;
    float y = y0 + h - bar - 14.f - lineH * static_cast<float>(lines.size());
    for (const std::string& s : lines) {
        dl->AddText(ImVec2(x0 + 26.f, y), IM_COL32(220, 232, 255, 200), s.c_str());
        y += lineH;
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
    const bool flying = m_approach.active();
    // During an approach the same inputs look around instead of moving the camera.
    auto turn = [&](float dYaw, float dPitch) {
        if (flying)
            m_approach.look(dYaw, dPitch);
        else
            cam.orbit(dYaw, dPitch);
    };

    if (!mouse.leftDown)
        m_dragging = false;
    else if (!m_dragging && !m_wasMouseDown && !m_imgui.wantsMouse())
        m_dragging = true;
    m_wasMouseDown = mouse.leftDown;

    if (m_dragging)
        turn(-mouse.dx * 0.005f, mouse.dy * 0.005f);
    if (mouse.scroll != 0.f && !m_imgui.wantsMouse() && !flying)
        cam.zoom(std::exp(-mouse.scroll * 0.1f));

    if (!m_imgui.wantsKeyboard()) {
        GLFWwindow* win = m_window.handle();
        const float step = static_cast<float>(dt);
        const float rate = 1.5f * step;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS)  turn(-rate, 0.f);
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) turn(rate, 0.f);
        if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)    turn(0.f, rate);
        if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS)  turn(0.f, -rate);
        if (!flying && glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) cam.zoom(std::exp(-1.0f * step));
        if (!flying && glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) cam.zoom(std::exp(1.0f * step));
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
        const double dt = m_recordPrefix.empty() ? std::min(now - last, 0.1) : 1.0 / 30.0;
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
            if (keyPressedOnce(GLFW_KEY_A))
                toggleApproach();
        } else {
            keyPressedOnce(GLFW_KEY_B); // keep the edge detector in sync
            keyPressedOnce(GLFW_KEY_A);
        }
        m_fade = std::max(0.f, m_fade - static_cast<float>(dt) / 0.35f);
        if (m_sim) {
            handleCameraInput(dt);
            m_approach.update(*m_sim, m_paused ? 0.0 : dt);
        }
        if (m_sim && m_autoOrbit != 0.f && !m_approach.active())
            m_sim->camera.yaw += m_autoOrbit * static_cast<float>(dt);

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
                    m_sim->update(m_approach.active() ? dt * m_sim->approachClockScale() : dt);
                m_renderer->draw(*m_sim, w, h);
                if (m_approach.active()) {
                    const float intensity = std::min(1.f, m_approach.speed() / 0.4f) * 0.9f;
                    m_approachFx.draw(m_sim->camera, m_approach.velocity(), static_cast<float>(w) / h, intensity);
                }
                if (hdr) {
                    core::PostFxSettings fx = m_fx;
                    if (m_approach.active()) {
                        fx.vignette = 0.25f + 0.45f * m_approach.proximity();
                        fx.aberration = 0.003f + 0.012f * m_approach.shake();
                    }
                    m_post.apply(fx, w, h);
                }
            } else {
                m_menuBg.use();
                m_menuBg.set("uResolution", glm::vec2(w, h));
                m_menuBg.set("uTime", static_cast<float>(now));
                bindSky(m_menuBg, h, 60.f);
                m_triangle.draw();
            }
        }

        if (!m_noUi) {
            if (m_sim) {
                drawSimulationUi();
                drawApproachHud();
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
        }
        m_imgui.endFrame();

        if (!m_capturePath.empty() && --m_captureFrames <= 0) {
            if (!saveScreenshotPPM(m_capturePath, w, h))
                std::fprintf(stderr, "Cannot write %s\n", m_capturePath.c_str());
            m_window.requestClose();
        }

        if (!m_recordPrefix.empty()) {
            char name[1024];
            std::snprintf(name, sizeof(name), "%s%04d.ppm", m_recordPrefix.c_str(), m_recordIndex++);
            if (!saveScreenshotPPM(name, w, h))
                std::fprintf(stderr, "Cannot write %s\n", name);
            if (m_recordIndex >= m_recordFrames)
                m_window.requestClose();
        }

        m_window.swapBuffers();
    }
    return 0;
}

} // namespace render
