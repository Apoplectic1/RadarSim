// ---- PhasedArrayBeam.h ----
#pragma once

#include "RadarBeam.h"
#include <functional>

// Derived class for phased array beam with full beam forming capabilities
class PhasedArrayBeam : public RadarBeam {
public:
    BeamType getBeamType() const override { return BeamType::Phased; }
    PhasedArrayBeam(float sphereRadius = 100.0f, float mainLobeWidthDegrees = 15.0f);
    virtual ~PhasedArrayBeam();

    // Specialized methods for phased arrays
    void setMainLobeDirection(float azimuthOffset, float elevationOffset);
    void setElementCount(int horizontalElements, int verticalElements);
    void setElementSpacing(float horizontalSpacing, float verticalSpacing);
    void setSideLobeVisibility(bool visible);
    void setSideLobeIntensity(float intensity);  // 0.0 to 1.0

    // Custom beam pattern using function
    void setCustomPattern(std::function<float(float azimuth, float elevation)> patternFunc);

protected:
    // Phased array specific properties
    float azimuthOffset_;
    float elevationOffset_;
    int horizontalElements_;
    int verticalElements_;
    float horizontalSpacing_;
    float verticalSpacing_;
    bool showSideLobes_;
    float sideLobeIntensity_;
    std::function<float(float, float)> patternFunction_;

    // Additional geometry for side lobes
    std::vector<float> sideLobeVertices_;
    std::vector<unsigned int> sideLobeIndices_;
    QOpenGLVertexArrayObject sideLobeVAO_;
    QOpenGLBuffer sideLobeVBO_;
    QOpenGLBuffer sideLobeEBO_;

    virtual void createBeamGeometry() override;
    void createSideLobes();
    void calculateBeamPattern();
};
