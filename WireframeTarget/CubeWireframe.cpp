// CubeWireframe.cpp

#include "CubeWireframe.h"

CubeWireframe::CubeWireframe()
    : WireframeTarget()
{
}

void CubeWireframe::generateGeometry() {
    clearGeometry();

    // Unit cube centered at origin: vertices at +/-0.5
    // Z-up convention: Z is vertical, Y is horizontal (was swapped)
    const float h = 0.5f;

    // Face normals (Z-up convention)
    QVector3D normalFront(0, -1, 0);   // was (0, 0, -1)
    QVector3D normalBack(0, 1, 0);     // was (0, 0, 1)
    QVector3D normalLeft(-1, 0, 0);
    QVector3D normalRight(1, 0, 0);
    QVector3D normalTop(0, 0, 1);      // was (0, 1, 0)
    QVector3D normalBottom(0, 0, -1);  // was (0, -1, 0)

    GLuint baseIdx;

    // Front face (y = -h) - CCW winding when viewed from front
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h, -h, -h), normalFront);  // 0
    addVertex(QVector3D( h, -h, -h), normalFront);  // 1
    addVertex(QVector3D( h, -h,  h), normalFront);  // 2 (swapped y,z)
    addVertex(QVector3D(-h, -h,  h), normalFront);  // 3 (swapped y,z)
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Back face (y = +h) - CCW winding when viewed from back
    baseIdx = getVertexCount();
    addVertex(QVector3D( h,  h, -h), normalBack);   // 0 (swapped y,z)
    addVertex(QVector3D(-h,  h, -h), normalBack);   // 1 (swapped y,z)
    addVertex(QVector3D(-h,  h,  h), normalBack);   // 2 (swapped y,z)
    addVertex(QVector3D( h,  h,  h), normalBack);   // 3 (swapped y,z)
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Left face (x = -h) - CCW winding when viewed from left
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h,  h, -h), normalLeft);   // 0 (swapped y,z)
    addVertex(QVector3D(-h, -h, -h), normalLeft);   // 1 (swapped y,z)
    addVertex(QVector3D(-h, -h,  h), normalLeft);   // 2 (swapped y,z)
    addVertex(QVector3D(-h,  h,  h), normalLeft);   // 3 (swapped y,z)
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Right face (x = +h) - CCW winding when viewed from right
    baseIdx = getVertexCount();
    addVertex(QVector3D( h, -h, -h), normalRight);  // 0 (swapped y,z)
    addVertex(QVector3D( h,  h, -h), normalRight);  // 1 (swapped y,z)
    addVertex(QVector3D( h,  h,  h), normalRight);  // 2 (swapped y,z)
    addVertex(QVector3D( h, -h,  h), normalRight);  // 3 (swapped y,z)
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Top face (z = +h) - CCW winding when viewed from top
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h, -h,  h), normalTop);    // 0 (swapped y,z)
    addVertex(QVector3D( h, -h,  h), normalTop);    // 1 (swapped y,z)
    addVertex(QVector3D( h,  h,  h), normalTop);    // 2 (swapped y,z)
    addVertex(QVector3D(-h,  h,  h), normalTop);    // 3 (swapped y,z)
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Bottom face (z = -h) - CCW winding when viewed from bottom
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h,  h, -h), normalBottom); // 0 (swapped y,z)
    addVertex(QVector3D( h,  h, -h), normalBottom); // 1 (swapped y,z)
    addVertex(QVector3D( h, -h, -h), normalBottom); // 2 (swapped y,z)
    addVertex(QVector3D(-h, -h, -h), normalBottom); // 3 (swapped y,z)
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);
}
