// BounceRenderer.h - Visualization of ray bounce paths
//
// Renders multi-bounce ray paths with intensity-based coloring.
// Shared by all beam types when bounce visualization is enabled.
//
#pragma once

#include <QObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include <vector>
#include <string_view>

#include "../RCS/RayTraceTypes.h"

// Forward declarations
namespace RCS {
struct HitResult;
}

class BounceRenderer : public QObject, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit BounceRenderer(QObject* parent = nullptr);
    ~BounceRenderer() override;

    // Lifecycle
    bool initialize();
    void cleanup();
    bool isInitialized() const { return initialized_; }

    // Set bounce data for rendering
    // @param radarPos: Starting position of the ray
    // @param bounces: Vector of hit results from multi-bounce trace
    // @param states: Vector of bounce states with intensity info (must match bounces size)
    // @param sphereRadius: Radius for extending final ray to sphere surface
    void setBounceData(const QVector3D& radarPos,
                       const std::vector<RCS::HitResult>& bounces,
                       const std::vector<RCS::BounceState>& states,
                       float sphereRadius);

    // Simplified version using a single state (intensity applied uniformly)
    void setBounceData(const QVector3D& radarPos,
                       const std::vector<RCS::HitResult>& bounces,
                       float sphereRadius);

    // Clear all bounce data
    void clearBounceData();

    // Rendering (cameraPos needed for view-aligned quad generation)
    void render(const QMatrix4x4& projection, const QMatrix4x4& view,
                const QVector3D& cameraPos);

    // Configuration
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    void setBaseColor(const QVector3D& color) { baseColor_ = color; }
    QVector3D getBaseColor() const { return baseColor_; }

    void setRayTraceMode(RCS::RayTraceMode mode) { rayTraceMode_ = mode; }
    RCS::RayTraceMode getRayTraceMode() const { return rayTraceMode_; }

    // Data accessors (for overlay text)
    bool hasHits() const { return !bounceHitPoints_.empty(); }
    int getBounceCount() const { return static_cast<int>(bounceHitPoints_.size()); }
    QVector3D getBounceHitPoint(int index) const;
    float getTotalPathLength() const { return totalPathLength_; }

    // Get color for a specific bounce based on intensity
    QVector3D getBounceColor(int bounceIndex, float intensity) const;

private:
    bool initialized_ = false;
    bool visible_ = false;
    bool geometryDirty_ = false;

    // Configuration
    QVector3D baseColor_;
    RCS::RayTraceMode rayTraceMode_ = RCS::RayTraceMode::PhysicsAccurate;

    // Ray data
    QVector3D radarPos_;
    float sphereRadius_ = 100.0f;

    // Bounce data
    std::vector<QVector3D> bounceHitPoints_;
    std::vector<float> bounceIntensities_;
    float totalPathLength_ = 0.0f;

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

    // Line segment data (stored before quad generation)
    struct LineSegment {
        QVector3D start;
        QVector3D end;
        QVector3D color;
    };
    std::vector<LineSegment> lineSegments_;

    // Helper methods
    void setupShaders();
    void generateGeometry();
    void uploadGeometry();

    // Generate quad geometry from stored line segments (requires camera position)
    void generateQuadGeometry(const QVector3D& cameraPos, float lineWidth);

    // Store line segment for later quad generation
    void addLineSegment(const QVector3D& start, const QVector3D& end,
                        const QVector3D& color);
    void addHitMarker(const QVector3D& position, float size,
                      const QVector3D& color);

    // Ray-sphere intersection for extending final ray
    bool raySphereIntersect(const QVector3D& origin, const QVector3D& dir,
                            float radius, float& t) const;
};
