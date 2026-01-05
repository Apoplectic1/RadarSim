// BounceEffect.h - Abstract interface for bounce physics effects
//
// This interface allows different physics effects to be applied to ray bounces.
// Each effect is a separate class that can be added/removed/tested independently.
//
// Future effects can include:
// - MaterialReflectivity: Intensity loss based on material properties
// - AtmosphericAttenuation: Distance-based intensity decay
// - TurbulenceScatter: Random perturbation of reflection direction
// - TemperatureGradient: Ray bending due to temperature variations
//
#pragma once

#include "RayTraceTypes.h"

// Forward declare HitResult from RCSTypes.h
namespace RCS {
struct HitResult;
}

namespace RCS {

// Abstract base class for all bounce effects
class BounceEffect {
public:
    virtual ~BounceEffect() = default;

    // Apply this effect to the bounce state based on the hit result
    // @param state: Mutable bounce state to modify
    // @param hit: The hit result that triggered this bounce
    virtual void apply(BounceState& state, const HitResult& hit) = 0;

    // Get the name of this effect (for debugging/UI)
    virtual const char* name() const = 0;

    // Enable/disable this effect
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

protected:
    bool enabled_ = true;
};

// Default intensity decay effect (always applied in Physics mode)
// This is the basic effect that reduces intensity per bounce
class IntensityDecayEffect : public BounceEffect {
public:
    IntensityDecayEffect(float decayFactor = 0.15f, float minIntensity = 0.2f)
        : decayFactor_(decayFactor), minIntensity_(minIntensity) {}

    void apply(BounceState& state, const HitResult& hit) override;
    const char* name() const override { return "Intensity Decay"; }

    void setDecayFactor(float factor) { decayFactor_ = factor; }
    void setMinIntensity(float min) { minIntensity_ = min; }

private:
    float decayFactor_;   // How much intensity is lost per bounce (0-1)
    float minIntensity_;  // Floor intensity (never goes below this)
};

} // namespace RCS
