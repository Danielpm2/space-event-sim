#pragma once

#include <glad/glad.h>

#include <cstdio>

namespace render {

// Logs pending GL errors (capped so a broken frame can't flood the console).
inline bool checkGl(const char* where) {
    static int logged = 0;
    bool any = false;
    for (GLenum e = glGetError(); e != GL_NO_ERROR; e = glGetError()) {
        any = true;
        if (logged++ < 20)
            std::fprintf(stderr, "[GL] error 0x%04X at %s\n", e, where);
    }
    return any;
}

inline const char* framebufferStatusName(GLenum s) {
    switch (s) {
    case GL_FRAMEBUFFER_COMPLETE: return "COMPLETE";
    case GL_FRAMEBUFFER_UNDEFINED: return "UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "INCOMPLETE_DRAW_BUFFER";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "INCOMPLETE_READ_BUFFER";
    case GL_FRAMEBUFFER_UNSUPPORTED: return "UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "INCOMPLETE_MULTISAMPLE";
    default: return "UNKNOWN";
    }
}

} // namespace render
