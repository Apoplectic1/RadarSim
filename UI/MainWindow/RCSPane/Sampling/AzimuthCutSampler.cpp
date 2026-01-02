// ---- RCSCompute/AzimuthCutSampler.cpp ----

#include "AzimuthCutSampler.h"
#include <cmath>
#include <algorithm>

using namespace RS::Constants;

AzimuthCutSampler::AzimuthCutSampler() {
    // Pre-allocate binning arrays
    binIntensity_.resize(kPolarPlotBins, 0.0f);
    binHitCount_.resize(kPolarPlotBins, 0);
}

void AzimuthCutSampler::prepare(size_t /*expectedHitCount*/) {
    // For now, we don't need to resize based on hit count
    // The binning arrays are already sized to kPolarPlotBins
}

void AzimuthCutSampler::clear() {
    std::fill(binIntensity_.begin(), binIntensity_.end(), 0.0f);
    std::fill(binHitCount_.begin(), binHitCount_.end(), 0);
}

void AzimuthCutSampler::sample(const std::vector<RCS::HitResult>& hits,
                                std::vector<RCSDataPoint>& outData) {
    // 1. Clear bins
    clear();

    // 2. Accumulate hits into bins
    for (const auto& hit : hits) {
        // Skip misses (distance == -1)
        if (hit.hitPoint.w() < 0.0f) {
            continue;
        }

        // Validate intensity (must be >= 0, not NaN/Inf)
        if (!validateHit(hit)) {
            continue;
        }

        // Check if hit is within our angular slice
        if (!isHitInSlice(hit)) {
            continue;
        }

        // Get azimuth bin (0-359)
        int bin = getAzimuthBin(hit.reflection.toVector3D());
        if (bin < 0 || bin >= kPolarPlotBins) {
            continue;
        }

        // Accumulate intensity
        binIntensity_[bin] += hit.reflection.w();  // w = intensity
        binHitCount_[bin]++;
    }

    // 3. Convert to dBsm and output
    outData.resize(kPolarPlotBins);
    for (int i = 0; i < kPolarPlotBins; ++i) {
        outData[i].angleDegrees = static_cast<float>(i);

        if (binHitCount_[i] > 0) {
            float avgIntensity = binIntensity_[i] / static_cast<float>(binHitCount_[i]);
            outData[i].dBsm = intensityToDBsm(avgIntensity);
            outData[i].valid = true;
        } else {
            // Empty bin - use floor value
            outData[i].dBsm = kDBsmFloor;
            outData[i].valid = false;
        }
    }
}

bool AzimuthCutSampler::validateHit(const RCS::HitResult& hit) const {
    float intensity = hit.reflection.w();

    // Check for negative, NaN, or infinity
    if (intensity < 0.0f || std::isnan(intensity) || std::isinf(intensity)) {
        return false;
    }

    return true;
}

bool AzimuthCutSampler::isHitInSlice(const RCS::HitResult& hit) const {
    // Get the reflection direction
    QVector3D reflectDir = hit.reflection.toVector3D().normalized();

    // Calculate elevation angle of reflection direction
    // Elevation = angle from horizontal plane (XY plane)
    // asin(z) gives elevation in radians
    float elevationRad = std::asin(std::clamp(reflectDir.z(), -1.0f, 1.0f));
    float elevationDeg = elevationRad * kRadToDegF;

    // Check if within slice thickness of the offset elevation
    float delta = std::abs(elevationDeg - elevationOffset_);
    return delta <= thickness_;
}

int AzimuthCutSampler::getAzimuthBin(const QVector3D& reflectionDir) const {
    // Get normalized direction in XY plane
    float x = reflectionDir.x();
    float y = reflectionDir.y();

    // Calculate azimuth angle (0 = +X axis, counter-clockwise)
    // atan2 returns radians in range [-pi, pi]
    float azimuthRad = std::atan2(y, x);

    // Convert to degrees [0, 360)
    float azimuthDeg = azimuthRad * kRadToDegF;
    if (azimuthDeg < 0.0f) {
        azimuthDeg += 360.0f;
    }

    // Convert to bin index (0-359)
    int bin = static_cast<int>(std::floor(azimuthDeg));
    if (bin >= kPolarPlotBins) {
        bin = kPolarPlotBins - 1;  // Handle edge case of exactly 360
    }
    if (bin < 0) {
        bin = 0;
    }

    return bin;
}

float AzimuthCutSampler::intensityToDBsm(float intensity) const {
    if (intensity <= kMinValidIntensity) {
        return kDBsmFloor;
    }

    // Convert intensity to dBsm: 10 * log10(intensity)
    float dBsm = 10.0f * std::log10(intensity);

    // Clamp to floor
    return std::max(dBsm, kDBsmFloor);
}
