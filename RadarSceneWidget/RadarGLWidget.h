// RadarGLWidget.h
#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QMenu>
#include <memory>

#include "RadarSceneWidget/SphereRenderer.h"
#include "RadarBeam/BeamController.h"
#include "RadarSceneWidget/CameraController.h"
#include "ModelManager/ModelManager.h"
#include "WireframeTargetController.h"
#include "RCSCompute/RCSCompute.h"

class RadarGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit RadarGLWidget(QWidget* parent = nullptr);
    ~RadarGLWidget() override;

    // Initialization
    void initialize(SphereRenderer* sphereRenderer,
        BeamController* beamController,
        CameraController* cameraController,
        ModelManager* modelManager,
        WireframeTargetController* wireframeController);

    // Radar position
    void setRadius(float radius);
    void setAngles(float theta, float phi);
    float getRadius() const { return radius_; }
    float getTheta() const { return theta_; }
    float getPhi() const { return phi_; }

    // Event handlers - override from QOpenGLWidget
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void radiusChanged(float radius);
    void anglesChanged(float theta, float phi);

protected:
    // Override OpenGL functions
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private slots:
    // Clean up OpenGL resources before context is destroyed
    void cleanupGL();

private:
    // Radar position
    float radius_ = 100.0f;
    float theta_ = 45.0f;
    float phi_ = 45.0f;
    bool beamDirty_ = true;
    bool glCleanedUp_ = false;  // Prevent double cleanup

    // Component references (owned by RadarSceneWidget)
    SphereRenderer* sphereRenderer_ = nullptr;
    BeamController* beamController_ = nullptr;
    CameraController* cameraController_ = nullptr;
    ModelManager* modelManager_ = nullptr;
    WireframeTargetController* wireframeController_ = nullptr;

    // RCS computation (owned by this widget)
    std::unique_ptr<RCS::RCSCompute> rcsCompute_;

    // Context menu
    QMenu* contextMenu_ = nullptr;

    // Helper methods
    QVector3D sphericalToCartesian(float r, float thetaDeg, float phiDeg);
    void setupContextMenu();
    void updateBeamPosition();
    QPointF projectToScreen(const QVector3D& worldPos, const QMatrix4x4& projection,
                            const QMatrix4x4& view, const QMatrix4x4& model);
};