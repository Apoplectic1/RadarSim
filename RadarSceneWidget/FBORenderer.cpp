// ---- FBORenderer.cpp ----

#include "FBORenderer.h"
#include <QOpenGLContext>
#include <QDebug>

FBORenderer::FBORenderer(QObject* parent)
    : QObject(parent)
{
}

FBORenderer::~FBORenderer()
{
    cleanup();
}

bool FBORenderer::initialize(int width, int height)
{
    if (initialized_) {
        qWarning() << "FBORenderer::initialize() - Already initialized";
        return true;
    }

    if (!QOpenGLContext::currentContext()) {
        qWarning() << "FBORenderer::initialize() - No current OpenGL context";
        return false;
    }

    if (!glFunctionsInitialized_) {
        if (!initializeOpenGLFunctions()) {
            qWarning() << "FBORenderer::initialize() - Failed to initialize OpenGL functions";
            return false;
        }
        glFunctionsInitialized_ = true;
    }

    width_ = width;
    height_ = height;

    // Create framebuffer object
    glGenFramebuffers(1, &fbo_);
    if (fbo_ == 0) {
        qWarning() << "FBORenderer::initialize() - Failed to create FBO";
        return false;
    }

    // Create attachments (texture and depth buffer)
    createAttachments();

    // Check framebuffer completeness
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        qWarning() << "FBORenderer::initialize() - Framebuffer is not complete, status:" << status;
        cleanup();
        return false;
    }

    initialized_ = true;
    qDebug() << "FBORenderer initialized:" << width_ << "x" << height_;
    return true;
}

void FBORenderer::resize(int width, int height)
{
    if (width == width_ && height == height_) {
        return;
    }

    if (!initialized_ || !QOpenGLContext::currentContext()) {
        width_ = width;
        height_ = height;
        return;
    }

    width_ = width;
    height_ = height;

    // Recreate attachments with new size
    deleteAttachments();
    createAttachments();

    // Verify framebuffer is still complete
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        qWarning() << "FBORenderer::resize() - Framebuffer is not complete after resize";
    }
}

void FBORenderer::bind()
{
    if (!initialized_ || fbo_ == 0) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
}

void FBORenderer::release()
{
    if (!initialized_) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    emit textureUpdated();
}

void FBORenderer::cleanup()
{
    if (!glFunctionsInitialized_ || !QOpenGLContext::currentContext()) {
        // Can't clean up GL resources without context
        fbo_ = 0;
        colorTexture_ = 0;
        depthRbo_ = 0;
        initialized_ = false;
        return;
    }

    deleteAttachments();

    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }

    initialized_ = false;
}

void FBORenderer::createAttachments()
{
    if (width_ <= 0 || height_ <= 0) {
        return;
    }

    // Create color texture
    glGenTextures(1, &colorTexture_);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create depth renderbuffer
    glGenRenderbuffers(1, &depthRbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Attach to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBORenderer::deleteAttachments()
{
    if (colorTexture_ != 0) {
        glDeleteTextures(1, &colorTexture_);
        colorTexture_ = 0;
    }

    if (depthRbo_ != 0) {
        glDeleteRenderbuffers(1, &depthRbo_);
        depthRbo_ = 0;
    }
}
