// SphereRenderer.h
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

    // Initialization
    void initialize();

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

signals:
    void radiusChanged(float radius);
    void visibilityChanged(const QString& element, bool visible);

private:
    // OpenGL objects
    QOpenGLShaderProgram* shaderProgram = nullptr;

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

    // Helper methods
    void createSphere(int latDivisions = 64, int longDivisions = 64);
    void createLatitudeLongitudeLines();
    void createCoordinateAxes();
    void setupShaders();
};