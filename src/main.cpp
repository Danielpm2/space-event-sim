#include "render/FullscreenTriangle.h"
#include "render/Shader.h"
#include "render/Window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <exception>

int main() {
    try {
        render::Window window(1280, 720, "Space Event Simulator");
        render::Shader menuBg("fullscreen.vert.glsl", "menu_bg.frag.glsl");
        render::FullscreenTriangle triangle;

        while (!window.shouldClose()) {
            window.pollEvents();
            if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                window.requestClose();

            int w, h;
            window.framebufferSize(w, h);
            glViewport(0, 0, w, h);
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);

            menuBg.use();
            menuBg.set("uResolution", glm::vec2(w, h));
            menuBg.set("uTime", static_cast<float>(window.time()));
            triangle.draw();

            window.swapBuffers();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
