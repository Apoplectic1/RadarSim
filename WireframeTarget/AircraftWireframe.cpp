// AircraftWireframe.cpp

#include "AircraftWireframe.h"

AircraftWireframe::AircraftWireframe()
    : WireframeTarget()
{
}

void AircraftWireframe::generateGeometry() {
    clearGeometry();

    // Simple fighter jet - solid surfaces
    // Aircraft is oriented with nose pointing in +X direction
    // Scale: roughly fits in a 2x1x1 bounding box centered at origin

    // Helper to calculate face normal
    auto calcNormal = [](const QVector3D& v0, const QVector3D& v1, const QVector3D& v2) {
        return QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();
    };

    GLuint baseIdx;
    const float fuselageWidth = 0.1f;

    // === Fuselage as a simple elongated shape ===
    // Define fuselage cross-section points along the length
    QVector3D nose(1.0f, 0.0f, 0.0f);
    QVector3D midTop(0.0f, 0.12f, 0.0f);
    QVector3D midLeft(0.0f, 0.0f, fuselageWidth);
    QVector3D midRight(0.0f, 0.0f, -fuselageWidth);
    QVector3D midBottom(0.0f, -0.08f, 0.0f);
    QVector3D tailTop(-0.9f, 0.1f, 0.0f);
    QVector3D tailLeft(-0.9f, 0.0f, 0.05f);
    QVector3D tailRight(-0.9f, 0.0f, -0.05f);
    QVector3D tailBottom(-0.9f, 0.0f, 0.0f);

    // Nose cone (4 triangles to point)
    QVector3D noseNormalTop = calcNormal(nose, midLeft, midTop);
    baseIdx = getVertexCount();
    addVertex(nose, noseNormalTop);
    addVertex(midLeft, noseNormalTop);
    addVertex(midTop, noseNormalTop);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    QVector3D noseNormalRight = calcNormal(nose, midTop, midRight);
    baseIdx = getVertexCount();
    addVertex(nose, noseNormalRight);
    addVertex(midTop, noseNormalRight);
    addVertex(midRight, noseNormalRight);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    QVector3D noseNormalBottom = calcNormal(nose, midRight, midBottom);
    baseIdx = getVertexCount();
    addVertex(nose, noseNormalBottom);
    addVertex(midRight, noseNormalBottom);
    addVertex(midBottom, noseNormalBottom);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    QVector3D noseNormalLeft = calcNormal(nose, midBottom, midLeft);
    baseIdx = getVertexCount();
    addVertex(nose, noseNormalLeft);
    addVertex(midBottom, noseNormalLeft);
    addVertex(midLeft, noseNormalLeft);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Fuselage body (quads from mid to tail)
    // Top face
    QVector3D bodyTopNormal(0, 1, 0);
    baseIdx = getVertexCount();
    addVertex(midLeft, bodyTopNormal);
    addVertex(tailLeft, bodyTopNormal);
    addVertex(tailTop, bodyTopNormal);
    addVertex(midTop, bodyTopNormal);
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    baseIdx = getVertexCount();
    addVertex(midTop, bodyTopNormal);
    addVertex(tailTop, bodyTopNormal);
    addVertex(tailRight, bodyTopNormal);
    addVertex(midRight, bodyTopNormal);
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Bottom face
    QVector3D bodyBottomNormal(0, -1, 0);
    baseIdx = getVertexCount();
    addVertex(midRight, bodyBottomNormal);
    addVertex(tailRight, bodyBottomNormal);
    addVertex(tailBottom, bodyBottomNormal);
    addVertex(midBottom, bodyBottomNormal);
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    baseIdx = getVertexCount();
    addVertex(midBottom, bodyBottomNormal);
    addVertex(tailBottom, bodyBottomNormal);
    addVertex(tailLeft, bodyBottomNormal);
    addVertex(midLeft, bodyBottomNormal);
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // === Delta Wings ===
    const float wingThickness = 0.02f;
    QVector3D wingRootFront(0.2f, 0.0f, 0.0f);
    QVector3D wingRootBack(-0.4f, 0.0f, 0.0f);
    QVector3D wingTipLeft(-0.1f, 0.0f, 0.6f);
    QVector3D wingTipRight(-0.1f, 0.0f, -0.6f);

    // Left wing - top surface
    QVector3D wingTopNormal(0, 1, 0);
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(wingTipLeft + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(wingRootBack + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Left wing - bottom surface
    QVector3D wingBottomNormal(0, -1, 0);
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(wingRootBack + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(wingTipLeft + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right wing - top surface
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(wingRootBack + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(wingTipRight + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right wing - bottom surface
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(wingTipRight + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(wingRootBack + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // === Vertical Stabilizer (tail fin) ===
    QVector3D finBase(-0.7f, 0.1f, 0.0f);
    QVector3D finTop(-0.75f, 0.45f, 0.0f);
    QVector3D finTail(-0.9f, 0.35f, 0.0f);

    // Left side of fin
    QVector3D finLeftNormal(0, 0, 1);
    baseIdx = getVertexCount();
    addVertex(finBase + QVector3D(0, 0, 0.01f), finLeftNormal);
    addVertex(finTop + QVector3D(0, 0, 0.01f), finLeftNormal);
    addVertex(finTail + QVector3D(0, 0, 0.01f), finLeftNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right side of fin
    QVector3D finRightNormal(0, 0, -1);
    baseIdx = getVertexCount();
    addVertex(finBase + QVector3D(0, 0, -0.01f), finRightNormal);
    addVertex(finTail + QVector3D(0, 0, -0.01f), finRightNormal);
    addVertex(finTop + QVector3D(0, 0, -0.01f), finRightNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // === Horizontal Stabilizers ===
    QVector3D hStabRoot(-0.75f, 0.1f, 0.0f);
    QVector3D hStabTipLeft(-0.85f, 0.08f, 0.25f);
    QVector3D hStabTipRight(-0.85f, 0.08f, -0.25f);
    QVector3D hStabBack(-0.9f, 0.1f, 0.0f);

    // Left horizontal stabilizer - top
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(hStabTipLeft + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(hStabBack + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Left horizontal stabilizer - bottom
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(hStabBack + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(hStabTipLeft + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right horizontal stabilizer - top
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(hStabBack + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addVertex(hStabTipRight + QVector3D(0, wingThickness/2, 0), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right horizontal stabilizer - bottom
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(hStabTipRight + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addVertex(hStabBack + QVector3D(0, -wingThickness/2, 0), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);
}
