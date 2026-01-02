// ReflectionRenderer.cpp - Implementation of reflection lobe visualization
#include "ReflectionRenderer.h"
#include "GLUtils.h"
#include "Constants.h"
#include <QOpenGLContext>
#include <QDebug>
#include <cmath>
#include <algorithm>

using namespace RS::Constants;

ReflectionRenderer::ReflectionRenderer(QObject* parent)
    : QObject(parent)
{
    // Vertex shader for lobe rendering
    vertexShaderSource_ = R"(
        #version 430 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec3 aColor;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec3 Color;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            Color = aColor;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    // Fragment shader with intensity-based coloring
    fragmentShaderSource_ = R"(
        #version 430 core
        in vec3 FragPos;
        in vec3 Normal;
        in vec3 Color;

        uniform vec3 viewPos;
        uniform float opacity;

        out vec4 FragColor;

        void main() {
            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);

            // Simple lighting
            float ambient = 0.4;
            float diffuse = max(dot(norm, viewDir), 0.0) * 0.6;
            float lighting = ambient + diffuse;

            // Fresnel for edge glow
            float fresnel = 0.4 + 0.6 * pow(1.0 - abs(dot(norm, viewDir)), 2.0);

            vec3 result = Color * lighting;
            FragColor = vec4(result, opacity * fresnel);
        }
    )";
}

ReflectionRenderer::~ReflectionRenderer() {
    // OpenGL cleanup should be done via cleanup() before context destruction
}

bool ReflectionRenderer::initialize() {
    if (initialized_) {
        return true;
    }

    if (!QOpenGLContext::currentContext()) {
        qWarning() << "ReflectionRenderer::initialize - No OpenGL context available";
        return false;
    }

    if (!initializeOpenGLFunctions()) {
        qCritical() << "ReflectionRenderer: Failed to initialize OpenGL functions!";
        return false;
    }

    GLUtils::clearGLErrors();

    setupShaders();

    vao_.create();
    vao_.bind();
    vao_.release();

    GLUtils::checkGLError("ReflectionRenderer::initialize");

    initialized_ = true;
    return true;
}

void ReflectionRenderer::cleanup() {
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
    if (eboId_ != 0) {
        glDeleteBuffers(1, &eboId_);
        eboId_ = 0;
    }
    shaderProgram_.reset();
    initialized_ = false;
}

void ReflectionRenderer::setupShaders() {
    if (vertexShaderSource_.empty() || fragmentShaderSource_.empty()) {
        qCritical() << "ReflectionRenderer: Shader sources not initialized!";
        return;
    }

    shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_.data())) {
        qCritical() << "ReflectionRenderer: Failed to compile vertex shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_.data())) {
        qCritical() << "ReflectionRenderer: Failed to compile fragment shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->link()) {
        qCritical() << "ReflectionRenderer: Failed to link shader program:" << shaderProgram_->log();
        return;
    }
}

void ReflectionRenderer::updateLobes(const std::vector<RCS::HitResult>& hits) {
    lobes_ = clusterHits(hits);
    generateLobeGeometry();
    geometryDirty_ = true;
    emit lobeCountChanged(static_cast<int>(lobes_.size()));
}

