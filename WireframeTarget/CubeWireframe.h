// CubeWireframe.h
#pragma once

#include "WireframeTarget.h"

class CubeWireframe : public WireframeTarget {
public:
    CubeWireframe();
    ~CubeWireframe() override = default;

    WireframeType getType() const override { return WireframeType::Cube; }

protected:
    void generateGeometry() override;
};
