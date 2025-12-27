// RadarSceneWidget.h - Component-based radar scene container
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QOpenGLContext>
#include "SphereRenderer.h"
#include "BeamController.h"
#include "CameraController.h"
#include "ModelManager/ModelManager.h"
#include "WireframeTargetController.h"
#include "RadarGLWidget.h"

class RadarSceneWidget : public QWidget {
    Q_OBJECT

public:
    explicit RadarSceneWidget(QWidget* parent = nullptr);
    ~RadarSceneWidget();

    // Radar position control
    void setRadius(float radius);
    float getRadius() const;
    void setAngles(float theta, float phi);
    float getTheta() const;
    float getPhi() const;

    // Beam control
    void setBeamWidth(float degrees);
    void setBeamType(BeamType type);
    void setBeamColor(const QVector3D& color);
    void setBeamOpacity(float opacity);
    RadarBeam* getBeam() const;

    // UI state control
    void setSphereVisible(bool visible);
    void setAxesVisible(bool visible);
    void setGridLinesVisible(bool visible);
    void setInertiaEnabled(bool enabled);

    // Visibility query methods
    bool isSphereVisible() const;
    bool areAxesVisible() const;
    bool areGridLinesVisible() const;
    bool isInertiaEnabled() const;

    // Component access
    SphereRenderer* getSphereRenderer() const { return sphereRenderer_; }
    BeamController* getBeamController() const { return beamController_; }
    CameraController* getCameraController() const { return cameraController_; }
    ModelManager* getModelManager() const { return modelManager_; }
    WireframeTargetController* getWireframeController() const { return wireframeController_; }

    // Force update of the OpenGL scene
    void updateScene();

signals:
    void radarPositionChanged(float radius, float theta, float phi);
    void beamTypeChanged(BeamType type);
    void beamWidthChanged(float width);
    void visibilityOptionChanged(const QString& option, bool visible);

private slots:
    void onRadiusChanged(float radius);
    void onAnglesChanged(float theta, float phi);

private:
    void createComponents();

    RadarGLWidget* radarGLWidget_ = nullptr;
    QVBoxLayout* layout_ = nullptr;

    // Component handles
    SphereRenderer* sphereRenderer_ = nullptr;
    BeamController* beamController_ = nullptr;
    CameraController* cameraController_ = nullptr;
    ModelManager* modelManager_ = nullptr;
    WireframeTargetController* wireframeController_ = nullptr;
};
