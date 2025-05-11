// ---- ConicalBeam.cpp ----

#include "ConicalBeam.h"

ConicalBeam::ConicalBeam(float sphereRadius, float beamWidthDegrees)
    : RadarBeam(sphereRadius, beamWidthDegrees)
{
    // Make sure initialize is called if this is the first beam
    // Note: This will be called again by SphereWidget but that's ok
    initialize();
}

ConicalBeam::~ConicalBeam() {
    // Base class destructor will handle cleanup
}

void ConicalBeam::createBeamGeometry() {
    // Use the base implementation for conical beam
    RadarBeam::createBeamGeometry();
}
