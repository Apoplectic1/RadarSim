// CylinderWireframe.h
#pragma once

#include "WireframeTarget.h"

class CylinderWireframe : public WireframeTarget {
public:
    CylinderWireframe();
    ~CylinderWireframe() override = default;

    WireframeType getType() const override { return WireframeType::Cylinder; }

protected:
    void generateGeometry() override;
};
