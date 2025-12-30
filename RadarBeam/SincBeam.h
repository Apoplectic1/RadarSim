// ---- SincBeam.h ----
// Airy beam pattern (circular aperture) with intensity falloff and side lobes

#pragma once

#include "RadarBeam.h"

class SincBeam : public RadarBeam {
public:
    SincBeam(float sphereRadius = 100.0f, float beamWidthDegrees = 15.0f);
    ~SincBeam() override;

    BeamType getBeamType() const override { return BeamType::Sinc; }

    // Airy pattern for circular aperture: [2·J₁(x)/x]²
    // theta: angle from beam axis (radians)
    // thetaMax: half-angle of main lobe (radians) - first null at theta = thetaMax
    // Returns: Airy intensity pattern normalized to 1.0 at center
    static float getAiryIntensity(float theta, float thetaMax);

    // Bessel function J₁(x) - first kind, order 1
    // Uses polynomial approximation (Numerical Recipes)
    static float besselJ1(float x);

    // Legacy sinc² calculation (kept for RCS ray weighting compatibility)
    static float getSincSquaredIntensity(float theta, float thetaMax);

    // Override render to set sideLobeColor uniform
    void render(QOpenGLShaderProgram* program, const QMatrix4x4& projection,
                const QMatrix4x4& view, const QMatrix4x4& model) override;

protected:
    void createBeamGeometry() override;
    void setupShaders() override;

    // Override for 7-float vertex format (pos + normal + intensity)
    void uploadGeometryToGPU() override;

private:
    // Generate vertices with per-vertex intensity for sinc² pattern
    void generateSincBeamVertices(const QVector3D& apex,
                                   const QVector3D& direction,
                                   float length,
                                   float mainLobeRadius);
};
