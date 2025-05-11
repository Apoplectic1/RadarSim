// ---- BeamController.h ----

#pragma once

#include <QObject>
#include <QVector3D>
#include "RadarBeam.h"

class BeamController : public QObject {
    Q_OBJECT

public:
    explicit BeamController(QObject* parent = nullptr);
    ~BeamController();

    // Beam type and properties
    void setBeamType(BeamType type);
    void setBeamWidth(float degrees);
    void setBeamColor(const QVector3D& color);
    void setBeamOpacity(float opacity);
    void setBeamVisible(bool visible);

    // Beam position
    void updateBeamPosition(const QVector3D& position);

    // Access to beam
    RadarBeam* getBeam() const { return radarBeam_; }

private:
    // Placeholder for future implementation
    // Will contain beam management code from SphereWidget
    RadarBeam* radarBeam_ = nullptr;
    bool showBeam_ = true;
};