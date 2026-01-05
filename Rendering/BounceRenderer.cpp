// BounceRenderer.cpp - Implementation of bounce path visualization
#include "BounceRenderer.h"
#include "../Common/GLUtils.h"
#include "../Common/Constants.h"
#include "../UI/MainWindow/RCSPane/Compute/RCSTypes.h"
#include <QOpenGLContext>
#include <QDebug>
#include <cmath>

using namespace RS::Constants;

BounceRenderer::BounceRenderer(QObject* parent)
    : QObject(parent)
    , baseColor_(Colors::kBounceBaseColor[0],
                 Colors::kBounceBaseColor[1],
                 Colors::kBounceBaseColor[2])
{
    // Simple vertex shader for colored lines
    vertexShaderSource_ = R"(
        #version 430 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;

        uniform mat4 view;
        uniform mat4 projection;

        out vec3 Color;

        void main() {
            Color = aColor;
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";

    // Simple fragment shader - pass through color
    fragmentShaderSource_ = R"(
        #version 430 core
        in vec3 Color;
        out vec4 FragColor;

        void main() {
            FragColor = vec4(Color, 1.0);
        }
    )";
}

BounceRenderer::~BounceRenderer() {
    // OpenGL cleanup should be done via cleanup() before context destruction
}

bool BounceRenderer::initialize() {
    if (initialized_) {
        return true;
    }

    if (!QOpenGLContext::currentContext()) {
        qWarning() << "BounceRenderer::initialize - No OpenGL context available";
        return false;
    }

    if (!initializeOpenGLFunctions()) {
        qCritical() << "BounceRenderer: Failed to initialize OpenGL functions!";
        return false;
    }

    GLUtils::clearGLErrors();

    setupShaders();

    vao_.create();
    vao_.bind();
    vao_.release();

    GLUtils::checkGLError("BounceRenderer::initialize");

    initialized_ = true;
    return true;
}

void BounceRenderer::cleanup() {
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        shaderProgram_.reset();
        return;
    }

    if (vao_.isCreated()) {
        vao_.destroy();
    }
    if (vboId_ != 0) {
        glDeleteBuffers(1, &vboId_);
        vboId_ = 0;
    }
    shaderProgram_.reset();
    initialized_ = false;
}

void BounceRenderer::setupShaders() {
    if (vertexShaderSource_.empty() || fragmentShaderSource_.empty()) {
        qCritical() << "BounceRenderer: Shader sources not initialized!";
        return;
    }

    shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_.data())) {
        qCritical() << "BounceRenderer: Failed to compile vertex shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_.data())) {
        qCritical() << "BounceRenderer: Failed to compile fragment shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->link()) {
        qCritical() << "BounceRenderer: Failed to link shader program:" << shaderProgram_->log();
        return;
    }
}

QVector3D BounceRenderer::getBounceHitPoint(int index) const {
    if (index >= 0 && index < static_cast<int>(bounceHitPoints_.size())) {
        return bounceHitPoints_[index];
    }
    return QVector3D();
}

QVector3D BounceRenderer::getBounceColor(int bounceIndex, float intensity) const {
    // In Path mode, use uniform brightness (full intensity)
    float effectiveIntensity = (rayTraceMode_ == RCS::RayTraceMode::Path) ? 1.0f : intensity;

    // Clamp intensity to minimum
    effectiveIntensity = std::max(effectiveIntensity, kBounceMinIntensity);

    // Return base color scaled by intensity
    return baseColor_ * effectiveIntensity;
}

bool BounceRenderer::raySphereIntersect(const QVector3D& origin, const QVector3D& dir,
                                         float radius, float& t) const {
    // Sphere centered at origin
    // Solve: |origin + t*dir|^2 = radius^2
    float a = QVector3D::dotProduct(dir, dir);
    float b = 2.0f * QVector3D::dotProduct(origin, dir);
    float c = QVector3D::dotProduct(origin, origin) - radius * radius;

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0) {
        return false;
    }

    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    // We want the positive t (ray going outward)
    if (t1 > 0.001f) {
        t = t1;
        return true;
    }
    if (t2 > 0.001f) {
        t = t2;
        return true;
    }
    return false;
}

