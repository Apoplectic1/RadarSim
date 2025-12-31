// SlicingPlaneRenderer.h - Visualizes the RCS slicing plane in 3D scene
#pragma once

#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include "RCSSampler.h"  // For CutType enum

class SlicingPlaneRenderer : protected QOpenGLFunctions_4_5_Core {
public:
    explicit SlicingPlaneRenderer(QObject* parent = nullptr);
    ~SlicingPlaneRenderer();

    bool initialize();
    void cleanup();

    // Configure the plane
    void setCutType(CutType type);
    void setOffset(float degrees);        // Z offset for azimuth, azimuth angle for elevation
    void setThickness(float degrees);     // Visual thickness indicator
    void setSphereRadius(float radius);   // Size plane to match sphere

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Show filled volume or just outlines
    void setShowFill(bool show) { showFill_ = show; }
    bool isShowFill() const { return showFill_; }

    // Render the translucent plane
    void render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);

private:
    void createShaders();
    void createGeometry();
    void updateGeometry();  // Rebuild geometry when parameters change

    // OpenGL resources - plane fill
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    // OpenGL resources - thickness outline lines
    GLuint outlineVao_ = 0;
    GLuint outlineVbo_ = 0;
    int outlineVertexCount_ = 0;

    std::unique_ptr<QOpenGLShaderProgram> shaderProgram_;
    bool initialized_ = false;
    bool geometryDirty_ = true;

    // Plane parameters
    CutType cutType_ = CutType::Azimuth;
    float offset_ = 0.0f;           // Degrees
    float thickness_ = 5.0f;        // Degrees (visual indication)
    float sphereRadius_ = 100.0f;

    // Visibility
    bool visible_ = true;
    bool showFill_ = true;  // Show filled volume (false = outlines only)

    // Geometry data
    std::vector<float> vertices_;
    int vertexCount_ = 0;
};
