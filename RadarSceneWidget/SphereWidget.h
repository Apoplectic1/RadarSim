// ---- SphereWidget.h ----

#pragma once
#include "RadarBeam/RadarBeam.h"
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QQuaternion>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
#include <QMenu>
#include <QAction>

class SphereWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit SphereWidget(QWidget* parent = nullptr);
    ~SphereWidget() override;

    void setRadius(float r);
    float getRadius() const { return radius_; }
    void setAngles(float theta, float phi);
    float getTheta() const { return theta_; }
    float getPhi() const { return phi_; }

    // Beam control methods
    void setBeamWidth(float degrees);
    void setBeamType(BeamType type);
    void setBeamColor(const QVector3D& color);
    void setBeamOpacity(float opacity);
    RadarBeam* getBeam() const { return radarBeam_; }


    void setInertiaParameters(float decay, float velocityScale) {
        rotationDecay_ = qBound(0.8f, decay, 0.99f);
        velocityScale_ = qBound(0.01f, velocityScale, 0.2f);
    }

    void setSphereVisible(bool visible) {
        showSphere_ = visible;
        update();  // Request a redraw
    }

    bool isSphereVisible() const {
        return showSphere_;
    }

    void setAxesVisible(bool visible) {
        showAxes_ = visible;
        update();  // Request a redraw
    }

    bool areAxesVisible() const {
        return showAxes_;
    }

    void setGridLinesVisible(bool visible) {
        showGridLines_ = visible;
        update();  // Request a redraw
    }

    bool areGridLinesVisible() const {
        return showGridLines_;
    }
    
    void setInertiaEnabled(bool enabled) {
        inertiaEnabled_ = enabled;

        // If disabling inertia, stop any ongoing inertia movement
        if (!enabled) {
            stopInertia();
        }
    }

    bool isInertiaEnabled() const {
        return inertiaEnabled_;
    }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    // Sphere properties
    float radius_ = 100.0f;
    float theta_ = 45.0f;
    float phi_ = 45.0f;
    bool showSphere_ = true;   // Track sphere visibility

    // Variables for axes
    QOpenGLVertexArrayObject axesVAO;
    QOpenGLBuffer axesVBO;
    std::vector<float> axesVertices;
    bool showAxes_ = true;  // Track axes visibility

    // OpenGL objects
    QOpenGLShaderProgram* shaderProgram = nullptr;
    QOpenGLShaderProgram* dotShaderProgram = nullptr;
    QOpenGLShaderProgram* axesShaderProgram = nullptr;  // Add this line
    QOpenGLVertexArrayObject sphereVAO;
    QOpenGLBuffer sphereVBO;
    QOpenGLBuffer sphereEBO;

    QOpenGLVertexArrayObject linesVAO;
    QOpenGLBuffer linesVBO;

    QOpenGLVertexArrayObject dotVAO;
    QOpenGLBuffer dotVBO;

    // Geometry data
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    std::vector<float> latLongLines;
    std::vector<float> dotVertices;
    bool showGridLines_ = true;  // Track grid lines visibility

    // Indices for special lines
    int equatorStartIndex = 0;
    int primeMeridianStartIndex = 0;

    // Matrices
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 modelMatrix;

    // Mouse rotation handling
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    QPoint lastMousePos_;
    bool isDragging_ = false;
    QQuaternion rotation_ = QQuaternion(); // Current rotation

    // Mouse wheel zoom handling
    void wheelEvent(QWheelEvent* event) override;

    float zoomFactor_ = 1.0f; // Default zoom (1.0 = no zoom)
    float minZoom_ = 0.5f;    // Minimum zoom (zoomed out)
    float maxZoom_ = 2.0f;    // Maximum zoom (zoomed in)

    // Double-click to reset view
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    // Helper method to reset view
    void resetView();

    // Inertia/momentum for rotation
    QTimer* inertiaTimer_ = nullptr;
    QVector3D rotationAxis_;
    float rotationVelocity_ = 0.0f;
    float rotationDecay_ = 0.99f;  // Increase decay (lower value = faster slowdown)
    float velocityScale_ = 0.15f;  // Add this to scale down initial velocity
    QElapsedTimer frameTimer_; // To measure frame time
    bool inertiaEnabled_ = false; // Track whether inertia is enabled

    // Helper methods for inertia
    void startInertia(QVector3D axis, float velocity);
    void stopInertia();

    // Pan functionality
    bool isPanning_ = false;
    QPoint panStartPos_;
    QVector3D translation_ = QVector3D(0, 0, 0);

    // Context menu
    void contextMenuEvent(QContextMenuEvent* event) override;
    QMenu* contextMenu_ = nullptr;

    // Pan method
    void pan(const QPoint& delta);

    // Helper methods
    void createSphere(int latDivisions = 64, int longDivisions = 64);
    void createLatitudeLongitudeLines();
    void createDot();
    void createCoordinateAxes();
    bool isDotVisible();
    QVector3D sphericalToCartesian(float r, float thetaDeg, float phiDeg);

    private:
        // Radar beam
        RadarBeam* radarBeam_ = nullptr;
        bool showBeam_ = true;

    // Add variables to store counts for debugging
    int latitudeLineCount = 0;
    int longitudeLineCount = 0;
};