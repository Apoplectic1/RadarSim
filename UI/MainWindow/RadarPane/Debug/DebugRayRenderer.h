// DebugRayRenderer.h - Visualization of single debug ray with reflection
#pragma once

#include <QObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include <string_view>

#include "RCSTypes.h"

class DebugRayRenderer : public QObject, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit DebugRayRenderer(QObject* parent = nullptr);
    ~DebugRayRenderer() override;

    // Lifecycle
    bool initialize();
    void cleanup();
    bool isInitialized() const { return initialized_; }

    // Update ray data from single RCS hit result
    void setRayData(const QVector3D& radarPos, const RCS::HitResult& hit,
                    float maxDistance);

    // Clear ray data (no hit)
    void clearRayData();

    // Rendering
    void render(const QMatrix4x4& projection, const QMatrix4x4& view);

    // Configuration
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Hit data accessors (for overlay text)
    bool hasHit() const { return hasHit_; }
    QVector3D getHitPoint() const { return hitPoint_; }
    float getHitDistance() const { return hitDistance_; }
    float getReflectionAngle() const { return reflectionAngle_; }
    QVector3D getReflectionDir() const { return reflectionDir_; }

private:
    bool initialized_ = false;
    bool visible_ = false;
    bool geometryDirty_ = false;

    // Ray data
    QVector3D radarPos_;
    QVector3D hitPoint_;
    QVector3D reflectionDir_;
    QVector3D rayDirection_;
    bool hasHit_ = false;
    float hitDistance_ = 0.0f;
    float reflectionAngle_ = 0.0f;  // Angle from surface normal
    float maxDistance_ = 300.0f;

    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> shaderProgram_;
    QOpenGLVertexArrayObject vao_;
    GLuint vboId_ = 0;

    // Geometry data (position + color per vertex)
    std::vector<float> vertices_;
    int vertexCount_ = 0;

    // Shader sources
    std::string_view vertexShaderSource_;
    std::string_view fragmentShaderSource_;

    // Helper methods
    void setupShaders();
    void generateGeometry();
    void uploadGeometry();

    // Generate specific ray segments
    void addLineSegment(const QVector3D& start, const QVector3D& end,
                        const QVector3D& color);
    void addHitMarker(const QVector3D& position, float size,
                      const QVector3D& color);
};
