// RCSTypes.h - GPU-aligned data structures for RCS computation
#pragma once

#include <QVector4D>
#include <cstdint>

namespace RCS {

// Ray structure - 32 bytes, GPU cache-line aligned
struct alignas(16) Ray {
    QVector4D origin;      // xyz = origin, w = tmin
    QVector4D direction;   // xyz = direction (normalized), w = tmax
};

// BVH Node structure - 32 bytes
struct alignas(16) BVHNode {
    QVector4D boundsMin;   // xyz = AABB min, w = left child index (negative = leaf)
    QVector4D boundsMax;   // xyz = AABB max, w = right child (or triangle count for leaf)
};

// Triangle structure for GPU - 48 bytes (3 vertices, position only)
struct alignas(16) Triangle {
    QVector4D v0;          // xyz = vertex 0, w = unused
    QVector4D v1;          // xyz = vertex 1, w = unused
    QVector4D v2;          // xyz = vertex 2, w = unused
};

// Hit result structure - 32 bytes
struct alignas(16) HitResult {
    QVector4D hitPoint;    // xyz = world position, w = distance (-1 = miss)
    QVector4D normal;      // xyz = surface normal, w = material ID
    uint32_t triangleId;   // Index of hit triangle
    uint32_t rayId;        // Index of ray that produced this hit
    float rcsContribution; // RCS contribution (Phase 2)
    float pad;             // Alignment padding
};

// Axis-Aligned Bounding Box
struct AABB {
    QVector3D min;
    QVector3D max;

    AABB() : min(1e30f, 1e30f, 1e30f), max(-1e30f, -1e30f, -1e30f) {}

    void expand(const QVector3D& point) {
        min.setX(std::min(min.x(), point.x()));
        min.setY(std::min(min.y(), point.y()));
        min.setZ(std::min(min.z(), point.z()));
        max.setX(std::max(max.x(), point.x()));
        max.setY(std::max(max.y(), point.y()));
        max.setZ(std::max(max.z(), point.z()));
    }

    void expand(const AABB& other) {
        expand(other.min);
        expand(other.max);
    }

    QVector3D center() const {
        return (min + max) * 0.5f;
    }

    float surfaceArea() const {
        QVector3D d = max - min;
        return 2.0f * (d.x() * d.y() + d.y() * d.z() + d.z() * d.x());
    }
};

} // namespace RCS