std::vector<ReflectionLobe> ReflectionRenderer::clusterHits(const std::vector<RCS::HitResult>& hits) {
    std::vector<ReflectionLobe> result;
    std::vector<bool> assigned(hits.size(), false);

    float clusterAngleCos = std::cos(kLobeClusterAngle * kDegToRadF);
    float clusterDistSq = kLobeClusterDist * kLobeClusterDist;

    for (size_t i = 0; i < hits.size() && result.size() < static_cast<size_t>(kMaxReflectionLobes); ++i) {
        const auto& hit = hits[i];

        // Skip misses and already assigned hits
        if (hit.hitPoint.w() < 0.0f || assigned[i]) {
            continue;
        }

        // Skip low intensity hits
        float intensity = hit.reflection.w();
        if (intensity < kLobeMinIntensity) {
            continue;
        }

        // Start new cluster with this hit
        ReflectionLobe lobe;
        lobe.position = hit.hitPoint.toVector3D();
        lobe.direction = hit.reflection.toVector3D().normalized();
        lobe.intensity = intensity;
        lobe.hitCount = 1;
        assigned[i] = true;

        // Find nearby hits with similar reflection directions
        for (size_t j = i + 1; j < hits.size(); ++j) {
            if (assigned[j] || hits[j].hitPoint.w() < 0.0f) {
                continue;
            }

            QVector3D pos = hits[j].hitPoint.toVector3D();
            QVector3D dir = hits[j].reflection.toVector3D().normalized();
            float hitIntensity = hits[j].reflection.w();

            // Skip low intensity hits in clustering too
            if (hitIntensity < kLobeMinIntensity) {
                continue;
            }

            // Check spatial distance
            float distSq = (pos - lobe.position).lengthSquared();
            if (distSq > clusterDistSq) {
                continue;
            }

            // Check angular similarity
            float dirDot = QVector3D::dotProduct(dir, lobe.direction);
            if (dirDot < clusterAngleCos) {
                continue;
            }

            // Add to cluster with weighted average
            float w = 1.0f / (lobe.hitCount + 1);
            lobe.position = lobe.position * (1.0f - w) + pos * w;
            lobe.direction = (lobe.direction * (1.0f - w) + dir * w).normalized();
            lobe.intensity = lobe.intensity * (1.0f - w) + hitIntensity * w;
            lobe.hitCount++;
            assigned[j] = true;
        }

        result.push_back(lobe);
    }

    // Sort by intensity (highest first)
    std::sort(result.begin(), result.end(),
              [](const ReflectionLobe& a, const ReflectionLobe& b) {
                  return a.intensity > b.intensity;
              });

    return result;
}

void ReflectionRenderer::generateLobeGeometry() {
    vertices_.clear();
    indices_.clear();

    for (const auto& lobe : lobes_) {
        generateConeGeometry(lobe);
    }

    vertexCount_ = static_cast<int>(vertices_.size() / 9);  // 9 floats per vertex (pos + normal + color)
    indexCount_ = static_cast<int>(indices_.size());
}

void ReflectionRenderer::generateConeGeometry(const ReflectionLobe& lobe) {
    // Cone parameters scaled by intensity (min + range * intensity)
    float lengthScale = kLobeScaleLengthMin + (1.0f - kLobeScaleLengthMin) * lobe.intensity;
    float radiusScale = kLobeScaleRadiusMin + (1.0f - kLobeScaleRadiusMin) * lobe.intensity;
    float length = kLobeConeLength * lobeScale_ * lengthScale;
    float radius = kLobeConeRadius * lobeScale_ * radiusScale;
    int segments = kLobeConeSegments;

    QVector3D apex = lobe.position;
    QVector3D dir = lobe.direction.normalized();
    QVector3D baseCenter = apex + dir * length;

    // Get color based on intensity
    QVector3D color = intensityToColor(lobe.intensity);

    // Find perpendicular vectors for cone base
    QVector3D up(0.0f, 0.0f, 1.0f);
    if (std::abs(QVector3D::dotProduct(dir, up)) > kGimbalLockThreshold) {
        up = QVector3D(1.0f, 0.0f, 0.0f);
    }
    QVector3D right = QVector3D::crossProduct(dir, up).normalized();
    up = QVector3D::crossProduct(right, dir).normalized();

    // Base index for this cone
    unsigned int baseIdx = static_cast<unsigned int>(vertices_.size() / 9);

    // Apex vertex
    vertices_.push_back(apex.x());
    vertices_.push_back(apex.y());
    vertices_.push_back(apex.z());
    vertices_.push_back(dir.x());  // Normal points along cone
    vertices_.push_back(dir.y());
    vertices_.push_back(dir.z());
    vertices_.push_back(color.x());
    vertices_.push_back(color.y());
    vertices_.push_back(color.z());

    // Base ring vertices
    for (int i = 0; i < segments; ++i) {
        float angle = kTwoPiF * static_cast<float>(i) / static_cast<float>(segments);
        QVector3D offset = (right * std::cos(angle) + up * std::sin(angle)) * radius;
        QVector3D pos = baseCenter + offset;
        QVector3D normal = offset.normalized();

        vertices_.push_back(pos.x());
        vertices_.push_back(pos.y());
        vertices_.push_back(pos.z());
        vertices_.push_back(normal.x());
        vertices_.push_back(normal.y());
        vertices_.push_back(normal.z());
        vertices_.push_back(color.x());
        vertices_.push_back(color.y());
        vertices_.push_back(color.z());
    }

    // Generate indices for cone faces
    for (int i = 0; i < segments; ++i) {
        unsigned int curr = baseIdx + 1 + static_cast<unsigned int>(i);
        unsigned int next = baseIdx + 1 + static_cast<unsigned int>((i + 1) % segments);
        indices_.push_back(baseIdx);  // Apex
        indices_.push_back(curr);
        indices_.push_back(next);
    }

    // Base cap (optional - for closed cones)
    unsigned int baseCenterIdx = static_cast<unsigned int>(vertices_.size() / 9);
    vertices_.push_back(baseCenter.x());
    vertices_.push_back(baseCenter.y());
    vertices_.push_back(baseCenter.z());
    vertices_.push_back(dir.x());
    vertices_.push_back(dir.y());
    vertices_.push_back(dir.z());
    vertices_.push_back(color.x());
    vertices_.push_back(color.y());
    vertices_.push_back(color.z());

    for (int i = 0; i < segments; ++i) {
        unsigned int curr = baseIdx + 1 + static_cast<unsigned int>(i);
        unsigned int next = baseIdx + 1 + static_cast<unsigned int>((i + 1) % segments);
        indices_.push_back(baseCenterIdx);
        indices_.push_back(next);
        indices_.push_back(curr);
    }
}

