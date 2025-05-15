// ---- ConicalBeam.h ----

#pragma once

#include "RadarBeam.h"

// Derived class for conical beam (simplest form)
class ConicalBeam : public RadarBeam {
public:
    BeamType getBeamType() const override { return BeamType::Conical; }
    ConicalBeam(float sphereRadius = 100.0f, float beamWidthDegrees = 15.0f);
    virtual ~ConicalBeam();

protected:
    virtual void createBeamGeometry() override;
};
