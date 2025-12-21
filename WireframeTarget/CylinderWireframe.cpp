// CylinderWireframe.cpp

#include "CylinderWireframe.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CylinderWireframe::CylinderWireframe()
    : WireframeTarget()
{
}

void CylinderWireframe::generateGeometry() {
    clearGeometry();

    const float radius = 0.5f;
    const float halfHeight = 0.5f;
    const int segments = 24;

    GLuint baseIdx;

    // Z-up convention: height is along Z axis, circle is in X-Y plane

    // === Top cap (z = +halfHeight) ===
    // Center vertex
    GLuint topCenterIdx = getVertexCount();
    addVertex(QVector3D(0, 0, halfHeight), QVector3D(0, 0, 1));

    // Rim vertices for top cap
    GLuint topRimStart = getVertexCount();
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * cos(theta);
        float y = radius * sin(theta);
        addVertex(QVector3D(x, y, halfHeight), QVector3D(0, 0, 1));
    }

    // Top cap triangles (fan from center)
    for (int i = 0; i < segments; i++) {
        addTriangle(topCenterIdx, topRimStart + i, topRimStart + i + 1);
    }

    // === Bottom cap (z = -halfHeight) ===
    // Center vertex
    GLuint bottomCenterIdx = getVertexCount();
    addVertex(QVector3D(0, 0, -halfHeight), QVector3D(0, 0, -1));

    // Rim vertices for bottom cap
    GLuint bottomRimStart = getVertexCount();
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * cos(theta);
        float y = radius * sin(theta);
        addVertex(QVector3D(x, y, -halfHeight), QVector3D(0, 0, -1));
    }

    // Bottom cap triangles (reversed winding for outward normal)
    for (int i = 0; i < segments; i++) {
        addTriangle(bottomCenterIdx, bottomRimStart + i + 1, bottomRimStart + i);
    }

    // === Side surface ===
    // Top ring vertices with radial normals
    GLuint sideTopStart = getVertexCount();
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * cos(theta);
        float y = radius * sin(theta);
        QVector3D normal(cos(theta), sin(theta), 0);
        addVertex(QVector3D(x, y, halfHeight), normal);
    }

    // Bottom ring vertices with radial normals
    GLuint sideBottomStart = getVertexCount();
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * cos(theta);
        float y = radius * sin(theta);
        QVector3D normal(cos(theta), sin(theta), 0);
        addVertex(QVector3D(x, y, -halfHeight), normal);
    }

    // Side surface quads
    for (int i = 0; i < segments; i++) {
        addQuad(sideTopStart + i, sideBottomStart + i,
                sideBottomStart + i + 1, sideTopStart + i + 1);
    }
}