QVector3D ReflectionRenderer::intensityToColor(float intensity) {
    // Blue (low) -> Yellow (mid) -> Red (high)
    using namespace Colors;
    if (intensity < kLobeColorThreshold) {
        float t = intensity / kLobeColorThreshold;
        return QVector3D(
            kLobeLowIntensity[0] * (1.0f - t) + kLobeMidIntensity[0] * t,
            kLobeLowIntensity[1] * (1.0f - t) + kLobeMidIntensity[1] * t,
            kLobeLowIntensity[2] * (1.0f - t) + kLobeMidIntensity[2] * t
        );
    } else {
        float t = (intensity - kLobeColorThreshold) / (1.0f - kLobeColorThreshold);
        return QVector3D(
            kLobeMidIntensity[0] * (1.0f - t) + kLobeHighIntensity[0] * t,
            kLobeMidIntensity[1] * (1.0f - t) + kLobeHighIntensity[1] * t,
            kLobeMidIntensity[2] * (1.0f - t) + kLobeHighIntensity[2] * t
        );
    }
}

void ReflectionRenderer::uploadGeometry() {
    if (!QOpenGLContext::currentContext() || !vao_.isCreated()) {
        geometryDirty_ = true;
        return;
    }

    if (vertices_.empty() || indices_.empty()) {
        vertexCount_ = 0;
        indexCount_ = 0;
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Color attribute (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // EBO
    if (eboId_ == 0) {
        glGenBuffers(1, &eboId_);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices_.size() * sizeof(unsigned int),
                 indices_.data(),
                 GL_DYNAMIC_DRAW);

    vao_.release();
    geometryDirty_ = false;
}

void ReflectionRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view,
                                 const QMatrix4x4& model) {
    if (!visible_ || lobes_.empty() || !shaderProgram_) {
        return;
    }

    if (!vao_.isCreated()) {
        return;
    }

    if (geometryDirty_) {
        uploadGeometry();
    }

    if (vboId_ == 0 || eboId_ == 0 || indexCount_ == 0) {
        return;
    }

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);  // Don't write to depth buffer for transparent objects
    glDisable(GL_CULL_FACE);

    shaderProgram_->bind();
    shaderProgram_->setUniformValue("projection", projection);
    shaderProgram_->setUniformValue("view", view);
    shaderProgram_->setUniformValue("model", model);
    shaderProgram_->setUniformValue("opacity", opacity_);

    // Extract camera position from view matrix
    QMatrix4x4 invView = view.inverted();
    QVector3D viewPos = invView.column(3).toVector3D();
    shaderProgram_->setUniformValue("viewPos", viewPos);

    vao_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);

    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

    vao_.release();
    shaderProgram_->release();

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
