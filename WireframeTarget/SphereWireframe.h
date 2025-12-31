// SphereWireframe.h
// Geodesic sphere target for RCS verification (theoretical RCS = pi*r^2)
#pragma once

#include "WireframeTarget.h"

class SphereWireframe : public WireframeTarget {
public:
    explicit SphereWireframe(int subdivisions = 3);
    ~SphereWireframe() override = default;

    WireframeType getType() const override { return WireframeType::Sphere; }

    // Get subdivision level (0=20 faces, 1=80, 2=320, 3=1280, 4=5120)
    int getSubdivisions() const { return subdivisions_; }

    // Set subdivision level and regenerate geometry
    void setSubdivisions(int level);

protected:
    void generateGeometry() override;

private:
    int subdivisions_;  // Number of recursive subdivisions (0-4 typical)

    // Helper methods for icosahedron subdivision
    void createIcosahedron();
    void subdivide(int levels);
    QVector3D getMidpoint(const QVector3D& v1, const QVector3D& v2);
};
