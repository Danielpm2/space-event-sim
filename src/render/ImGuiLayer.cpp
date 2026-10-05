#include "render/ImGuiLayer.h"

#include "render/Paths.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <filesystem>

namespace render {

namespace {

void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.f;
    s.ChildRounding = 4.f;
    s.PopupRounding = 4.f;
    s.FrameRounding = 4.f;
    s.GrabRounding = 3.f;
    s.ScrollbarRounding = 4.f;
    s.WindowPadding = ImVec2(16.f, 14.f);
    s.FramePadding = ImVec2(10.f, 5.f);
    s.ItemSpacing = ImVec2(10.f, 8.f);
    s.ItemInnerSpacing = ImVec2(8.f, 6.f);
    s.GrabMinSize = 10.f;
    s.ScrollbarSize = 10.f;
    s.WindowBorderSize = 1.f;
    s.FrameBorderSize = 0.f;
    s.PopupBorderSize = 1.f;
    s.WindowTitleAlign = ImVec2(0.f, 0.5f);

    auto white = [](float a) { return ImVec4(1.f, 1.f, 1.f, a); };

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.94f, 1.f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.56f, 0.60f, 1.f);
    c[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.06f, 0.86f);
    c[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.97f);
    c[ImGuiCol_Border] = white(0.10f);
    c[ImGuiCol_TitleBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.f, 0.f, 0.f, 0.f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.f, 0.f, 0.f, 0.f);
    c[ImGuiCol_FrameBg] = white(0.07f);
    c[ImGuiCol_FrameBgHovered] = white(0.11f);
    c[ImGuiCol_FrameBgActive] = white(0.15f);
    c[ImGuiCol_SliderGrab] = white(0.70f);
    c[ImGuiCol_SliderGrabActive] = white(0.95f);
    c[ImGuiCol_CheckMark] = white(0.90f);
    c[ImGuiCol_Button] = white(0.08f);
    c[ImGuiCol_ButtonHovered] = white(0.14f);
    c[ImGuiCol_ButtonActive] = white(0.20f);
    c[ImGuiCol_Header] = white(0.07f);
    c[ImGuiCol_HeaderHovered] = white(0.12f);
    c[ImGuiCol_HeaderActive] = white(0.16f);
    c[ImGuiCol_Separator] = white(0.10f);
    c[ImGuiCol_TableRowBg] = white(0.f);
    c[ImGuiCol_TableRowBgAlt] = white(0.03f);
    c[ImGuiCol_ScrollbarGrab] = white(0.18f);
    c[ImGuiCol_ScrollbarGrabHovered] = white(0.28f);
    c[ImGuiCol_ScrollbarGrabActive] = white(0.38f);
}

ImFont* addFont(ImGuiIO& io, const char* file, float px) {
    const auto path = assetDir() / "fonts" / file;
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return nullptr;
    return io.Fonts->AddFontFromFileTTF(path.string().c_str(), px);
}

} // namespace

ImGuiLayer::ImGuiLayer(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    applyTheme();

    float xs = 1.f, ys = 1.f;
    glfwGetWindowContentScale(window, &xs, &ys);
    const float scale = std::max(1.f, std::max(xs, ys));

    // Font order is relied on by MainMenu: 0 body, 1 card title, 2 heading.
    if (!addFont(io, "Roboto-Regular.ttf", 16.f * scale)) {
        ImFontConfig fontCfg;
        fontCfg.SizePixels = 16.f * scale;
        io.Fonts->AddFontDefault(&fontCfg);
    } else {
        addFont(io, "Roboto-Medium.ttf", 20.f * scale);
        addFont(io, "Roboto-Medium.ttf", 36.f * scale);
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

ImGuiLayer::~ImGuiLayer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiLayer::wantsMouse() const { return ImGui::GetIO().WantCaptureMouse; }
bool ImGuiLayer::wantsKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

ImGuiLayer::MouseState ImGuiLayer::mouse() const {
    const ImGuiIO& io = ImGui::GetIO();
    return {io.MouseDelta.x, io.MouseDelta.y, io.MouseWheel, io.MouseDown[0]};
}

float ImGuiLayer::fps() const { return ImGui::GetIO().Framerate; }

} // namespace render
