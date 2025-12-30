// ----RadarBeam.h----

#pragma once

#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <memory>
#include <string_view>

// Enum for different beam types
enum class BeamType {
    Conical,       // Standard conical beam (uniform intensity)
    Shaped,        // Custom shaped beam using a pattern function
    Phased,        // Full phased array beam with multiple lobes
    Sinc           // Sinc² pattern with intensity falloff and side lobes
};

// Enum for beam direction reference
enum class BeamDirection {
    ToOrigin,      // Beam points toward origin (0,0,0)
    AwayFromOrigin,// Beam points away from origin
    Custom         // Beam points in custom direction
};

// Base class for all beam shapes
class RadarBeam : protected QOpenGLFunctions_4_5_Core {
public:
    // Constructor
    RadarBeam(float sphereRadius = 100.0f, float beamWidthDegrees = 15.0f);

    // Destructor
    virtual ~RadarBeam();

    // Clean up OpenGL resources (call before context is destroyed)
    virtual void cleanup();

    virtual void uploadGeometryToGPU();

    // Core methods
    virtual void initialize();
    virtual void update(const QVector3D& radarPosition);
    virtual void render(QOpenGLShaderProgram* program, const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);

    // Property setters
    virtual BeamType getBeamType() const { return BeamType::Conical; }  // Default

    void setBeamWidth(float degrees);
    void setSphereRadius(float radius);  // Update sphere radius and regenerate geometry
    void setBeamDirection(BeamDirection direction);
    void setCustomDirection(const QVector3D& direction);
    void setColor(const QVector3D& color);
    void setOpacity(float opacity);
    void setVisible(bool visible);
    void setFootprintOnly(bool footprintOnly);
    void setBeamLength(float length);  // Length as fraction of sphere diameter

    // GPU shadow map (from RCS compute)
    void setGPUShadowMap(GLuint textureId);
    void setGPUShadowEnabled(bool enabled);
    void setBeamAxis(const QVector3D& axis);
    void setBeamWidthRadians(float radians);
    void setNumRings(int numRings);

    // Property getters
    float getBeamWidth() const { return beamWidthDegrees_; }
    float getSphereRadius() const { return sphereRadius_; }
    BeamDirection getBeamDirection() const { return beamDirection_; }
    QVector3D getCustomDirection() const { return customDirection_; }
    QVector3D getColor() const { return color_; }
    float getOpacity() const { return opacity_; }
    bool isVisible() const { return visible_; }
    bool isFootprintOnly() const { return footprintOnly_; }
    float getBeamLength() const { return beamLengthFactor_; }
    const std::vector<float>& getVertices() const { return vertices_; }

    // Factory method to create different beam types
    static RadarBeam* createBeam(BeamType type, float sphereRadius, float beamWidthDegrees = 15.0f);

protected:
    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> beamShaderProgram_;
    QOpenGLVertexArrayObject beamVAO_;
    GLuint vboId_ = 0;  // Raw OpenGL buffer ID for vertices
    GLuint eboId_ = 0;  // Raw OpenGL buffer ID for indices

    // Beam properties
    float sphereRadius_;
    float beamWidthDegrees_;
    QVector3D color_;
    float opacity_;
    bool visible_;
    bool footprintOnly_ = false;  // Show only sphere surface intersection
    float beamLengthFactor_; // How far the beam extends (1.0 = to opposite side)
    BeamDirection beamDirection_;
    QVector3D customDirection_;
    QVector3D currentRadarPosition_;

    // GPU shadow map for ray-traced shadows
    GLuint gpuShadowMapTexture_ = 0;
    bool gpuShadowEnabled_ = false;
    QVector3D beamAxis_;
    float beamWidthRadians_ = 0.2618f;  // ~15 degrees default
    int numRings_ = 157;  // Default for 10000 rays / 64 per ring

    // Geometry data
    std::vector<float> vertices_;
    std::vector<unsigned int> indices_;
    bool geometryDirty_ = false;  // Flag to defer GPU upload until valid context

    // Shader sources (string_view for type-safe literals)
    std::string_view beamVertexShaderSource_;
    std::string_view beamFragmentShaderSource_;

    // Visibility constants (set by derived classes, used by shader)
    float visFresnelBase_ = 0.1f;
    float visFresnelRange_ = 0.2f;
    float visRimLow_ = 0.6f;
    float visRimHigh_ = 0.95f;
    float visRimStrength_ = 0.1f;
    float visAlphaMin_ = 0.03f;
    float visAlphaMax_ = 0.6f;

    // Helper methods
    virtual void createBeamGeometry();
    virtual void setupShaders();
    QVector3D calculateBeamDirection(const QVector3D& radarPosition);
    QVector3D calculateOppositePoint(const QVector3D& radarPosition, const QVector3D& direction);
    void calculateBeamVertices(const QVector3D& apex, const QVector3D& direction, float length, float baseRadius);
};
