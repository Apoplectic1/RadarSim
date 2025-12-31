// ---- RCSCompute/RCSSampler.h ----
// Abstract interface for sampling RCS hit results into polar plot data

#pragma once

#include "RCSTypes.h"
#include "../PolarPlot/PolarRCSPlot.h"  // For RCSDataPoint
#include <vector>

// Cut type for RCS slicing plane
enum class CutType {
    Azimuth = 0,    // Horizontal cut (samples by azimuth angle)
    Elevation = 1   // Vertical cut (samples by elevation angle)
};

// Abstract interface for RCS sampling strategies
class RCSSampler {
public:
    virtual ~RCSSampler() = default;

    // Prepare sampler for expected data size (pre-allocate buffers)
    virtual void prepare(size_t expectedHitCount) = 0;

    // Clear internal state between samples
    virtual void clear() = 0;

    // Sample hit results, produce angle->dBsm data
    // Output vector should be pre-sized to kPolarPlotBins (360) entries
    virtual void sample(const std::vector<RCS::HitResult>& hits,
                        std::vector<RCSDataPoint>& outData) = 0;

    // Configuration
    virtual void setThickness(float degrees) = 0;  // Angular slab ±thickness
    virtual void setOffset(float offset) = 0;      // Plane offset (elevation for azimuth cut)

    // Get current settings
    virtual float getThickness() const = 0;
    virtual float getOffset() const = 0;
};
