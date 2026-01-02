// ---- TextureBlitWidget.cpp ----

#include "TextureBlitWidget.h"
#include "RadarGLWidget.h"
#include "FBORenderer.h"
#include "CameraController.h"
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>

namespace {

// Simple fullscreen quad vertex shader
const char* kBlitVertexShader = R"(
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Simple texture sampling fragment shader
const char* kBlitFragmentShader = R"(
#version 450 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
    FragColor = texture(uTexture, TexCoord);
}
)";

// Fullscreen quad vertices: position (x, y), texcoord (u, v)
const float kQuadVertices[] = {
    // Position    // TexCoord
    -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
    -1.0f, -1.0f,  0.0f, 0.0f,  // Bottom-left
     1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right

    -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
     1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
     1.0f,  1.0f,  1.0f, 1.0f   // Top-right
};

} // anonymous namespace

TextureBlitWidget::TextureBlitWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
}

TextureBlitWidget::~TextureBlitWidget()
{
    makeCurrent();
    cleanupGL();
    doneCurrent();
}

void TextureBlitWidget::setSourceWidget(RadarGLWidget* source)
{
    if (sourceWidget_ == source) {
        return;
    }

    // Disconnect from old source
    if (sourceWidget_ && sourceWidget_->getFBORenderer()) {
        disconnect(sourceWidget_->getFBORenderer(), &FBORenderer::textureUpdated,
                   this, QOverload<>::of(&QWidget::update));
    }

    sourceWidget_ = source;

    // Request FBO resize to match our current size for sharp rendering
    if (sourceWidget_ && width() > 0 && height() > 0) {
        qreal dpr = devicePixelRatioF();
        int scaledW = static_cast<int>(width() * dpr);
        int scaledH = static_cast<int>(height() * dpr);
        sourceWidget_->requestFBOResize(scaledW, scaledH);
    }

    // Connect to new source's FBO updates
    if (sourceWidget_ && sourceWidget_->getFBORenderer()) {
        connect(sourceWidget_->getFBORenderer(), &FBORenderer::textureUpdated,
                this, QOverload<>::of(&QWidget::update));
    }

    update();
}

void TextureBlitWidget::initializeGL()
{
    if (!initializeOpenGLFunctions()) {
        qWarning() << "TextureBlitWidget: Failed to initialize OpenGL functions";
        return;
    }

    // Create shader program
    blitShader_ = std::make_unique<QOpenGLShaderProgram>();
    if (!blitShader_->addShaderFromSourceCode(QOpenGLShader::Vertex, kBlitVertexShader)) {
        qWarning() << "TextureBlitWidget: Vertex shader error:" << blitShader_->log();
        return;
    }
    if (!blitShader_->addShaderFromSourceCode(QOpenGLShader::Fragment, kBlitFragmentShader)) {
        qWarning() << "TextureBlitWidget: Fragment shader error:" << blitShader_->log();
        return;
    }
    if (!blitShader_->link()) {
        qWarning() << "TextureBlitWidget: Shader link error:" << blitShader_->log();
        return;
    }

    createBlitGeometry();
    initialized_ = true;

    qDebug() << "TextureBlitWidget initialized";
}

void TextureBlitWidget::createBlitGeometry()
{
    // Create VAO
    glGenVertexArrays(1, &blitVao_);
    glBindVertexArray(blitVao_);

    // Create VBO
    glGenBuffers(1, &blitVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, blitVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void TextureBlitWidget::paintGL()
{
    if (!initialized_ || !sourceWidget_) {
        // Clear to dark gray if no source
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    FBORenderer* fbo = sourceWidget_->getFBORenderer();
    if (!fbo || !fbo->isValid()) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    // Clear and disable depth test for 2D blit
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // Bind shader and texture
    blitShader_->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo->getTexture());
    blitShader_->setUniformValue("uTexture", 0);

    // Draw fullscreen quad
    glBindVertexArray(blitVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    blitShader_->release();
}

void TextureBlitWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    // Request FBO resize to match our window size for sharp rendering
    if (sourceWidget_) {
        // Account for device pixel ratio on high-DPI displays
        qreal dpr = devicePixelRatioF();
        int scaledW = static_cast<int>(w * dpr);
        int scaledH = static_cast<int>(h * dpr);
        sourceWidget_->requestFBOResize(scaledW, scaledH);
    }
}

void TextureBlitWidget::cleanupGL()
{
    if (blitVao_ != 0) {
        glDeleteVertexArrays(1, &blitVao_);
        blitVao_ = 0;
    }
    if (blitVbo_ != 0) {
        glDeleteBuffers(1, &blitVbo_);
        blitVbo_ = 0;
    }
    blitShader_.reset();
    initialized_ = false;
}

// Mouse event forwarding to source widget's camera controller

void TextureBlitWidget::mousePressEvent(QMouseEvent* event)
{
    if (sourceWidget_) {
        if (CameraController* camera = sourceWidget_->getCameraController()) {
            camera->mousePressEvent(event);
        }
    }
}

void TextureBlitWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (sourceWidget_) {
        if (CameraController* camera = sourceWidget_->getCameraController()) {
            camera->mouseMoveEvent(event);
        }
    }
}

void TextureBlitWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (sourceWidget_) {
        if (CameraController* camera = sourceWidget_->getCameraController()) {
            camera->mouseReleaseEvent(event);
        }
    }
}

void TextureBlitWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Shift+double-click closes the pop-out window
    if (event->modifiers() & Qt::ShiftModifier) {
        emit closeRequested();
        return;
    }

    // Otherwise forward to camera controller (resets view)
    if (sourceWidget_) {
        if (CameraController* camera = sourceWidget_->getCameraController()) {
            camera->mouseDoubleClickEvent(event);
        }
    }
}

void TextureBlitWidget::wheelEvent(QWheelEvent* event)
{
    if (sourceWidget_) {
        if (CameraController* camera = sourceWidget_->getCameraController()) {
            camera->wheelEvent(event);
        }
    }
}
