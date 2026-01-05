// RayTraceTypes.h - Ray tracing mode and bounce state types
#pragma once

#include <cstdint>

namespace RCS {

// Ray tracing modes for bounce visualization
enum class RayTraceMode {
    Path,           // Show all ray paths at uniform brightness (no intensity losses)
    PhysicsAccurate // Apply full physics effects (intensity losses per bounce)
};

// State tracking for ray bounces during tracing
// This struct accumulates effects as a ray bounces through the scene
struct BounceState {
    float intensity = 1.0f;      // Current intensity (1.0 = full, decays with bounces)
    float pathLength = 0.0f;     // Total path length traveled
    uint32_t materialId = 0;     // Last material hit (for future material effects)
    int bounceCount = 0;         // Number of bounces so far

    // Reset state for a new ray trace
    void reset() {
        intensity = 1.0f;
        pathLength = 0.0f;
        materialId = 0;
        bounceCount = 0;
    }

    // Apply intensity decay for a bounce (used in PhysicsAccurate mode)
    void applyBounceDecay(float decayFactor, float minIntensity) {
        intensity *= (1.0f - decayFactor);
        if (intensity < minIntensity) {
            intensity = minIntensity;
        }
        ++bounceCount;
    }

    // Add path length from this bounce segment
    void addPathLength(float length) {
        pathLength += length;
    }
};

} // namespace RCS