void BounceRenderer::setBounceData(const QVector3D& radarPos,
                                    const std::vector<RCS::HitResult>& bounces,
                                    const std::vector<RCS::BounceState>& states,
                                    float sphereRadius) {
    radarPos_ = radarPos;
    sphereRadius_ = sphereRadius;
    bounceHitPoints_.clear();
    bounceIntensities_.clear();
    totalPathLength_ = 0.0f;
    vertices_.clear();
    lineSegments_.clear();

    if (bounces.empty()) {
        // No hits - draw miss ray toward center
        QVector3D missColor(Colors::kBounceMissColor[0],
                           Colors::kBounceMissColor[1],
                           Colors::kBounceMissColor[2]);
        QVector3D missDir = -radarPos.normalized();  // Toward origin
        QVector3D missEnd = radarPos + missDir * sphereRadius * 2.0f;
        addLineSegment(radarPos, missEnd, missColor);
        vertexCount_ = static_cast<int>(vertices_.size() / 6);
        geometryDirty_ = true;
        return;
    }

    // Draw each bounce segment with intensity-based coloring
    QVector3D segmentStart = radarPos;

    for (size_t i = 0; i < bounces.size(); ++i) {
        const RCS::HitResult& hit = bounces[i];
        QVector3D hitPos = hit.hitPoint.toVector3D();

        // Get intensity from state (default to 1.0 if states don't match)
        float intensity = 1.0f;
        if (i < states.size()) {
            intensity = states[i].intensity;
        } else if (!states.empty()) {
            // Apply decay manually if states not provided
            intensity = 1.0f - (kBounceIntensityDecay * static_cast<float>(i));
            intensity = std::max(intensity, kBounceMinIntensity);
        }

        QVector3D color = getBounceColor(static_cast<int>(i), intensity);

        // Draw segment from previous point to this hit
        addLineSegment(segmentStart, hitPos, color);

        // Add hit marker
        QVector3D markerColor(Colors::kBounceHitMarkerColor[0],
                             Colors::kBounceHitMarkerColor[1],
                             Colors::kBounceHitMarkerColor[2]);
        addHitMarker(hitPos, kBounceHitMarkerSize, markerColor);

        // Track path length
        totalPathLength_ += (hitPos - segmentStart).length();

        // Store hit point and intensity for accessors
        bounceHitPoints_.push_back(hitPos);
        bounceIntensities_.push_back(intensity);

        // Prepare for next segment
        segmentStart = hitPos;
    }

    // Extend final ray to sphere surface
    QVector3D lastHitPos = bounces.back().hitPoint.toVector3D();
    QVector3D lastReflection = bounces.back().reflection.toVector3D().normalized();

    QVector3D finalColor(Colors::kBounceFinalRayColor[0],
                        Colors::kBounceFinalRayColor[1],
                        Colors::kBounceFinalRayColor[2]);

    float tSphere;
    if (raySphereIntersect(lastHitPos, lastReflection, sphereRadius, tSphere)) {
        QVector3D sphereHit = lastHitPos + lastReflection * tSphere;
        addLineSegment(lastHitPos, sphereHit, finalColor);
        totalPathLength_ += tSphere;
    } else {
        // If no sphere intersection, extend a fixed length
        QVector3D extendedEnd = lastHitPos + lastReflection * 50.0f;
        addLineSegment(lastHitPos, extendedEnd, finalColor);
    }

    vertexCount_ = static_cast<int>(vertices_.size() / 6);
    geometryDirty_ = true;
}

