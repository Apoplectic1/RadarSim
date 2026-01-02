// RadarSiteRenderer.h - Renders the radar site dot on the sphere
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

class RadarSiteRenderer : protected QOpenGLFunctions_4_5_Core {
public:
    RadarSiteRenderer();
    ~RadarSiteRenderer();

    // Initialization and cleanup
    bool initialize();
    void cleanup();

    // Rendering
    void render(const QMatrix4x4& projection, const QMatrix4x4& view,
                const QMatrix4x4& model, float radius);

    // Position on sphere (spherical coordinates)
    void setPosition(float theta, float phi);
    float getTheta() const { return theta_; }
    float getPhi() const { return phi_; }
    QVector3D getCartesianPosition(float radius) const;

    // Appearance
    void setColor(const QVector3D& color);
    const QVector3D& getColor() const { return color_; }

private:
    bool initialized_ = false;

    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> shaderProgram_;
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vbo_;
    std::vector<float> vertices_;

    // Position (spherical coordinates)
    float theta_ = 45.0f;  // Longitude in degrees
    float phi_ = 45.0f;    // Latitude in degrees

    // Appearance
    QVector3D color_{1.0f, 0.0f, 0.0f};  // Red default

    // Shader sources
    std::string_view vertexShaderSource_;
    std::string_view fragmentShaderSource_;

    // Helper methods
    void createDotGeometry();
    QVector3D sphericalToCartesian(float r, float thetaDeg, float phiDeg) const;
};
