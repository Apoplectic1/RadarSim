// ---- RCSCompute/ElevationCutSampler.cpp ----

#include "ElevationCutSampler.h"
#include <cmath>
#include <algorithm>

using namespace RS::Constants;

ElevationCutSampler::ElevationCutSampler() {
    // Pre-allocate binning arrays
    binIntensity_.resize(kPolarPlotBins, 0.0f);
    binHitCount_.resize(kPolarPlotBins, 0);
}

void ElevationCutSampler::prepare(size_t /*expectedHitCount*/) {
    // For now, we don't need to resize based on hit count
    // The binning arrays are already sized to kPolarPlotBins
}

void ElevationCutSampler::clear() {
    std::fill(binIntensity_.begin(), binIntensity_.end(), 0.0f);
    std::fill(binHitCount_.begin(), binHitCount_.end(), 0);
}

void ElevationCutSampler::sample(const std::vector<RCS::HitResult>& hits,
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

        // Get elevation bin (0-359)
        int bin = getElevationBin(hit.reflection.toVector3D());
        if (bin < 0 || bin >= kPolarPlotBins) {
            continue;
        }

        // Accumulate intensity
        binIntensity_[bin] += hit.reflection.w();  // w = intensity
        binHitCount_[bin]++;
    }

    // 3. Convert to dBsm and output
    // For elevation cut: bin 0 = -90°, bin 90 = 0°, bin 180 = +90°, etc.
    // We map bins to degrees for the polar plot display
    outData.resize(kPolarPlotBins);
    for (int i = 0; i < kPolarPlotBins; ++i) {
        // Convert bin index to elevation angle for display
        // Bin 0 corresponds to the "0 degree" position on the polar plot
        // which we map to -90° elevation, going counter-clockwise
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

bool ElevationCutSampler::validateHit(const RCS::HitResult& hit) const {
    float intensity = hit.reflection.w();

    // Check for negative, NaN, or infinity
    if (intensity < 0.0f || std::isnan(intensity) || std::isinf(intensity)) {
        return false;
    }

    return true;
}

bool ElevationCutSampler::isHitInSlice(const RCS::HitResult& hit) const {
    // Get the reflection direction
    QVector3D reflectDir = hit.reflection.toVector3D().normalized();

    // Calculate azimuth angle of reflection direction
    // Azimuth = angle in XY plane from +X axis
    float azimuthRad = std::atan2(reflectDir.y(), reflectDir.x());
    float azimuthDeg = azimuthRad * kRadToDegF;

    // Normalize azimuthOffset_ to [-180, 180)
    float offsetNorm = azimuthOffset_;
    while (offsetNorm >= 180.0f) offsetNorm -= 360.0f;
    while (offsetNorm < -180.0f) offsetNorm += 360.0f;

    // Check both sides of the vertical plane (azimuth and azimuth + 180)
    float delta1 = std::abs(azimuthDeg - offsetNorm);
    if (delta1 > 180.0f) delta1 = 360.0f - delta1;

    float oppositeAzimuth = offsetNorm + 180.0f;
    if (oppositeAzimuth >= 180.0f) oppositeAzimuth -= 360.0f;
    float delta2 = std::abs(azimuthDeg - oppositeAzimuth);
    if (delta2 > 180.0f) delta2 = 360.0f - delta2;

    float minDelta = std::min(delta1, delta2);
    return minDelta <= thickness_;
}

int ElevationCutSampler::getElevationBin(const QVector3D& reflectionDir) const {
    QVector3D dir = reflectionDir.normalized();

    // Calculate elevation angle
    // Elevation = asin(z) gives angle from horizontal plane
    float elevationRad = std::asin(std::clamp(dir.z(), -1.0f, 1.0f));
    float elevationDeg = elevationRad * kRadToDegF;  // Range: [-90, +90]

    // Determine which side of the vertical plane (front or back)
    float azimuthRad = std::atan2(dir.y(), dir.x());
    float azimuthDeg = azimuthRad * kRadToDegF;

    float offsetNorm = azimuthOffset_;
    while (offsetNorm >= 180.0f) offsetNorm -= 360.0f;
    while (offsetNorm < -180.0f) offsetNorm += 360.0f;

    float delta = azimuthDeg - offsetNorm;
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;

    // Map elevation to bin:
    // - Front side (delta near 0): elevation -90 to +90 maps to bins 0-180
    // - Back side (delta near ±180): elevation +90 to -90 maps to bins 180-360
    int bin;
    if (std::abs(delta) <= 90.0f) {
        // Front side: -90° -> bin 0, 0° -> bin 90, +90° -> bin 180
        bin = static_cast<int>(std::round(elevationDeg + 90.0f));
    } else {
        // Back side: +90° -> bin 180, 0° -> bin 270, -90° -> bin 360 (wraps to 0)
        bin = static_cast<int>(std::round(270.0f - elevationDeg));
    }

    // Clamp to valid range
    if (bin >= kPolarPlotBins) bin = kPolarPlotBins - 1;
    if (bin < 0) bin = 0;

    return bin;
}

float ElevationCutSampler::intensityToDBsm(float intensity) const {
    if (intensity <= kMinValidIntensity) {
        return kDBsmFloor;
    }

    // Convert intensity to dBsm: 10 * log10(intensity)
    float dBsm = 10.0f * std::log10(intensity);

    // Clamp to floor
    return std::max(dBsm, kDBsmFloor);
}
