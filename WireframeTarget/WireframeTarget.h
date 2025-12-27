// WireframeTarget.h
#pragma once

#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <vector>

#include "WireframeShapes.h"

class WireframeTarget : protected QOpenGLFunctions_4_3_Core {
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

    // Geometry accessors (for RCS ray tracing)
    const std::vector<float>& getVertices() const { return vertices_; }
    const std::vector<GLuint>& getIndices() const { return indices_; }
    QMatrix4x4 getModelMatrix() const { return buildModelMatrix(); }

    // Factory method
    static WireframeTarget* createTarget(WireframeType type);

protected:
    // OpenGL resources for target geometry
    QOpenGLShaderProgram* shaderProgram_ = nullptr;
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

    // Transform state
    QVector3D position_ = QVector3D(0.0f, 0.0f, 0.0f);
    QQuaternion rotation_ = QQuaternion();
    QVector3D scale_ = QVector3D(1.0f, 1.0f, 1.0f);

    // Appearance state
    QVector3D color_ = QVector3D(0.0f, 1.0f, 0.0f);  // Default: green
    bool visible_ = true;

    // Shader sources
    const char* vertexShaderSource_;
    const char* fragmentShaderSource_;

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
};
