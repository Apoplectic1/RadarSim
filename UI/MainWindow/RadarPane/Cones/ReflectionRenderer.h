// ReflectionRenderer.h - Visualization of reflected radar lobes
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

// Aggregated reflection lobe for visualization
struct ReflectionLobe {
    QVector3D position;     // Center position on target surface
    QVector3D direction;    // Average reflection direction (normalized)
    float intensity;        // Average intensity (0-1)
    int hitCount;           // Number of hits aggregated into this lobe
};

class ReflectionRenderer : public QObject, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit ReflectionRenderer(QObject* parent = nullptr);
    ~ReflectionRenderer() override;

    // Lifecycle
    bool initialize();
    void cleanup();
    bool isInitialized() const { return initialized_; }

    // Update lobes from RCS compute results
    void updateLobes(const std::vector<RCS::HitResult>& hits);

    // Rendering
    void render(const QMatrix4x4& projection, const QMatrix4x4& view,
                const QMatrix4x4& model);

    // Configuration
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void setOpacity(float opacity) { opacity_ = opacity; }
    float getOpacity() const { return opacity_; }
    void setLobeScale(float scale) { lobeScale_ = scale; }
    float getLobeScale() const { return lobeScale_; }

    // Statistics
    int getLobeCount() const { return static_cast<int>(lobes_.size()); }

signals:
    void lobeCountChanged(int count);

private:
    bool initialized_ = false;
    bool visible_ = true;
    float opacity_ = 0.7f;
    float lobeScale_ = 1.0f;

    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> shaderProgram_;
    QOpenGLVertexArrayObject vao_;
    GLuint vboId_ = 0;
    GLuint eboId_ = 0;

    // Geometry data
    std::vector<float> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<ReflectionLobe> lobes_;
    bool geometryDirty_ = false;
    int vertexCount_ = 0;
    int indexCount_ = 0;

    // Shader sources
    std::string_view vertexShaderSource_;
    std::string_view fragmentShaderSource_;

    // Helper methods
    void setupShaders();
    std::vector<ReflectionLobe> clusterHits(const std::vector<RCS::HitResult>& hits);
    void generateLobeGeometry();
    void uploadGeometry();
    void generateConeGeometry(const ReflectionLobe& lobe);

    // Intensity to color conversion
    static QVector3D intensityToColor(float intensity);
};
