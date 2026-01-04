// DebugRayRenderer.cpp - Implementation of debug ray visualization
#include "DebugRayRenderer.h"
#include "GLUtils.h"
#include "Constants.h"
#include <QOpenGLContext>
#include <QDebug>
#include <cmath>

using namespace RS::Constants;

// Colors for debug visualization
namespace {
    const QVector3D kIncidentRayColor(0.0f, 1.0f, 0.0f);      // Bright green
    const QVector3D kReflectionRayColor(1.0f, 0.0f, 1.0f);    // Bright magenta
    const QVector3D kMissRayColor(1.0f, 0.0f, 0.0f);          // Red
    const QVector3D kHitMarkerColor(1.0f, 1.0f, 0.0f);        // Yellow
    const float kHitMarkerSize = 2.0f;
    const float kReflectionRayLength = 50.0f;
}

DebugRayRenderer::DebugRayRenderer(QObject* parent)
    : QObject(parent)
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

DebugRayRenderer::~DebugRayRenderer() {
    // OpenGL cleanup should be done via cleanup() before context destruction
}

bool DebugRayRenderer::initialize() {
    if (initialized_) {
        return true;
    }

    if (!QOpenGLContext::currentContext()) {
        qWarning() << "DebugRayRenderer::initialize - No OpenGL context available";
        return false;
    }

    if (!initializeOpenGLFunctions()) {
        qCritical() << "DebugRayRenderer: Failed to initialize OpenGL functions!";
        return false;
    }

    GLUtils::clearGLErrors();

    setupShaders();

    vao_.create();
    vao_.bind();
    vao_.release();

    GLUtils::checkGLError("DebugRayRenderer::initialize");

    initialized_ = true;
    return true;
}

void DebugRayRenderer::cleanup() {
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

void DebugRayRenderer::setupShaders() {
    if (vertexShaderSource_.empty() || fragmentShaderSource_.empty()) {
        qCritical() << "DebugRayRenderer: Shader sources not initialized!";
        return;
    }

    shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_.data())) {
        qCritical() << "DebugRayRenderer: Failed to compile vertex shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_.data())) {
        qCritical() << "DebugRayRenderer: Failed to compile fragment shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->link()) {
        qCritical() << "DebugRayRenderer: Failed to link shader program:" << shaderProgram_->log();
        return;
    }
}

void DebugRayRenderer::setRayData(const QVector3D& radarPos, const RCS::HitResult& hit,
                                   float maxDistance) {
    radarPos_ = radarPos;
    maxDistance_ = maxDistance;

    // Check if we have a valid hit (hitPoint.w >= 0 means hit)
    hasHit_ = (hit.hitPoint.w() >= 0.0f);

    if (hasHit_) {
        hitPoint_ = hit.hitPoint.toVector3D();
        hitDistance_ = hit.hitPoint.w();
        reflectionDir_ = hit.reflection.toVector3D().normalized();

        // Calculate ray direction from radar to hit
        rayDirection_ = (hitPoint_ - radarPos_).normalized();

        // Calculate reflection angle from surface normal
        QVector3D normal = hit.normal.toVector3D().normalized();
        float dotProduct = QVector3D::dotProduct(rayDirection_, normal);
        reflectionAngle_ = std::acos(std::abs(dotProduct)) * kRadToDegF;
    } else {
        // No hit - ray goes to max distance
        // Use beam direction (assume it was set to target center direction)
        rayDirection_ = QVector3D(0, 0, -1);  // Default, will be overridden
        hitDistance_ = maxDistance_;
        reflectionAngle_ = 0.0f;
    }

    generateGeometry();
    geometryDirty_ = true;
}

void DebugRayRenderer::clearRayData() {
    hasHit_ = false;
    vertices_.clear();
    vertexCount_ = 0;
    geometryDirty_ = true;
}

void DebugRayRenderer::generateGeometry() {
    vertices_.clear();

    if (hasHit_) {
        // Incident ray: radar -> hit point (green)
        addLineSegment(radarPos_, hitPoint_, kIncidentRayColor);

        // Hit marker (yellow cross)
        addHitMarker(hitPoint_, kHitMarkerSize, kHitMarkerColor);

        // Reflection ray: hit point -> extended reflection direction (magenta)
        QVector3D reflectionEnd = hitPoint_ + reflectionDir_ * kReflectionRayLength;
        addLineSegment(hitPoint_, reflectionEnd, kReflectionRayColor);
    } else {
        // Miss: draw ray to max distance (red)
        QVector3D missEnd = radarPos_ + rayDirection_ * maxDistance_;
        addLineSegment(radarPos_, missEnd, kMissRayColor);
    }

    vertexCount_ = static_cast<int>(vertices_.size() / 6);  // 6 floats per vertex (pos + color)
}

void DebugRayRenderer::addLineSegment(const QVector3D& start, const QVector3D& end,
                                       const QVector3D& color) {
    // Start vertex
    vertices_.push_back(start.x());
    vertices_.push_back(start.y());
    vertices_.push_back(start.z());
    vertices_.push_back(color.x());
    vertices_.push_back(color.y());
    vertices_.push_back(color.z());

    // End vertex
    vertices_.push_back(end.x());
    vertices_.push_back(end.y());
    vertices_.push_back(end.z());
    vertices_.push_back(color.x());
    vertices_.push_back(color.y());
    vertices_.push_back(color.z());
}

void DebugRayRenderer::addHitMarker(const QVector3D& position, float size,
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

void DebugRayRenderer::uploadGeometry() {
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

void DebugRayRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view) {
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

    // Disable depth test so ray is always visible
    glDisable(GL_DEPTH_TEST);

    // Set thick line width for visibility
    glLineWidth(3.0f);

    shaderProgram_->bind();
    shaderProgram_->setUniformValue("projection", projection);
    shaderProgram_->setUniformValue("view", view);

    vao_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);

    // Draw all lines
    glDrawArrays(GL_LINES, 0, vertexCount_);

    vao_.release();
    shaderProgram_->release();

    // Restore state
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}
