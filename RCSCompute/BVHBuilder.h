// BVHBuilder.h - Bounding Volume Hierarchy construction
#pragma once

#include "RCSTypes.h"
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <cstdint>

namespace RCS {

class BVHBuilder {
public:
    BVHBuilder() = default;
    ~BVHBuilder() = default;

    // Build BVH from mesh data
    // vertices: interleaved [x,y,z,nx,ny,nz] per vertex (6 floats)
    // indices: triangle indices (3 per triangle)
    // transform: model matrix to apply to vertices
    void build(const std::vector<float>& vertices,
               const std::vector<uint32_t>& indices,
               const QMatrix4x4& transform);

    // Get results for GPU upload
    const std::vector<BVHNode>& getNodes() const { return nodes_; }
    const std::vector<Triangle>& getTriangles() const { return triangles_; }
    int getNodeCount() const { return static_cast<int>(nodes_.size()); }
    int getTriangleCount() const { return static_cast<int>(triangles_.size()); }

    // Debug info
    int getMaxDepth() const { return maxDepth_; }

private:
    std::vector<BVHNode> nodes_;
    std::vector<Triangle> triangles_;
    std::vector<AABB> triangleBounds_;
    std::vector<QVector3D> triangleCentroids_;
    int maxDepth_ = 0;

    // Recursive build using SAH (Surface Area Heuristic)
    int buildRecursive(std::vector<int>& triIndices, int start, int end, int depth);

    // Compute AABB for a range of triangles
    AABB computeBounds(const std::vector<int>& triIndices, int start, int end);

    // Find best split using SAH
    struct SplitResult {
        int axis;
        float position;
        float cost;
    };
    SplitResult findBestSplit(const std::vector<int>& triIndices, int start, int end, const AABB& bounds);
};

} // namespace RCS
