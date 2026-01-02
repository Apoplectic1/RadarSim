// RadarSiteRenderer.cpp - Renders the radar site dot on the sphere

#include "RadarSiteRenderer.h"
#include "Constants.h"
#include "GLUtils.h"

#include <QOpenGLContext>
#include <cmath>
#include <array>

using namespace RS::Constants;

RadarSiteRenderer::RadarSiteRenderer() {
    // Initialize shader sources
    vertexShaderSource_ = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 Normal;
        out vec3 FragPos;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    fragmentShaderSource_ = R"(
        #version 330 core
        in vec3 Normal;
        in vec3 FragPos;

        uniform vec3 lightPos;
        uniform vec3 color;
        uniform float opacity;

        out vec4 outColor;

        void main() {
            // Simple lighting calculation
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
            vec3 ambient = vec3(0.3, 0.3, 0.3);
            vec3 result = (ambient + diffuse) * color;

            // Apply opacity
            outColor = vec4(result, opacity);
        }
    )";
}

RadarSiteRenderer::~RadarSiteRenderer() {
    // OpenGL resources should be cleaned up via cleanup() before context destruction
}

void RadarSiteRenderer::cleanup() {
    if (!initialized_) {
        return;
    }

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning() << "RadarSiteRenderer::cleanup() - no current context, skipping GL cleanup";
        shaderProgram_.reset();
        return;
    }

    if (vao_.isCreated()) {
        vao_.destroy();
    }
    if (vbo_.isCreated()) {
        vbo_.destroy();
    }

    shaderProgram_.reset();
    initialized_ = false;
}

bool RadarSiteRenderer::initialize() {
    if (!initializeOpenGLFunctions()) {
        qCritical() << "RadarSiteRenderer: Failed to initialize OpenGL functions!";
        return false;
    }

    GLUtils::clearGLErrors();

    // Create shader program
    shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_.data())) {
        qWarning() << "Failed to compile radar site vertex shader:" << shaderProgram_->log();
        return false;
    }

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_.data())) {
        qWarning() << "Failed to compile radar site fragment shader:" << shaderProgram_->log();
        return false;
    }

    if (!shaderProgram_->link()) {
        qWarning() << "Failed to link radar site shader program:" << shaderProgram_->log();
        return false;
    }

    GLUtils::checkGLError("RadarSiteRenderer::initialize after shaders");

    // Create geometry
    createDotGeometry();

    initialized_ = true;
    return true;
}

void RadarSiteRenderer::createDotGeometry() {
    vertices_.clear();

    float dotRadius = kRadarDotRadius;

    // Generate icosahedron vertices for the dot
    std::vector<QVector3D> positions;

    // Icosahedron vertices
    float t = (1.0f + sqrt(5.0f)) / 2.0f;

    positions.push_back(QVector3D(-1, t, 0).normalized() * dotRadius);
    positions.push_back(QVector3D(1, t, 0).normalized() * dotRadius);
    positions.push_back(QVector3D(-1, -t, 0).normalized() * dotRadius);
    positions.push_back(QVector3D(1, -t, 0).normalized() * dotRadius);

    positions.push_back(QVector3D(0, -1, t).normalized() * dotRadius);
    positions.push_back(QVector3D(0, 1, t).normalized() * dotRadius);
    positions.push_back(QVector3D(0, -1, -t).normalized() * dotRadius);
    positions.push_back(QVector3D(0, 1, -t).normalized() * dotRadius);

    positions.push_back(QVector3D(t, 0, -1).normalized() * dotRadius);
    positions.push_back(QVector3D(t, 0, 1).normalized() * dotRadius);
    positions.push_back(QVector3D(-t, 0, -1).normalized() * dotRadius);
    positions.push_back(QVector3D(-t, 0, 1).normalized() * dotRadius);

    // Icosahedron faces
    std::vector<std::array<int, 3>> faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    // Create vertex data (position + normal)
    for (const auto& face : faces) {
        for (int idx : face) {
            QVector3D pos = positions[idx];
            QVector3D normal = pos.normalized();

            vertices_.push_back(pos.x());
            vertices_.push_back(pos.y());
            vertices_.push_back(pos.z());
            vertices_.push_back(normal.x());
            vertices_.push_back(normal.y());
            vertices_.push_back(normal.z());
        }
    }

    // Set up VAO and VBO
    if (!vao_.isCreated()) {
        vao_.create();
    }
    vao_.bind();

    if (!vbo_.isCreated()) {
        vbo_.create();
    }
    vbo_.bind();
    vbo_.allocate(vertices_.data(), static_cast<int>(vertices_.size() * sizeof(float)));

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    vao_.release();
}

void RadarSiteRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view,
                                const QMatrix4x4& model, float radius) {
    if (!initialized_ || !shaderProgram_ || !shaderProgram_->isLinked()) {
        return;
    }

    // Calculate dot position in spherical coordinates
    QVector3D dotPos = sphericalToCartesian(radius, theta_, phi_);

    // Create model matrix for the dot
    QMatrix4x4 dotModelMatrix = model;
    dotModelMatrix.translate(dotPos);

    // 1. First pass: Draw opaque front-facing parts
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    shaderProgram_->bind();
    shaderProgram_->setUniformValue("projection", projection);
    shaderProgram_->setUniformValue("view", view);
    shaderProgram_->setUniformValue("model", dotModelMatrix);
    shaderProgram_->setUniformValue("color", color_);
    shaderProgram_->setUniformValue("lightPos", QVector3D(Lighting::kLightPosition[0],
                                                          Lighting::kLightPosition[1],
                                                          Lighting::kLightPosition[2]));
    shaderProgram_->setUniformValue("opacity", 1.0f);

    vao_.bind();

    // Only draw front-facing polygons
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size() / 6));
    glDisable(GL_CULL_FACE);

    vao_.release();

    // 2. Second pass: Draw transparent back-facing parts
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    shaderProgram_->setUniformValue("opacity", 0.2f);

    vao_.bind();

    // Draw behind other objects
    glDepthFunc(GL_GREATER);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size() / 6));

    // Restore defaults
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    vao_.release();
    shaderProgram_->release();
}

void RadarSiteRenderer::setPosition(float theta, float phi) {
    theta_ = theta;
    phi_ = phi;
}

QVector3D RadarSiteRenderer::getCartesianPosition(float radius) const {
    return sphericalToCartesian(radius, theta_, phi_);
}

void RadarSiteRenderer::setColor(const QVector3D& color) {
    color_ = color;
}

QVector3D RadarSiteRenderer::sphericalToCartesian(float r, float thetaDeg, float phiDeg) const {
    float theta = thetaDeg * kDegToRadF;
    float phi = phiDeg * kDegToRadF;
    return QVector3D(
        r * cos(phi) * cos(theta),
        r * cos(phi) * sin(theta),  // Y is horizontal
        r * sin(phi)                // Z is vertical (elevation)
    );
}
