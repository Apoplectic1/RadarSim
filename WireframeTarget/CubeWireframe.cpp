// CubeWireframe.cpp

#include "CubeWireframe.h"

CubeWireframe::CubeWireframe()
    : WireframeTarget()
{
}

void CubeWireframe::generateGeometry() {
    clearGeometry();

    // Unit cube centered at origin: vertices at +/-0.5
    const float h = 0.5f;

    // Face normals
    QVector3D normalFront(0, 0, -1);
    QVector3D normalBack(0, 0, 1);
    QVector3D normalLeft(-1, 0, 0);
    QVector3D normalRight(1, 0, 0);
    QVector3D normalTop(0, 1, 0);
    QVector3D normalBottom(0, -1, 0);

    GLuint baseIdx;

    // Front face (z = -h) - CCW winding when viewed from front
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h, -h, -h), normalFront);  // 0
    addVertex(QVector3D( h, -h, -h), normalFront);  // 1
    addVertex(QVector3D( h,  h, -h), normalFront);  // 2
    addVertex(QVector3D(-h,  h, -h), normalFront);  // 3
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Back face (z = +h) - CCW winding when viewed from back
    baseIdx = getVertexCount();
    addVertex(QVector3D( h, -h,  h), normalBack);   // 0
    addVertex(QVector3D(-h, -h,  h), normalBack);   // 1
    addVertex(QVector3D(-h,  h,  h), normalBack);   // 2
    addVertex(QVector3D( h,  h,  h), normalBack);   // 3
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Left face (x = -h) - CCW winding when viewed from left
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h, -h,  h), normalLeft);   // 0
    addVertex(QVector3D(-h, -h, -h), normalLeft);   // 1
    addVertex(QVector3D(-h,  h, -h), normalLeft);   // 2
    addVertex(QVector3D(-h,  h,  h), normalLeft);   // 3
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Right face (x = +h) - CCW winding when viewed from right
    baseIdx = getVertexCount();
    addVertex(QVector3D( h, -h, -h), normalRight);  // 0
    addVertex(QVector3D( h, -h,  h), normalRight);  // 1
    addVertex(QVector3D( h,  h,  h), normalRight);  // 2
    addVertex(QVector3D( h,  h, -h), normalRight);  // 3
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Top face (y = +h) - CCW winding when viewed from top
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h,  h, -h), normalTop);    // 0
    addVertex(QVector3D( h,  h, -h), normalTop);    // 1
    addVertex(QVector3D( h,  h,  h), normalTop);    // 2
    addVertex(QVector3D(-h,  h,  h), normalTop);    // 3
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);

    // Bottom face (y = -h) - CCW winding when viewed from bottom
    baseIdx = getVertexCount();
    addVertex(QVector3D(-h, -h,  h), normalBottom); // 0
    addVertex(QVector3D( h, -h,  h), normalBottom); // 1
    addVertex(QVector3D( h, -h, -h), normalBottom); // 2
    addVertex(QVector3D(-h, -h, -h), normalBottom); // 3
    addQuad(baseIdx, baseIdx+1, baseIdx+2, baseIdx+3);
}
