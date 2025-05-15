// ---- EllipticalBeam.h ----

#pragma once

#include "RadarBeam.h"

// Derived class for elliptical beam (different horizontal/vertical widths)
class EllipticalBeam : public RadarBeam {
public:
    BeamType getBeamType() const override { return BeamType::Elliptical; }
    EllipticalBeam(float sphereRadius = 100.0f, float horizontalWidthDegrees = 20.0f, float verticalWidthDegrees = 10.0f);
    virtual ~EllipticalBeam();

    void setHorizontalWidth(float degrees);
    void setVerticalWidth(float degrees);

    float getHorizontalWidth() const { return horizontalWidthDegrees_; }
    float getVerticalWidth() const { return verticalWidthDegrees_; }

protected:
    float horizontalWidthDegrees_;
    float verticalWidthDegrees_;

    virtual void createBeamGeometry() override;
};
