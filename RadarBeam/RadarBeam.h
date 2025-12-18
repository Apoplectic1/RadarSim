// ----RadarBeam.h----

#pragma once

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>

// Forward declarations
class QOpenGLShaderProgram;

// Enum for different beam types
enum class BeamType {
    Conical,       // Standard conical beam
    Shaped,        // Custom shaped beam using a pattern function
    Phased         // Full phased array beam with multiple lobes
};

// Enum for beam direction reference
enum class BeamDirection {
    ToOrigin,      // Beam points toward origin (0,0,0)
    AwayFromOrigin,// Beam points away from origin
    Custom         // Beam points in custom direction
};

// Base class for all beam shapes
class RadarBeam : protected QOpenGLFunctions_3_3_Core {
public:
    // Constructor
    RadarBeam(float sphereRadius = 100.0f, float beamWidthDegrees = 15.0f);

    // Destructor
    virtual ~RadarBeam();

    void uploadGeometryToGPU();

    // Core methods
    virtual void initialize();
    virtual void update(const QVector3D& radarPosition);
    virtual void render(QOpenGLShaderProgram* program, const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);
    virtual void renderDepthOnly(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);

    // Property setters
    virtual BeamType getBeamType() const { return BeamType::Conical; }  // Default

    void setBeamWidth(float degrees);
    void setSphereRadius(float radius);  // Update sphere radius and regenerate geometry
    void setBeamDirection(BeamDirection direction);
    void setCustomDirection(const QVector3D& direction);
    void setColor(const QVector3D& color);
    void setOpacity(float opacity);
    void setVisible(bool visible);
    void setBeamLength(float length);  // Length as fraction of sphere diameter

    // Property getters
    float getBeamWidth() const { return beamWidthDegrees_; }
    float getSphereRadius() const { return sphereRadius_; }
    BeamDirection getBeamDirection() const { return beamDirection_; }
    QVector3D getCustomDirection() const { return customDirection_; }
    QVector3D getColor() const { return color_; }
    float getOpacity() const { return opacity_; }
    bool isVisible() const { return visible_; }
    float getBeamLength() const { return beamLengthFactor_; }

    // Factory method to create different beam types
    static RadarBeam* createBeam(BeamType type, float sphereRadius, float beamWidthDegrees = 15.0f);

protected:
    // OpenGL resources
    QOpenGLShaderProgram* beamShaderProgram = nullptr;
    QOpenGLVertexArrayObject beamVAO;
    GLuint vboId_ = 0;  // Raw OpenGL buffer ID for vertices
    GLuint eboId_ = 0;  // Raw OpenGL buffer ID for indices

    // Keep QOpenGLBuffer for compatibility but mark as deprecated
    QOpenGLBuffer beamVBO;  // deprecated - use vboId_
    QOpenGLBuffer beamEBO;  // deprecated - use eboId_

    // Beam properties
    float sphereRadius_;
    float beamWidthDegrees_;
    QVector3D color_;
    float opacity_;
    bool visible_;
    float beamLengthFactor_; // How far the beam extends (1.0 = to opposite side)
    BeamDirection beamDirection_;
    QVector3D customDirection_;
    QVector3D currentRadarPosition_;

    // Geometry data
    std::vector<float> vertices_;
    std::vector<unsigned int> indices_;
    bool geometryDirty_ = false;  // Flag to defer GPU upload until valid context

    // Shader sources
    const char* beamVertexShaderSource;
    const char* beamFragmentShaderSource;

    // Helper methods
    virtual void createBeamGeometry();
    virtual void setupShaders();
    QVector3D calculateBeamDirection(const QVector3D& radarPosition);
    QVector3D calculateOppositePoint(const QVector3D& radarPosition, const QVector3D& direction);
    void calculateBeamVertices(const QVector3D& apex, const QVector3D& direction, float length, float baseRadius);
};
