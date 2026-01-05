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
    // Z-up convention: Z is vertical, Y is horizontal span
    // Scale: roughly fits in a 2x1x1 bounding box centered at origin

    // Helper to calculate face normal
    auto calcNormal = [](const QVector3D& v0, const QVector3D& v1, const QVector3D& v2) {
        return QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();
    };

    GLuint baseIdx;
    const float fuselageWidth = 0.1f;

    // === Fuselage as a simple elongated shape ===
    // Define fuselage cross-section points along the length
    // Z-up convention: y is horizontal (was z), z is vertical (was y)
    QVector3D nose(1.0f, 0.0f, 0.0f);
    QVector3D midTop(0.0f, 0.0f, 0.12f);           // was (0, 0.12, 0)
    QVector3D midLeft(0.0f, fuselageWidth, 0.0f);  // was (0, 0, fuselageWidth)
    QVector3D midRight(0.0f, -fuselageWidth, 0.0f);// was (0, 0, -fuselageWidth)
    QVector3D midBottom(0.0f, 0.0f, -0.08f);       // was (0, -0.08, 0)
    QVector3D tailTop(-0.9f, 0.0f, 0.1f);          // was (-0.9, 0.1, 0)
    QVector3D tailLeft(-0.9f, 0.05f, 0.0f);        // was (-0.9, 0, 0.05)
    QVector3D tailRight(-0.9f, -0.05f, 0.0f);      // was (-0.9, 0, -0.05)
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
    // Top face (Z-up: normal is (0, 0, 1))
    QVector3D bodyTopNormal(0, 0, 1);   // was (0, 1, 0)
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

    // Bottom face (Z-up: normal is (0, 0, -1))
    QVector3D bodyBottomNormal(0, 0, -1);  // was (0, -1, 0)
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
    QVector3D wingTipLeft(-0.1f, 0.6f, 0.0f);    // was (-.1, 0, 0.6)
    QVector3D wingTipRight(-0.1f, -0.6f, 0.0f);  // was (-.1, 0, -0.6)

    // Left wing - top surface (Z-up: normal is (0, 0, 1))
    QVector3D wingTopNormal(0, 0, 1);  // was (0, 1, 0)
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(wingTipLeft + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(wingRootBack + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Left wing - bottom surface (Z-up: normal is (0, 0, -1))
    QVector3D wingBottomNormal(0, 0, -1);  // was (0, -1, 0)
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(wingRootBack + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(wingTipLeft + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right wing - top surface
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(wingRootBack + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(wingTipRight + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right wing - bottom surface
    baseIdx = getVertexCount();
    addVertex(wingRootFront + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(wingTipRight + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(wingRootBack + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // === Vertical Stabilizer (tail fin) ===
    // Z-up: fin extends upward in +Z, has left/right faces in Y
    QVector3D finBase(-0.7f, 0.0f, 0.1f);    // was (-0.7, 0.1, 0)
    QVector3D finTop(-0.75f, 0.0f, 0.45f);   // was (-0.75, 0.45, 0)
    QVector3D finTail(-0.9f, 0.0f, 0.35f);   // was (-0.9, 0.35, 0)

    // Left side of fin (Y+)
    QVector3D finLeftNormal(0, 1, 0);   // was (0, 0, 1)
    baseIdx = getVertexCount();
    addVertex(finBase + QVector3D(0, 0.01f, 0), finLeftNormal);
    addVertex(finTop + QVector3D(0, 0.01f, 0), finLeftNormal);
    addVertex(finTail + QVector3D(0, 0.01f, 0), finLeftNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right side of fin (Y-)
    QVector3D finRightNormal(0, -1, 0);  // was (0, 0, -1)
    baseIdx = getVertexCount();
    addVertex(finBase + QVector3D(0, -0.01f, 0), finRightNormal);
    addVertex(finTail + QVector3D(0, -0.01f, 0), finRightNormal);
    addVertex(finTop + QVector3D(0, -0.01f, 0), finRightNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // === Horizontal Stabilizers ===
    // Z-up: these extend in Y direction (horizontal span), slightly above center
    QVector3D hStabRoot(-0.75f, 0.0f, 0.1f);        // was (-0.75, 0.1, 0)
    QVector3D hStabTipLeft(-0.85f, 0.25f, 0.08f);   // was (-0.85, 0.08, 0.25)
    QVector3D hStabTipRight(-0.85f, -0.25f, 0.08f); // was (-0.85, 0.08, -0.25)
    QVector3D hStabBack(-0.9f, 0.0f, 0.1f);         // was (-0.9, 0.1, 0)

    // Left horizontal stabilizer - top
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(hStabTipLeft + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(hStabBack + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Left horizontal stabilizer - bottom
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(hStabBack + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(hStabTipLeft + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right horizontal stabilizer - top
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(hStabBack + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addVertex(hStabTipRight + QVector3D(0, 0, wingThickness/2), wingTopNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Right horizontal stabilizer - bottom
    baseIdx = getVertexCount();
    addVertex(hStabRoot + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(hStabTipRight + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addVertex(hStabBack + QVector3D(0, 0, -wingThickness/2), wingBottomNormal);
    addTriangle(baseIdx, baseIdx+1, baseIdx+2);

    // Detect crease edges for rendering (structural edges, not internal triangulation)
    detectEdges();
    generateEdgeGeometry();
}
