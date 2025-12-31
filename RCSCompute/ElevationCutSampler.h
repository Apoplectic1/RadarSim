// ---- RCSCompute/ElevationCutSampler.h ----
// Samples RCS hit results along a vertical (elevation) cut plane

#pragma once

#include "RCSSampler.h"
#include "../Constants.h"
#include <vector>

class ElevationCutSampler : public RCSSampler {
public:
    ElevationCutSampler();
    ~ElevationCutSampler() override = default;

    // RCSSampler interface
    void prepare(size_t expectedHitCount) override;
    void clear() override;
    void sample(const std::vector<RCS::HitResult>& hits,
                std::vector<RCSDataPoint>& outData) override;

    void setThickness(float degrees) override { thickness_ = degrees; }
    void setOffset(float offset) override { azimuthOffset_ = offset; }
    float getThickness() const override { return thickness_; }
    float getOffset() const override { return azimuthOffset_; }

private:
    // Configuration
    float thickness_ = RS::Constants::kDefaultSliceThickness;  // ±degrees
    float azimuthOffset_ = 0.0f;  // Azimuth angle of the vertical slice (0 = +X direction)

    // Pre-allocated binning arrays (one per degree, 0-359 mapped to elevation)
    // Bin 0 = -90° elevation, Bin 180 = 0° elevation, Bin 359 = +89°
    std::vector<float> binIntensity_;   // Accumulated intensity per bin
    std::vector<int> binHitCount_;      // Hit count per bin

    // Helper methods
    bool isHitInSlice(const RCS::HitResult& hit) const;
    int getElevationBin(const QVector3D& reflectionDir) const;
    float intensityToDBsm(float intensity) const;
    bool validateHit(const RCS::HitResult& hit) const;
};
