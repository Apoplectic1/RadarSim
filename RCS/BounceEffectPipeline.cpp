// BounceEffectPipeline.cpp - Implementation
#include "BounceEffectPipeline.h"
#include <cstring>

// Include full HitResult definition
#include "../UI/MainWindow/RCSPane/Compute/RCSTypes.h"

namespace RCS {

// IntensityDecayEffect implementation
void IntensityDecayEffect::apply(BounceState& state, const HitResult& /*hit*/) {
    if (!enabled_) return;
    state.applyBounceDecay(decayFactor_, minIntensity_);
}

// BounceEffectPipeline implementation
BounceEffectPipeline::BounceEffectPipeline() {
    // Add default intensity decay effect
    addEffect(std::make_unique<IntensityDecayEffect>());
}

BounceEffectPipeline::~BounceEffectPipeline() = default;

void BounceEffectPipeline::addEffect(std::unique_ptr<BounceEffect> effect) {
    if (effect) {
        effects_.push_back(std::move(effect));
    }
}

void BounceEffectPipeline::clearEffects() {
    effects_.clear();
}

BounceEffect* BounceEffectPipeline::getEffect(const char* name) {
    for (auto& effect : effects_) {
        if (std::strcmp(effect->name(), name) == 0) {
            return effect.get();
        }
    }
    return nullptr;
}

void BounceEffectPipeline::apply(BounceState& state, const HitResult& hit) {
    // In Path mode, don't apply any effects (uniform intensity)
    if (mode_ == RayTraceMode::Path) {
        // Just increment bounce count without intensity changes
        ++state.bounceCount;
        return;
    }

    // In PhysicsAccurate mode, apply all enabled effects
    for (auto& effect : effects_) {
        if (effect->isEnabled()) {
            effect->apply(state, hit);
        }
    }
}

void BounceEffectPipeline::applyToSequence(std::vector<BounceState>& states,
                                            const std::vector<HitResult>& hits) {
    if (states.size() != hits.size()) {
        return;
    }

    BounceState cumulativeState;
    cumulativeState.reset();

    for (size_t i = 0; i < hits.size(); ++i) {
        apply(cumulativeState, hits[i]);
        states[i] = cumulativeState;
    }
}

} // namespace RCS
