// SingleRayBeam.cpp - Implementation of single-ray diagnostic beam
#include "SingleRayBeam.h"
#include "GLUtils.h"
#include "Constants.h"

using namespace RS::Constants;

SingleRayBeam::SingleRayBeam(float sphereRadius, float beamWidthDegrees)
    : RadarBeam(sphereRadius, beamWidthDegrees)
{
    // SingleRay beam defaults to showing bounce visualization
    showBounceVisualization_ = true;

    // Use green color for diagnostic ray
    color_ = QVector3D(Colors::kBounceBaseColor[0],
                       Colors::kBounceBaseColor[1],
                       Colors::kBounceBaseColor[2]);
}

SingleRayBeam::~SingleRayBeam() {
    // Base class destructor handles cleanup
}

void SingleRayBeam::setupShaders() {
    // Simple vertex shader for lines (position + color)
    beamVertexShaderSource_ = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 Color;

        void main() {
            Color = aColor;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    // Simple fragment shader - pass through color
    beamFragmentShaderSource_ = R"(
        #version 330 core
        in vec3 Color;
        out vec4 FragColor;

        uniform float opacity;

        void main() {
            FragColor = vec4(Color, opacity);
        }
    )";

    // Call base class to compile and link shaders
    RadarBeam::setupShaders();
}

std::vector<QVector3D> SingleRayBeam::getDiagnosticRayDirections() const {
    std::vector<QVector3D> directions;

    // Return the single ray direction (toward origin)
    if (!currentRadarPosition_.isNull()) {
        QVector3D rayDir = -currentRadarPosition_.normalized();
        directions.push_back(rayDir);
    }

    return directions;
}

void SingleRayBeam::createBeamGeometry() {
    vertices_.clear();
    indices_.clear();

    // Skip if no valid radar position
    if (currentRadarPosition_.isNull()) {
        return;
    }

    // Calculate target point (opposite side of sphere through origin)
    QVector3D direction = -currentRadarPosition_.normalized();
    targetPoint_ = calculateOppositePoint(currentRadarPosition_, direction);

    // Create line geometry (2 vertices, position + color format)
    // Vertex 0: radar position (start)
    vertices_.push_back(currentRadarPosition_.x());
    vertices_.push_back(currentRadarPosition_.y());
    vertices_.push_back(currentRadarPosition_.z());
    vertices_.push_back(color_.x());
    vertices_.push_back(color_.y());
    vertices_.push_back(color_.z());

    // Vertex 1: target point (end)
    vertices_.push_back(targetPoint_.x());
    vertices_.push_back(targetPoint_.y());
    vertices_.push_back(targetPoint_.z());
    vertices_.push_back(color_.x());
    vertices_.push_back(color_.y());
    vertices_.push_back(color_.z());

    // For GL_LINES, we use indices 0, 1
    indices_.push_back(0);
    indices_.push_back(1);
}

void SingleRayBeam::render(QOpenGLShaderProgram* program,
                            const QMatrix4x4& projection,
                            const QMatrix4x4& view,
                            const QMatrix4x4& model) {
    if (!visible_ || vertices_.empty()) {
        return;
    }

    // Check if OpenGL resources are valid
    if (!beamVAO_.isCreated() || !beamShaderProgram_) {
        qWarning() << "SingleRayBeam::render called with invalid OpenGL resources";
        return;
    }

    // Upload geometry if deferred
    if (geometryDirty_) {
        uploadGeometryToGPU();
    }

    if (vboId_ == 0 || indices_.empty()) {
        return;
    }

    // Enable depth test so ray is occluded by target
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set thick line width for visibility
    glLineWidth(kBounceLineWidth);

    beamShaderProgram_->bind();

    // Set uniforms
    beamShaderProgram_->setUniformValue("projection", projection);
    beamShaderProgram_->setUniformValue("view", view);
    beamShaderProgram_->setUniformValue("model", model);
    beamShaderProgram_->setUniformValue("opacity", opacity_);

    // Bind VAO and draw as lines
    beamVAO_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);

    // Draw as GL_LINES (2 vertices)
    glDrawArrays(GL_LINES, 0, 2);

    beamVAO_.release();
    beamShaderProgram_->release();

    // Restore state
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}
