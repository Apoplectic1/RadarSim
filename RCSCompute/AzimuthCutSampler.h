// ---- RCSCompute/AzimuthCutSampler.h ----
// Samples RCS hit results along a horizontal (azimuth) cut plane

#pragma once

#include "RCSSampler.h"
#include "../Constants.h"
#include <vector>

class AzimuthCutSampler : public RCSSampler {
public:
    AzimuthCutSampler();
    ~AzimuthCutSampler() override = default;

    // RCSSampler interface
    void prepare(size_t expectedHitCount) override;
    void clear() override;
    void sample(const std::vector<RCS::HitResult>& hits,
                std::vector<RCSDataPoint>& outData) override;

    void setThickness(float degrees) override { thickness_ = degrees; }
    void setOffset(float offset) override { elevationOffset_ = offset; }
    float getThickness() const override { return thickness_; }
    float getOffset() const override { return elevationOffset_; }

private:
    // Configuration
    float thickness_ = RS::Constants::kDefaultSliceThickness;  // ±degrees
    float elevationOffset_ = 0.0f;  // Elevation angle of the horizontal slice (0 = equator)

    // Pre-allocated binning arrays (one per degree, 0-359)
    std::vector<float> binIntensity_;   // Accumulated intensity per bin
    std::vector<int> binHitCount_;      // Hit count per bin

    // Helper methods
    bool isHitInSlice(const RCS::HitResult& hit) const;
    int getAzimuthBin(const QVector3D& reflectionDir) const;
    float intensityToDBsm(float intensity) const;
    bool validateHit(const RCS::HitResult& hit) const;
};
