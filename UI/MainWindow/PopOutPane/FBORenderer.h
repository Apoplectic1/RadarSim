// ---- FBORenderer.h ----
// Framebuffer Object wrapper for offscreen rendering
// Used to render the scene to a texture that can be displayed in a pop-out window

#pragma once

#include <QObject>
#include <QOpenGLFunctions_4_5_Core>

class FBORenderer : public QObject, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
public:
    explicit FBORenderer(QObject* parent = nullptr);
    ~FBORenderer();

    // Initialize FBO with given dimensions (must be called with valid GL context)
    bool initialize(int width, int height);

    // Resize FBO attachments (recreates texture and renderbuffer)
    void resize(int width, int height);

    // Bind FBO for rendering (call before rendering scene)
    void bind();

    // Release FBO and emit textureUpdated signal (call after rendering scene)
    void release();

    // Cleanup GL resources (call with valid GL context)
    void cleanup();

    // Get the color texture ID for display in another widget
    GLuint getTexture() const { return colorTexture_; }

    // Get FBO dimensions
    int width() const { return width_; }
    int height() const { return height_; }

    // Check if FBO is initialized and valid
    bool isValid() const { return initialized_ && fbo_ != 0; }

signals:
    // Emitted after release() to notify that the texture has been updated
    void textureUpdated();

private:
    void createAttachments();
    void deleteAttachments();

    GLuint fbo_ = 0;
    GLuint colorTexture_ = 0;
    GLuint depthRbo_ = 0;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
    bool glFunctionsInitialized_ = false;
};
