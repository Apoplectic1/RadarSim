// ---- EllipticalBeam.h ----

#pragma once

#include "RadarBeam.h"

// Derived class for elliptical beam (different horizontal/vertical widths)
class EllipticalBeam : public RadarBeam {
public:
    // Override virtual methods from RadarBeam
    BeamType getBeamType() const override { return BeamType::Elliptical; }
    float getHorizontalWidth() const override { return horizontalWidthDegrees_; }
    float getVerticalWidth() const override { return verticalWidthDegrees_; }

    // Constructor and destructor
    EllipticalBeam(float sphereRadius = 100.0f, float horizontalWidthDegrees = 20.0f, float verticalWidthDegrees = 10.0f);
    virtual ~EllipticalBeam();

    // Override the virtual methods from RadarBeam for setting widths
    void setHorizontalWidth(float width) override;
    void setVerticalWidth(float width) override;

protected:
    float horizontalWidthDegrees_;
    float verticalWidthDegrees_;

    // Override the geometry creation method
    virtual void createBeamGeometry() override;
};