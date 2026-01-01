// ---- TextureBlitWidget.h ----
// Lightweight widget that displays a texture from another RadarGLWidget's FBO
// Forwards mouse events to the source widget's CameraController for synchronized interaction

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <memory>

class RadarGLWidget;
class QOpenGLShaderProgram;

class TextureBlitWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT
public:
    explicit TextureBlitWidget(QWidget* parent = nullptr);
    ~TextureBlitWidget();

    // Set the source widget whose FBO texture we'll display
    void setSourceWidget(RadarGLWidget* source);

    // Get the source widget
    RadarGLWidget* getSourceWidget() const { return sourceWidget_; }

signals:
    // Emitted when the widget needs to close (e.g., Shift+double-click)
    void closeRequested();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    // Forward mouse events to source widget's camera controller
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void createBlitGeometry();
    void cleanupGL();

    RadarGLWidget* sourceWidget_ = nullptr;
    GLuint blitVao_ = 0;
    GLuint blitVbo_ = 0;
    std::unique_ptr<QOpenGLShaderProgram> blitShader_;
    bool initialized_ = false;
};
