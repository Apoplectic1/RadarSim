// SphereRenderer.h
#pragma once

#include <QObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include <vector>
#include <QQuaternion>

class SphereRenderer : public QObject, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit SphereRenderer(QObject* parent = nullptr);
    ~SphereRenderer();

    // Initialization
    bool initialize();
    bool initializeShaders();

    // Rendering
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

    // Radar dot methods
    void setRadarPosition(float theta, float phi);
    QVector3D getRadarPosition() const;
    float getTheta() const { return theta_; }
    float getPhi() const { return phi_; }

    // Inertia control
    void setInertiaEnabled(bool enabled);
    bool isInertiaEnabled() const { return inertiaEnabled_; }
    void setInertiaParameters(float decay, float velocityScale);
    void applyRotation(const QVector3D& axis, float angle, bool withInertia = true);
    void resetView();
    const QQuaternion& getRotation() const { return rotation_; }
    void setRotation(const QQuaternion& rotation);

signals:
    void radiusChanged(float radius);
	void visibilityChanged(const QString& element, bool visible); // Dummy function for signal to silence warnings

private:
    bool initialized_ = false;  // Track initialization state

    // OpenGL objects
    QOpenGLShaderProgram* shaderProgram = nullptr;
    QOpenGLShaderProgram* axesShaderProgram = nullptr;

    // Sphere geometry
    QOpenGLVertexArrayObject sphereVAO;
    QOpenGLBuffer sphereVBO;
    QOpenGLBuffer sphereEBO;
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;

    // Grid lines geometry
    QOpenGLVertexArrayObject linesVAO;
    QOpenGLBuffer linesVBO;
    std::vector<float> latLongLines;
    int equatorStartIndex = 0;
    int primeMeridianStartIndex = 0;
    int latitudeLineCount = 0;
    int longitudeLineCount = 0;

    // Axes geometry
    QOpenGLVertexArrayObject axesVAO;
    QOpenGLBuffer axesVBO;
    std::vector<float> axesVertices;

    // Properties
    float radius_ = 100.0f;
    bool showSphere_ = true;
    bool showGridLines_ = true;
    bool showAxes_ = true;

    // Shader sources
    const char* vertexShaderSource;
    const char* fragmentShaderSource;
    const char* axesVertexShaderSource;
    const char* axesFragmentShaderSource;
    const char* dotVertexShaderSource;
    const char* dotFragmentShaderSource;

    // Helper methods
    void createSphere(int latDivisions = 64, int longDivisions = 64);
    void createGridLines();
    void createAxesLines();

    // Radar dot geometry and properties
    QOpenGLVertexArrayObject dotVAO;
    QOpenGLBuffer dotVBO;
    std::vector<float> dotVertices;
    QOpenGLShaderProgram* dotShaderProgram = nullptr;
    float theta_ = 45.0f;  // Spherical coordinate theta (longitude)
    float phi_ = 45.0f;    // Spherical coordinate phi (latitude)

    // Helper methods
    void createDot();
    QVector3D sphericalToCartesian(float r, float thetaDeg, float phiDeg) const;

    // Inertia control
    // Inertia/momentum for rotation
    QTimer* inertiaTimer_ = nullptr;
    QVector3D rotationAxis_;
    float rotationVelocity_ = 0.0f;
    float rotationDecay_ = 0.95f;  // Determines how quickly inertia slows down (0.9-0.99)
    QElapsedTimer frameTimer_;  // Add this line
    bool inertiaEnabled_ = true;
    QQuaternion rotation_ = QQuaternion(); // Current rotation

    // Helper methods for inertia
    void startInertia(const QVector3D& axis, float velocity);
    void stopInertia();
    void updateInertia();
};