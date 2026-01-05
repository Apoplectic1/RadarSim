// ---- ConicalBeam.cpp ----

#include "ConicalBeam.h"

ConicalBeam::ConicalBeam(float sphereRadius, float beamWidthDegrees)
    : RadarBeam(sphereRadius, beamWidthDegrees)
{
    // Don't call initialize() here - it requires a valid OpenGL context
    // BeamController::createBeam() will call initialize() after construction
}

ConicalBeam::~ConicalBeam() {
    // Base class destructor will handle cleanup
}

void ConicalBeam::createBeamGeometry() {
    // Use the base implementation for conical beam
    RadarBeam::createBeamGeometry();
}