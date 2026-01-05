// BounceEffectPipeline.h - Manages and applies bounce effects
//
// The pipeline holds a list of effects and applies them in order.
// Effects can be added, removed, and enabled/disabled independently.
//
// In Path mode: No effects are applied (uniform intensity)
// In PhysicsAccurate mode: All enabled effects are applied
//
#pragma once

#include "RayTraceTypes.h"
#include "BounceEffect.h"
#include <vector>
#include <memory>

namespace RCS {

class BounceEffectPipeline {
public:
    BounceEffectPipeline();
    ~BounceEffectPipeline();

    // Set the ray trace mode
    void setMode(RayTraceMode mode) { mode_ = mode; }
    RayTraceMode getMode() const { return mode_; }

    // Add an effect to the pipeline
    // Effects are applied in the order they are added
    void addEffect(std::unique_ptr<BounceEffect> effect);

    // Remove all effects
    void clearEffects();

    // Get effect by name (for configuration)
    BounceEffect* getEffect(const char* name);

    // Apply all enabled effects to the bounce state
    // In Path mode, this does nothing (uniform intensity)
    // In PhysicsAccurate mode, all enabled effects are applied
    void apply(BounceState& state, const HitResult& hit);

    // Apply effects to a full bounce sequence
    void applyToSequence(std::vector<BounceState>& states,
                         const std::vector<HitResult>& hits);

    // Get number of effects
    size_t effectCount() const { return effects_.size(); }

private:
    RayTraceMode mode_ = RayTraceMode::PhysicsAccurate;
    std::vector<std::unique_ptr<BounceEffect>> effects_;
};

} // namespace RCS
