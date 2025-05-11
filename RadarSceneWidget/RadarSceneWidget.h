// ---- RadarSceneWidget.h ----

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "SphereWidget.h"

// Forward declarations
class SphereRenderer;
class BeamController;
class CameraController;
class ModelManager;

class RadarSceneWidget : public QWidget {
    Q_OBJECT

public:
    explicit RadarSceneWidget(QWidget* parent = nullptr);
    ~RadarSceneWidget();

    // Access to the underlying SphereWidget during transition
    SphereWidget* getSphereWidget() const { return sphereWidget_; }

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

signals:
    // Signals that will be used for component communication
    void radarPositionChanged(float radius, float theta, float phi);
    void beamTypeChanged(BeamType type);
    void beamWidthChanged(float width);
    void visibilityOptionChanged(const QString& option, bool visible);

private:
    // Eventually, these will become the component classes
    // For now, we'll just keep the SphereWidget instance
    SphereWidget* sphereWidget_;

    // Layout for this widget
    QVBoxLayout* layout_;

    // Future component handles (initially nullptr)
    SphereRenderer* sphereRenderer_ = nullptr;
    BeamController* beamController_ = nullptr;
    CameraController* cameraController_ = nullptr;
    ModelManager* modelManager_ = nullptr;
};
