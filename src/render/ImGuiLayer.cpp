#include "render/ImGuiLayer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>

namespace render {

namespace {

void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 10.f;
    s.ChildRounding = 8.f;
    s.PopupRounding = 8.f;
    s.FrameRounding = 6.f;
    s.GrabRounding = 6.f;
    s.ScrollbarRounding = 8.f;
    s.WindowPadding = ImVec2(14.f, 12.f);
    s.FramePadding = ImVec2(9.f, 5.f);
    s.ItemSpacing = ImVec2(8.f, 8.f);
    s.ItemInnerSpacing = ImVec2(6.f, 6.f);
    s.GrabMinSize = 12.f;
    s.WindowBorderSize = 0.f;
    s.FrameBorderSize = 0.f;
    s.PopupBorderSize = 1.f;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    const ImVec4 accent(0.30f, 0.62f, 1.00f, 1.00f);
    const ImVec4 accentHi(0.42f, 0.72f, 1.00f, 1.00f);
    const ImVec4 panel(0.05f, 0.06f, 0.10f, 0.84f);
    const ImVec4 widget(0.14f, 0.17f, 0.25f, 0.90f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.98f, 1.f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.60f, 0.70f, 1.f);
    c[ImGuiCol_WindowBg] = panel;
    c[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.08f, 0.13f, 0.96f);
    c[ImGuiCol_Border] = ImVec4(0.30f, 0.40f, 0.60f, 0.35f);
    c[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.07f, 0.12f, 0.90f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.11f, 0.20f, 0.95f);
    c[ImGuiCol_FrameBg] = widget;
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.26f, 0.38f, 0.95f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.32f, 0.48f, 1.f);
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = accentHi;
    c[ImGuiCol_CheckMark] = accentHi;
    c[ImGuiCol_Button] = ImVec4(0.17f, 0.30f, 0.52f, 0.85f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.42f, 0.72f, 0.95f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.52f, 0.90f, 1.f);
    c[ImGuiCol_Header] = ImVec4(0.17f, 0.30f, 0.52f, 0.55f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.42f, 0.72f, 0.75f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.52f, 0.90f, 0.90f);
    c[ImGuiCol_Separator] = ImVec4(0.30f, 0.40f, 0.60f, 0.35f);
}

} // namespace

ImGuiLayer::ImGuiLayer(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    applyTheme();

    // The embedded default font is vector-based, so a larger size stays crisp on HiDPI screens.
    float xs = 1.f, ys = 1.f;
    glfwGetWindowContentScale(window, &xs, &ys);
    ImFontConfig fontCfg;
    fontCfg.SizePixels = 16.f * std::max(1.f, std::max(xs, ys));
    io.Fonts->AddFontDefault(&fontCfg);

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
