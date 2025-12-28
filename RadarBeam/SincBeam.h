// ---- SincBeam.h ----
// Sinc² beam pattern with intensity falloff and side lobes

#pragma once

#include "RadarBeam.h"

class SincBeam : public RadarBeam {
public:
    SincBeam(float sphereRadius = 100.0f, float beamWidthDegrees = 15.0f);
    ~SincBeam() override;

    BeamType getBeamType() const override { return BeamType::Sinc; }

    // Static utility for sinc² calculation (also used by RCS ray weighting)
    // theta: angle from beam axis (radians)
    // thetaMax: half-angle of main lobe (radians) - first null at theta = thetaMax
    // Returns: sinc²(π * theta / thetaMax) = [sin(π * theta / thetaMax) / (π * theta / thetaMax)]²
    static float getSincSquaredIntensity(float theta, float thetaMax);

    // Override render to set sideLobeColor uniform
    void render(QOpenGLShaderProgram* program, const QMatrix4x4& projection,
                const QMatrix4x4& view, const QMatrix4x4& model) override;

protected:
    void createBeamGeometry() override;
    void setupShaders() override;

    // Override for 7-float vertex format (pos + normal + intensity)
    void uploadGeometryToGPU();

private:
    // Generate vertices with per-vertex intensity for sinc² pattern
    void generateSincBeamVertices(const QVector3D& apex,
                                   const QVector3D& direction,
                                   float length,
                                   float mainLobeRadius);
};
