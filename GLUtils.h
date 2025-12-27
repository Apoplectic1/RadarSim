// GLUtils.h - OpenGL utility functions and error checking
#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QDebug>
#include <QString>

namespace GLUtils {

// Convert GL error code to string
inline const char* glErrorString(GLenum error) {
    switch (error) {
    case GL_NO_ERROR:          return "GL_NO_ERROR";
    case GL_INVALID_ENUM:      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:     return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:     return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:                   return "UNKNOWN_ERROR";
    }
}

// Check for GL errors and log them using current context
// Returns true if there was an error
inline bool checkGLError(const char* context = nullptr) {
    auto* ctx = QOpenGLContext::currentContext();
    if (!ctx) return false;

    GLenum error = ctx->functions()->glGetError();
    if (error != GL_NO_ERROR) {
        if (context) {
            qWarning() << "OpenGL error at" << context << ":" << glErrorString(error) << "(" << error << ")";
        } else {
            qWarning() << "OpenGL error:" << glErrorString(error) << "(" << error << ")";
        }
        return true;
    }
    return false;
}

// Clear any pending GL errors (call before a sequence of operations)
inline void clearGLErrors() {
    auto* ctx = QOpenGLContext::currentContext();
    if (!ctx) return;

    auto* funcs = ctx->functions();
    while (funcs->glGetError() != GL_NO_ERROR) {
        // drain all pending errors
    }
}

} // namespace GLUtils

// Convenience macro for checking GL errors with file/line info
#define GL_CHECK_ERROR() \
    GLUtils::checkGLError(__FILE__ ":" QT_STRINGIFY(__LINE__))

// Macro to wrap a GL call and check for errors (debug builds only)
#ifdef QT_DEBUG
#define GL_CALL(call) \
    do { \
        GLUtils::clearGLErrors(); \
        call; \
        GLUtils::checkGLError(#call); \
    } while(0)
#else
#define GL_CALL(call) call
#endif