void BounceRenderer::setBounceData(const QVector3D& radarPos,
                                    const std::vector<RCS::HitResult>& bounces,
                                    float sphereRadius) {
    // Create default states with decay
    std::vector<RCS::BounceState> states;
    states.reserve(bounces.size());

    for (size_t i = 0; i < bounces.size(); ++i) {
        RCS::BounceState state;
        state.bounceCount = static_cast<int>(i);

        if (rayTraceMode_ == RCS::RayTraceMode::Path) {
            state.intensity = 1.0f;  // Uniform brightness in Path mode
        } else {
            // Apply decay per bounce in Physics mode
            state.intensity = 1.0f - (kBounceIntensityDecay * static_cast<float>(i));
            state.intensity = std::max(state.intensity, kBounceMinIntensity);
        }

        states.push_back(state);
    }

    setBounceData(radarPos, bounces, states, sphereRadius);
}

void BounceRenderer::clearBounceData() {
    vertices_.clear();
    lineSegments_.clear();
    vertexCount_ = 0;
    bounceHitPoints_.clear();
    bounceIntensities_.clear();
    totalPathLength_ = 0.0f;
    geometryDirty_ = true;
}

void BounceRenderer::addLineSegment(const QVector3D& start, const QVector3D& end,
                                     const QVector3D& color) {
    // Add line vertices directly (position + color)
    vertices_.push_back(start.x());
    vertices_.push_back(start.y());
    vertices_.push_back(start.z());
    vertices_.push_back(color.x());
    vertices_.push_back(color.y());
    vertices_.push_back(color.z());

    vertices_.push_back(end.x());
    vertices_.push_back(end.y());
    vertices_.push_back(end.z());
    vertices_.push_back(color.x());
    vertices_.push_back(color.y());
    vertices_.push_back(color.z());
}

void BounceRenderer::addHitMarker(const QVector3D& position, float size,
                                   const QVector3D& color) {
    // Draw a 3D cross at the hit point
    float halfSize = size * 0.5f;

    // X axis line
    addLineSegment(position - QVector3D(halfSize, 0, 0),
                   position + QVector3D(halfSize, 0, 0), color);

    // Y axis line
    addLineSegment(position - QVector3D(0, halfSize, 0),
                   position + QVector3D(0, halfSize, 0), color);

    // Z axis line
    addLineSegment(position - QVector3D(0, 0, halfSize),
                   position + QVector3D(0, 0, halfSize), color);
}

void BounceRenderer::generateGeometry() {
    // Geometry is generated in setBounceData
    // This method is kept for consistency with the interface
}

void BounceRenderer::uploadGeometry() {
    if (!QOpenGLContext::currentContext() || !vao_.isCreated()) {
        geometryDirty_ = true;
        return;
    }

    if (vertices_.empty()) {
        vertexCount_ = 0;
        return;
    }

    vao_.bind();

    // VBO
    if (vboId_ == 0) {
        glGenBuffers(1, &vboId_);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices_.size() * sizeof(float),
                 vertices_.data(),
                 GL_DYNAMIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    vao_.release();
    geometryDirty_ = false;
}

void BounceRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view,
                            const QVector3D& cameraPos) {
    (void)cameraPos;  // Not used in GL_LINES mode

    if (!visible_ || vertices_.empty() || !shaderProgram_) {
        return;
    }

    if (!vao_.isCreated()) {
        return;
    }

    if (geometryDirty_) {
        uploadGeometry();
    }

    if (vboId_ == 0 || vertexCount_ == 0) {
        return;
    }

    // Enable depth test so rays are occluded by solid geometry (target)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Set line width for visibility
    glLineWidth(kBounceLineWidth);

    shaderProgram_->bind();
    shaderProgram_->setUniformValue("projection", projection);
    shaderProgram_->setUniformValue("view", view);

    vao_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);

    // Draw lines
    glDrawArrays(GL_LINES, 0, vertexCount_);

    vao_.release();
    shaderProgram_->release();

    // Restore state
    glLineWidth(1.0f);
}
