// HeatMapRenderer.h - RCS heat map visualization on radar sphere
#pragma once

#include <QObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <vector>
#include <memory>
#include <string_view>

#include "RCSTypes.h"
#include "RCSSampler.h"  // For CutType enum

class HeatMapRenderer : public QObject, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit HeatMapRenderer(QObject* parent = nullptr);
    ~HeatMapRenderer() override;

    // Lifecycle
    bool initialize();
    void cleanup();
    bool isInitialized() const { return initialized_; }

    // Update heat map from RCS hit results
    void updateFromHits(const std::vector<RCS::HitResult>& hits, float sphereRadius);

    // Rendering
    void render(const QMatrix4x4& projection, const QMatrix4x4& view,
                const QMatrix4x4& model);

    // Configuration
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void setOpacity(float opacity) { opacity_ = opacity; }
    float getOpacity() const { return opacity_; }
    void setMinIntensityThreshold(float threshold) { minIntensity_ = threshold; }
    float getMinIntensityThreshold() const { return minIntensity_; }
    void setSphereRadius(float radius);

    // Slice filtering (to match polar plot)
    void setSliceParameters(CutType type, float offset, float thickness);
    CutType getCutType() const { return cutType_; }
    float getSliceOffset() const { return sliceOffset_; }
    float getSliceThickness() const { return sliceThickness_; }

signals:
    void visibilityChanged(bool visible);

private:
    bool initialized_ = false;
    bool visible_ = true;
    float opacity_ = 0.7f;
    float minIntensity_ = 0.05f;
    float sphereRadius_ = 100.0f;

    // Slice filtering parameters
    CutType cutType_ = CutType::Azimuth;
    float sliceOffset_ = 0.0f;      // Degrees
    float sliceThickness_ = 10.0f;  // Degrees (+/-)

    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> shaderProgram_;
    QOpenGLVertexArrayObject vao_;
    GLuint vboId_ = 0;
    GLuint eboId_ = 0;

    // Geometry data - sphere mesh
    std::vector<float> vertices_;        // Position + Normal (6 floats per vertex)
    std::vector<unsigned int> indices_;
    std::vector<float> intensities_;     // Per-vertex intensity (updated each frame)
    int vertexCount_ = 0;
    int indexCount_ = 0;
    bool geometryDirty_ = true;

    // Spherical binning for intensity accumulation
    int latBins_;
    int lonBins_;
    std::vector<float> binIntensity_;
    std::vector<int> binHitCount_;

    // Shader sources
    std::string_view vertexShaderSource_;
    std::string_view fragmentShaderSource_;

    // Helper methods
    void setupShaders();
    void generateSphereMesh();
    void uploadGeometry();
    void uploadIntensities();
    void clearBins();
    void accumulateHit(const RCS::HitResult& hit);
    void computeVertexIntensities();
    int getBinIndex(float theta, float phi) const;
    void getVertexSphericalCoords(int vertexIndex, float& theta, float& phi) const;
    bool isHitInSlice(const RCS::HitResult& hit) const;

    // Intensity to color conversion (same gradient as ReflectionRenderer)
    static QVector3D intensityToColor(float intensity);
};
