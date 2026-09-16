#pragma once

struct GLFWwindow;

namespace render {

// RAII wrapper around the Dear ImGui GLFW + OpenGL3 backends.
class ImGuiLayer {
public:
    explicit ImGuiLayer(GLFWwindow* window);
    ~ImGuiLayer();
    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    void beginFrame();
    void endFrame();

    // True when ImGui wants the mouse/keyboard (e.g. hovering a slider).
    bool wantsMouse() const;
    bool wantsKeyboard() const;
};

} // namespace render
