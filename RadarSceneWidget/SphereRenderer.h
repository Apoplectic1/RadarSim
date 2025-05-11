// ---- SphereRenderer.h ----

#pragma once

#include <QObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>

class SphereRenderer : public QObject, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit SphereRenderer(QObject* parent = nullptr);
    ~SphereRenderer();

    // Initialization and core rendering
    void initialize();
    void render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);

    // Geometry settings
    void setRadius(float radius);
    float getRadius() const { return radius_; }

    // Visibility settings
    void setSphereVisible(bool visible);
    void setGridLinesVisible(bool visible);
    void setAxesVisible(bool visible);

    bool isSphereVisible() const { return showSphere_; }
    bool areGridLinesVisible() const { return showGridLines_; }
    bool areAxesVisible() const { return showAxes_; }

private:
    // Placeholder for future implementation
    // Will contain sphere, grid, and axes rendering code from SphereWidget
    float radius_ = 100.0f;
    bool showSphere_ = true;
    bool showGridLines_ = true;
    bool showAxes_ = true;
};