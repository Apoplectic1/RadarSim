// BeamController.h
#pragma once

#include <QObject>
#include <QVector3D>
#include "RadarBeam.h"

class BeamController : public QObject {
    Q_OBJECT

public:
    explicit BeamController(QObject* parent = nullptr);
    ~BeamController();

    // Initialization
    void initialize();

    // Rendering
    void render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);

    // Beam management
    void rebuildBeamGeometry();

    void setBeamType(BeamType type);
    BeamType getBeamType() const;

    void setBeamWidth(float degrees);
    float getBeamWidth() const;

    void setBeamColor(const QVector3D& color);
    QVector3D getBeamColor() const;

    void setBeamOpacity(float opacity);
    float getBeamOpacity() const;

    void setBeamVisible(bool visible);
    bool isBeamVisible() const;

    // Beam position
    void updateBeamPosition(const QVector3D& position);

    // Access beam
    RadarBeam* getBeam() const { return radarBeam_; }

signals:
    void beamTypeChanged(BeamType type);
    void beamWidthChanged(float width);
    void beamColorChanged(const QVector3D& color);
    void beamOpacityChanged(float opacity);
    void beamVisibilityChanged(bool visible);

private:
    // Beam properties
    RadarBeam* radarBeam_ = nullptr;
    BeamType currentBeamType_ = BeamType::Conical;
    float sphereRadius_ = 100.0f;
    bool showBeam_ = true;

    // Create the appropriate beam based on current settings
    void createBeam();
};