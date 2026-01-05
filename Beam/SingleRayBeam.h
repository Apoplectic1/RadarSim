// SingleRayBeam.h - Diagnostic single-ray beam type
//
// This beam type renders a single line from the radar position toward the target.
// Used primarily for diagnostic purposes and bounce visualization.
// Unlike cone beams, this renders as a simple GL_LINES primitive.
//
#pragma once

#include "RadarBeam.h"

class SingleRayBeam : public RadarBeam {
public:
    explicit SingleRayBeam(float sphereRadius, float beamWidthDegrees = 0.0f);
    ~SingleRayBeam() override;

    // Override beam type
    BeamType getBeamType() const override { return BeamType::SingleRay; }

    // SingleRay has no side lobes
    float getVisualExtentMultiplier() const override { return 1.0f; }

    // Returns the single ray direction (toward target/origin)
    std::vector<QVector3D> getDiagnosticRayDirections() const override;

    // Override render to use GL_LINES instead of triangles
    void render(QOpenGLShaderProgram* program, const QMatrix4x4& projection,
                const QMatrix4x4& view, const QMatrix4x4& model) override;

protected:
    // Override geometry creation for line-based rendering
    void createBeamGeometry() override;
    void setupShaders() override;

private:
    // Line endpoint (calculated from radar position)
    QVector3D targetPoint_;
};
