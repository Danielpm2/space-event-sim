#pragma once

#include <glad/glad.h>

#include <vector>

namespace render {

// Unit UV-sphere; attribute 0 is the vec3 position (== normal).
class SphereMesh {
public:
    explicit SphereMesh(int stacks = 32, int sectors = 64);
    ~SphereMesh();
    SphereMesh(const SphereMesh&) = delete;
    SphereMesh& operator=(const SphereMesh&) = delete;

    void draw() const;

private:
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    GLsizei m_indexCount = 0;
};

// Per-frame streamed vec4 vertices (attribute 0), drawn as points or lines.
class StreamBuffer4 {
public:
    StreamBuffer4();
    ~StreamBuffer4();
    StreamBuffer4(const StreamBuffer4&) = delete;
    StreamBuffer4& operator=(const StreamBuffer4&) = delete;

    void upload(const std::vector<float>& xyzw);
    void draw(GLenum mode) const;
    bool empty() const { return m_count == 0; }

private:
    GLuint m_vao = 0, m_vbo = 0;
    GLsizei m_count = 0;
};

// Attribute-less quad for shaders that build corners from gl_VertexID.
class QuadStrip {
public:
    QuadStrip() { glGenVertexArrays(1, &m_vao); }
    ~QuadStrip() { glDeleteVertexArrays(1, &m_vao); }
    QuadStrip(const QuadStrip&) = delete;
    QuadStrip& operator=(const QuadStrip&) = delete;

    void draw() const {
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

private:
    GLuint m_vao = 0;
};

} // namespace render
