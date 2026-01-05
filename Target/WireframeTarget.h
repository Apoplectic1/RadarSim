// WireframeTarget.h
#pragma once

#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <vector>
#include <memory>
#include <string_view>

#include "WireframeShapes.h"

// Edge structure for rendering and physics (edge diffraction)
struct GeometricEdge {
    uint32_t v0, v1;          // Vertex indices
    float creaseAngle;        // Angle between adjacent faces (radians)
    bool isCrease;            // true if angle > threshold (should be rendered)
};

class WireframeTarget : protected QOpenGLFunctions_4_5_Core {
public:
    WireframeTarget();
    virtual ~WireframeTarget();

    // Clean up OpenGL resources (call before context is destroyed)
    virtual void cleanup();

    // Core lifecycle
    virtual void initialize();
    virtual void render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& sceneModel);
    void uploadGeometryToGPU();

    // Type identification
    virtual WireframeType getType() const = 0;

    // Transform setters
    void setPosition(const QVector3D& position);
    void setPosition(float x, float y, float z);
    void setRotation(const QVector3D& eulerAngles);  // pitch, yaw, roll in degrees
    void setRotation(float pitch, float yaw, float roll);
    void setRotation(const QQuaternion& rotation);
    void setScale(float uniformScale);
    void setScale(const QVector3D& scale);

    // Transform getters
    QVector3D getPosition() const { return position_; }
    QVector3D getRotationEuler() const;
    QQuaternion getRotation() const { return rotation_; }
    QVector3D getScale() const { return scale_; }

    // Appearance
    void setColor(const QVector3D& color);
    void setVisible(bool visible);

    QVector3D getColor() const { return color_; }
    bool isVisible() const { return visible_; }

    // Radar position for angle-based edge shading
    void setRadarPosition(const QVector3D& pos) { radarPos_ = pos; }

    // Geometry accessors (for RCS ray tracing)
    const std::vector<float>& getVertices() const { return vertices_; }
    const std::vector<GLuint>& getIndices() const { return indices_; }
    QMatrix4x4 getModelMatrix() const { return buildModelMatrix(); }

    // Factory method
    static std::unique_ptr<WireframeTarget> createTarget(WireframeType type);

protected:
    // OpenGL resources for target geometry
    std::unique_ptr<QOpenGLShaderProgram> shaderProgram_;
    QOpenGLVertexArrayObject vao_;
    GLuint vboId_ = 0;
    GLuint eboId_ = 0;

    // Geometry data - vertices for GL_TRIANGLES
    // Format: [x, y, z, nx, ny, nz] per vertex (position + surface normal)
    std::vector<float> vertices_;
    std::vector<GLuint> indices_;
    int vertexCount_ = 0;
    int indexCount_ = 0;
    bool geometryDirty_ = false;

    // Edge data for rendering and physics
    std::vector<GeometricEdge> edges_;     // All detected edges with crease info
    std::vector<float> edgeVertices_;      // Edge line vertices for rendering (x,y,z pairs)
    GLuint edgeVboId_ = 0;                 // Separate VBO for edge lines
    int edgeVertexCount_ = 0;

    // Transform state
    QVector3D position_ = QVector3D(0.0f, 0.0f, 0.0f);
    QQuaternion rotation_ = QQuaternion();
    QVector3D scale_ = QVector3D(1.0f, 1.0f, 1.0f);

    // Appearance state
    QVector3D color_ = QVector3D(0.0f, 1.0f, 0.0f);  // Default: green
    bool visible_ = true;

    // Radar position for angle-based shading
    QVector3D radarPos_ = QVector3D(0.0f, 0.0f, 100.0f);

    // Shader sources (string_view for type-safe literals)
    std::string_view vertexShaderSource_;
    std::string_view fragmentShaderSource_;

    // Abstract method for derived classes
    virtual void generateGeometry() = 0;

    // Helper methods
    void setupShaders();
    QMatrix4x4 buildModelMatrix() const;

    // Geometry helpers for solid surface rendering
    void addVertex(const QVector3D& position, const QVector3D& normal);
    void addTriangle(GLuint v0, GLuint v1, GLuint v2);
    void addQuad(GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    GLuint getVertexCount() const { return static_cast<GLuint>(vertices_.size() / 6); }
    void clearGeometry();

    // Edge detection and rendering helpers
    void detectEdges();               // Build edge list from triangles
    void generateEdgeGeometry();      // Build edge line vertices for rendering
    void uploadEdgeGeometry();        // Upload edge lines to GPU
    QVector3D getTriangleNormal(int triIdx) const;  // Compute face normal for edge detection
};
