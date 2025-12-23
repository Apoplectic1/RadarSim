// RCSCompute.h - GPU ray tracing for RCS calculations
#pragma once

#include <QObject>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <memory>

#include "RCSTypes.h"
#include "BVHBuilder.h"

namespace RCS {

class RCSCompute : public QObject, protected QOpenGLFunctions_4_3_Core {
    Q_OBJECT

public:
    explicit RCSCompute(QObject* parent = nullptr);
    ~RCSCompute() override;

    // Lifecycle
    bool initialize();
    void cleanup();
    bool isInitialized() const { return initialized_; }

    // Geometry management
    void setTargetGeometry(const std::vector<float>& vertices,
                           const std::vector<uint32_t>& indices,
                           const QMatrix4x4& modelMatrix);

    // Beam configuration
    void setRadarPosition(const QVector3D& position);
    void setBeamDirection(const QVector3D& direction);
    void setBeamWidth(float widthDegrees);
    void setSphereRadius(float radius);

    // Ray tracing
    void compute();

    // Results
    int getHitCount() const { return hitCount_; }
    float getOcclusionRatio() const;

    // Debug
    void setNumRays(int numRays);
    int getNumRays() const { return numRays_; }

signals:
    void computeComplete(int hitCount, float occlusionRatio);

private:
    bool initialized_ = false;

    // OpenGL resources
    GLuint rayBuffer_ = 0;       // SSBO for rays
    GLuint bvhBuffer_ = 0;       // SSBO for BVH nodes
    GLuint triangleBuffer_ = 0;  // SSBO for triangles
    GLuint hitBuffer_ = 0;       // SSBO for hit results
    GLuint counterBuffer_ = 0;   // Atomic counter for hit count

    // Compute shaders
    std::unique_ptr<QOpenGLShaderProgram> rayGenShader_;
    std::unique_ptr<QOpenGLShaderProgram> traceShader_;

    // BVH
    BVHBuilder bvhBuilder_;
    bool bvhDirty_ = true;

    // Configuration
    QVector3D radarPosition_;
    QVector3D beamDirection_{0, 0, -1};
    float beamWidthDegrees_ = 15.0f;
    float sphereRadius_ = 100.0f;
    int numRays_ = 10000;

    // Results
    int hitCount_ = 0;

    // Shader source
    bool compileShaders();
    void createBuffers();
    void uploadBVH();
    void dispatchRayGeneration();
    void dispatchTracing();
    void readResults();
};

} // namespace RCS
