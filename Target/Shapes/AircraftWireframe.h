// AircraftWireframe.h
#pragma once

#include "WireframeTarget.h"

class AircraftWireframe : public WireframeTarget {
public:
    AircraftWireframe();
    ~AircraftWireframe() override = default;

    WireframeType getType() const override { return WireframeType::Aircraft; }

protected:
    void generateGeometry() override;
};
