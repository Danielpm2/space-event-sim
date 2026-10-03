#pragma once

#include <string>

struct GLFWwindow;

namespace render {

// Owns the GLFW window and the OpenGL 3.3 core context.
class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void requestClose();
    void pollEvents();
    void waitEvents(double timeoutSeconds);
    void swapBuffers();

    void toggleFullscreen();
    void framebufferSize(int& w, int& h) const;
    double time() const;

    GLFWwindow* handle() const { return m_window; }

private:
    GLFWwindow* m_window = nullptr;
    bool m_fullscreen = false;
    int m_winX = 0, m_winY = 0, m_winW = 0, m_winH = 0;
};

} // namespace render
