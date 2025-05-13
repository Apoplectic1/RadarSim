// RadarSceneWidget.h
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QOpenGLContext>
#include "SphereWidget.h"
#include "SphereRenderer.h"
#include "BeamController.h"
#include "CameraController.h"
#include "ModelManager/ModelManager.h"

class RadarSceneWidget : public QWidget {
    Q_OBJECT

public:
    explicit RadarSceneWidget(QWidget* parent = nullptr);
    ~RadarSceneWidget();

    // Radar position control
    void setRadius(float radius);
    void setAngles(float theta, float phi);
    float getTheta() const;
    float getPhi() const;

    // Beam control (forwarded to SphereWidget for now)
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

    // New methods for components
    SphereRenderer* getSphereRenderer() const { return sphereRenderer_; }
    BeamController* getBeamController() const { return beamController_; }
    CameraController* getCameraController() const { return cameraController_; }
    ModelManager* getModelManager() const { return modelManager_; }

signals:
    void radarPositionChanged(float radius, float theta, float phi);
    void beamTypeChanged(BeamType type);
    void beamWidthChanged(float width);
    void visibilityOptionChanged(const QString& option, bool visible);

private:
    // During transition, maintain the SphereWidget instance
    SphereWidget* sphereWidget_;

    // Layout for this widget
    QVBoxLayout* layout_;

    // Component handles (initially nullptr)
    SphereRenderer* sphereRenderer_ = nullptr;
    BeamController* beamController_ = nullptr;
    CameraController* cameraController_ = nullptr;
    ModelManager* modelManager_ = nullptr;

    // Create and initialize components
    void initializeComponents();
};