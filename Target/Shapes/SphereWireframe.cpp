// SphereWireframe.cpp
// Geodesic sphere using icosahedron subdivision

#include "SphereWireframe.h"
#include <cmath>
#include <map>

SphereWireframe::SphereWireframe(int subdivisions)
    : WireframeTarget(),
      subdivisions_(subdivisions)
{
}

void SphereWireframe::setSubdivisions(int level) {
    if (level != subdivisions_) {
        subdivisions_ = level;
        generateGeometry();
        geometryDirty_ = true;
    }
}

void SphereWireframe::generateGeometry() {
    clearGeometry();
    createIcosahedron();
    subdivide(subdivisions_);

    // Detect crease edges - for spheres, all faces are ~coplanar at high subdivision
    // so no edges should be detected (smooth surface)
    detectEdges();
    generateEdgeGeometry();
}

void SphereWireframe::createIcosahedron() {
    // Golden ratio for icosahedron vertex positions
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
    const float a = 1.0f;
    const float b = 1.0f / phi;

    // 12 vertices of icosahedron (normalized to unit sphere)
    // Arranged as 3 orthogonal golden rectangles
    std::vector<QVector3D> verts = {
        QVector3D(-b,  a,  0).normalized(),
        QVector3D( b,  a,  0).normalized(),
        QVector3D(-b, -a,  0).normalized(),
        QVector3D( b, -a,  0).normalized(),
        QVector3D( 0, -b,  a).normalized(),
        QVector3D( 0,  b,  a).normalized(),
        QVector3D( 0, -b, -a).normalized(),
        QVector3D( 0,  b, -a).normalized(),
        QVector3D( a,  0, -b).normalized(),
        QVector3D( a,  0,  b).normalized(),
        QVector3D(-a,  0, -b).normalized(),
        QVector3D(-a,  0,  b).normalized()
    };

    // Add vertices with their position as the normal (for a sphere, normal = position)
    for (const auto& v : verts) {
        addVertex(v, v);
    }

    // 20 triangular faces of icosahedron (CCW winding when viewed from outside)
    // 5 faces around vertex 0 (top)
    addTriangle(0, 11, 5);
    addTriangle(0, 5, 1);
    addTriangle(0, 1, 7);
    addTriangle(0, 7, 10);
    addTriangle(0, 10, 11);

    // 5 adjacent faces
    addTriangle(1, 5, 9);
    addTriangle(5, 11, 4);
    addTriangle(11, 10, 2);
    addTriangle(10, 7, 6);
    addTriangle(7, 1, 8);

    // 5 faces around vertex 3 (bottom)
    addTriangle(3, 9, 4);
    addTriangle(3, 4, 2);
    addTriangle(3, 2, 6);
    addTriangle(3, 6, 8);
    addTriangle(3, 8, 9);

    // 5 adjacent faces
    addTriangle(4, 9, 5);
    addTriangle(2, 4, 11);
    addTriangle(6, 2, 10);
    addTriangle(8, 6, 7);
    addTriangle(9, 8, 1);
}

QVector3D SphereWireframe::getMidpoint(const QVector3D& v1, const QVector3D& v2) {
    // Get midpoint and normalize to sphere surface
    return ((v1 + v2) * 0.5f).normalized();
}

void SphereWireframe::subdivide(int levels) {
    if (levels <= 0) {
        return;
    }

    for (int level = 0; level < levels; ++level) {
        // Store current triangles
        std::vector<GLuint> oldIndices = indices_;
        indices_.clear();

        // Cache for midpoint vertices to avoid duplicates
        // Key: pair of vertex indices (smaller first), Value: new vertex index
        std::map<std::pair<GLuint, GLuint>, GLuint> midpointCache;

        auto getOrCreateMidpoint = [&](GLuint i1, GLuint i2) -> GLuint {
            // Ensure consistent ordering for cache key
            if (i1 > i2) std::swap(i1, i2);
            auto key = std::make_pair(i1, i2);

            auto it = midpointCache.find(key);
            if (it != midpointCache.end()) {
                return it->second;
            }

            // Get vertex positions from vertices_ array (6 floats per vertex)
            QVector3D v1(vertices_[i1 * 6], vertices_[i1 * 6 + 1], vertices_[i1 * 6 + 2]);
            QVector3D v2(vertices_[i2 * 6], vertices_[i2 * 6 + 1], vertices_[i2 * 6 + 2]);

            // Create midpoint on sphere surface
            QVector3D mid = getMidpoint(v1, v2);

            // Add new vertex (position = normal for sphere)
            GLuint newIdx = getVertexCount();
            addVertex(mid, mid);

            midpointCache[key] = newIdx;
            return newIdx;
        };

        // Process each triangle
        for (size_t i = 0; i < oldIndices.size(); i += 3) {
            GLuint v0 = oldIndices[i];
            GLuint v1 = oldIndices[i + 1];
            GLuint v2 = oldIndices[i + 2];

            // Get midpoints of each edge
            GLuint m01 = getOrCreateMidpoint(v0, v1);
            GLuint m12 = getOrCreateMidpoint(v1, v2);
            GLuint m20 = getOrCreateMidpoint(v2, v0);

            // Create 4 new triangles from the original
            //       v0
            //      /  \
            //    m01--m20
            //    / \  / \
            //  v1--m12--v2
            addTriangle(v0, m01, m20);
            addTriangle(m01, v1, m12);
            addTriangle(m20, m12, v2);
            addTriangle(m01, m12, m20);
        }
    }
}
