#pragma once

#include <glad/glad.h>

namespace render {

// Attribute-less full-screen triangle; the vertex shader uses gl_VertexID.
class FullscreenTriangle {
public:
    FullscreenTriangle() { glGenVertexArrays(1, &m_vao); }
    ~FullscreenTriangle() { glDeleteVertexArrays(1, &m_vao); }
    FullscreenTriangle(const FullscreenTriangle&) = delete;
    FullscreenTriangle& operator=(const FullscreenTriangle&) = delete;

    void draw() const {
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

private:
    GLuint m_vao = 0;
};

} // namespace render
