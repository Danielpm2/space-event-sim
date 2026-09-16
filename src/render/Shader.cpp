#include "render/Shader.h"

#include "render/Paths.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace render {

namespace {

std::string readFile(const std::string& name) {
    const auto path = shaderDir() / name;
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open shader file: " + path.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

GLuint compile(GLenum type, const std::string& file) {
    const std::string src = readFile(file);
    const char* csrc = src.c_str();
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &csrc, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        glDeleteShader(s);
        throw std::runtime_error("Shader compile error in " + file + ":\n" + log.data());
    }
    return s;
}

} // namespace

Shader::Shader(const std::string& vertFile, const std::string& fragFile) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertFile);
    GLuint fs;
    try {
        fs = compile(GL_FRAGMENT_SHADER, fragFile);
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vs);
    glAttachShader(m_id, fs);
    glLinkProgram(m_id);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1);
        glGetProgramInfoLog(m_id, len, nullptr, log.data());
        glDeleteProgram(m_id);
        m_id = 0;
        throw std::runtime_error("Shader link error (" + vertFile + " + " + fragFile + "):\n" + log.data());
    }
}

Shader::~Shader() {
    if (m_id)
        glDeleteProgram(m_id);
}

Shader::Shader(Shader&& o) noexcept : m_id(o.m_id), m_locations(std::move(o.m_locations)) {
    o.m_id = 0;
}

Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (m_id)
            glDeleteProgram(m_id);
        m_id = o.m_id;
        m_locations = std::move(o.m_locations);
        o.m_id = 0;
    }
    return *this;
}

GLint Shader::location(const char* name) const {
    auto it = m_locations.find(name);
    if (it != m_locations.end())
        return it->second;
    GLint loc = glGetUniformLocation(m_id, name);
    m_locations.emplace(name, loc);
    return loc;
}

void Shader::set(const char* n, int v) const { glUniform1i(location(n), v); }
void Shader::set(const char* n, float v) const { glUniform1f(location(n), v); }
void Shader::set(const char* n, const glm::vec2& v) const { glUniform2fv(location(n), 1, glm::value_ptr(v)); }
void Shader::set(const char* n, const glm::vec3& v) const { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
void Shader::set(const char* n, const glm::vec4& v) const { glUniform4fv(location(n), 1, glm::value_ptr(v)); }
void Shader::set(const char* n, const glm::mat4& v) const {
    glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(v));
}

} // namespace render
