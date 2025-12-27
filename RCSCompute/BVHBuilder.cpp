// BVHBuilder.cpp - BVH construction using Surface Area Heuristic
#include "BVHBuilder.h"
#include <algorithm>
#include <limits>
#include <QDebug>

namespace RCS {

void BVHBuilder::build(const std::vector<float>& vertices,
                       const std::vector<uint32_t>& indices,
                       const QMatrix4x4& transform) {
    nodes_.clear();
    triangles_.clear();
    triangleBounds_.clear();
    triangleCentroids_.clear();
    maxDepth_ = 0;

    if (indices.empty()) {
        return;
    }

    int numTriangles = static_cast<int>(indices.size()) / 3;
    triangles_.reserve(numTriangles);
    triangleBounds_.reserve(numTriangles);
    triangleCentroids_.reserve(numTriangles);

    // Extract and transform triangles
    for (int t = 0; t < numTriangles; t++) {
        uint32_t i0 = indices[t * 3 + 0];
        uint32_t i1 = indices[t * 3 + 1];
        uint32_t i2 = indices[t * 3 + 2];

        // Get positions (skip normals - stride of 6 floats)
        QVector3D v0(vertices[i0 * 6 + 0], vertices[i0 * 6 + 1], vertices[i0 * 6 + 2]);
        QVector3D v1(vertices[i1 * 6 + 0], vertices[i1 * 6 + 1], vertices[i1 * 6 + 2]);
        QVector3D v2(vertices[i2 * 6 + 0], vertices[i2 * 6 + 1], vertices[i2 * 6 + 2]);

        // Apply transform
        v0 = transform.map(v0);
        v1 = transform.map(v1);
        v2 = transform.map(v2);

        // Store triangle
        Triangle tri;
        tri.v0 = QVector4D(v0, 0.0f);
        tri.v1 = QVector4D(v1, 0.0f);
        tri.v2 = QVector4D(v2, 0.0f);
        triangles_.push_back(tri);

        // Compute bounds
        AABB bounds;
        bounds.expand(v0);
        bounds.expand(v1);
        bounds.expand(v2);
        triangleBounds_.push_back(bounds);

        // Compute centroid
        triangleCentroids_.push_back((v0 + v1 + v2) / 3.0f);
    }

    // Create list of triangle indices for sorting
    std::vector<int> triIndices(numTriangles);
    for (int i = 0; i < numTriangles; i++) {
        triIndices[i] = i;
    }

    // Reserve space for nodes (worst case: 2N-1 for N triangles)
    nodes_.reserve(2 * numTriangles);

    // Build recursively
    buildRecursive(triIndices, 0, numTriangles, 0);

    // IMPORTANT: Reorder triangles to match the BVH's sorted order
    // The BVH leaf nodes store indices into triIndices (which got partitioned/sorted)
    // The GPU shader expects triangles in this sorted order
    std::vector<Triangle> sortedTriangles(numTriangles);
    std::vector<AABB> sortedBounds(numTriangles);
    for (int i = 0; i < numTriangles; i++) {
        sortedTriangles[i] = triangles_[triIndices[i]];
        sortedBounds[i] = triangleBounds_[triIndices[i]];
    }
    triangles_ = std::move(sortedTriangles);
    triangleBounds_ = std::move(sortedBounds);
}

int BVHBuilder::buildRecursive(std::vector<int>& triIndices, int start, int end, int depth) {
    maxDepth_ = std::max(maxDepth_, depth);
    int nodeIndex = static_cast<int>(nodes_.size());
    nodes_.push_back(BVHNode());

    AABB bounds = computeBounds(triIndices, start, end);
    int count = end - start;

    // Leaf node threshold
    const int maxLeafSize = 4;

    if (count <= maxLeafSize) {
        // Create leaf node
        // Store negative index to indicate leaf, and triangle count
        nodes_[nodeIndex].boundsMin = QVector4D(bounds.min, static_cast<float>(-(start + 1)));
        nodes_[nodeIndex].boundsMax = QVector4D(bounds.max, static_cast<float>(count));
        return nodeIndex;
    }

    // Find best split using SAH
    SplitResult split = findBestSplit(triIndices, start, end, bounds);

    // Partition triangles
    auto midIt = std::partition(triIndices.begin() + start, triIndices.begin() + end,
        [this, &split](int triIdx) {
            return triangleCentroids_[triIdx][split.axis] < split.position;
        });
    int mid = static_cast<int>(midIt - triIndices.begin());

    // Fallback if partition failed (all on one side)
    if (mid == start || mid == end) {
        mid = (start + end) / 2;
    }

    // Build children
    int leftChild = buildRecursive(triIndices, start, mid, depth + 1);
    int rightChild = buildRecursive(triIndices, mid, end, depth + 1);

    // Store internal node
    nodes_[nodeIndex].boundsMin = QVector4D(bounds.min, static_cast<float>(leftChild));
    nodes_[nodeIndex].boundsMax = QVector4D(bounds.max, static_cast<float>(rightChild));

    return nodeIndex;
}

AABB BVHBuilder::computeBounds(const std::vector<int>& triIndices, int start, int end) {
    AABB bounds;
    for (int i = start; i < end; i++) {
        bounds.expand(triangleBounds_[triIndices[i]]);
    }
    return bounds;
}

BVHBuilder::SplitResult BVHBuilder::findBestSplit(const std::vector<int>& triIndices,
                                                   int start, int end,
                                                   const AABB& bounds) {
    SplitResult best;
    best.cost = std::numeric_limits<float>::max();
    best.axis = 0;
    best.position = 0.0f;

    const int numBins = 12;
    float parentArea = bounds.surfaceArea();

    for (int axis = 0; axis < 3; axis++) {
        float axisMin = bounds.min[axis];
        float axisMax = bounds.max[axis];
        if (axisMax - axisMin < 1e-6f) continue;

        // Binning
        struct Bin {
            AABB bounds;
            int count = 0;
        };
        std::vector<Bin> bins(numBins);

        float scale = static_cast<float>(numBins) / (axisMax - axisMin);

        for (int i = start; i < end; i++) {
            int triIdx = triIndices[i];
            float centroid = triangleCentroids_[triIdx][axis];
            int binIdx = std::min(static_cast<int>((centroid - axisMin) * scale), numBins - 1);
            bins[binIdx].bounds.expand(triangleBounds_[triIdx]);
            bins[binIdx].count++;
        }

        // Sweep from left to right, accumulating bounds and counts
        std::vector<float> leftArea(numBins), rightArea(numBins);
        std::vector<int> leftCount(numBins), rightCount(numBins);

        AABB leftBounds, rightBounds;
        int leftN = 0, rightN = 0;

        for (int i = 0; i < numBins; i++) {
            leftN += bins[i].count;
            leftCount[i] = leftN;
            leftBounds.expand(bins[i].bounds);
            leftArea[i] = leftBounds.surfaceArea();

            int j = numBins - 1 - i;
            rightN += bins[j].count;
            rightCount[j] = rightN;
            rightBounds.expand(bins[j].bounds);
            rightArea[j] = rightBounds.surfaceArea();
        }

        // Find best split for this axis
        for (int i = 0; i < numBins - 1; i++) {
            if (leftCount[i] == 0 || rightCount[i + 1] == 0) continue;

            float cost = 1.0f + (leftCount[i] * leftArea[i] + rightCount[i + 1] * rightArea[i + 1]) / parentArea;
            if (cost < best.cost) {
                best.cost = cost;
                best.axis = axis;
                best.position = axisMin + (i + 1) * (axisMax - axisMin) / numBins;
            }
        }
    }

    return best;
}

} // namespace RCS
