#include "render/Primitives.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace render {

SphereMesh::SphereMesh(int stacks, int sectors) {
    std::vector<float> v;
    std::vector<unsigned> idx;
    for (int i = 0; i <= stacks; ++i) {
        const float phi = glm::pi<float>() * i / stacks;
        for (int j = 0; j <= sectors; ++j) {
            const float th = 2.f * glm::pi<float>() * j / sectors;
            v.push_back(std::sin(phi) * std::cos(th));
            v.push_back(std::cos(phi));
            v.push_back(std::sin(phi) * std::sin(th));
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            const unsigned a = i * (sectors + 1) + j, b = a + sectors + 1;
            idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    m_indexCount = static_cast<GLsizei>(idx.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

SphereMesh::~SphereMesh() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

void SphereMesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}

StreamBuffer4::StreamBuffer4() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

StreamBuffer4::~StreamBuffer4() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
}

void StreamBuffer4::upload(const std::vector<float>& xyzw) {
    m_count = static_cast<GLsizei>(xyzw.size() / 4);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, xyzw.size() * sizeof(float), xyzw.data(), GL_STREAM_DRAW);
}

void StreamBuffer4::draw(GLenum mode) const {
    if (m_count == 0)
        return;
    glBindVertexArray(m_vao);
    glDrawArrays(mode, 0, m_count);
}

} // namespace render
