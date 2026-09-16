#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

namespace render {

// Compiles and links a program from two .glsl files in the shader directory.
// Throws std::runtime_error with the GL log on failure.
class Shader {
public:
    Shader(const std::string& vertFile, const std::string& fragFile);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;

    void use() const { glUseProgram(m_id); }
    GLuint id() const { return m_id; }

    void set(const char* name, int v) const;
    void set(const char* name, float v) const;
    void set(const char* name, const glm::vec2& v) const;
    void set(const char* name, const glm::vec3& v) const;
    void set(const char* name, const glm::vec4& v) const;
    void set(const char* name, const glm::mat4& v) const;

private:
    GLint location(const char* name) const;

    GLuint m_id = 0;
    mutable std::unordered_map<std::string, GLint> m_locations;
};

} // namespace render
