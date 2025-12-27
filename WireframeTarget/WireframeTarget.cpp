// WireframeTarget.cpp

#include "WireframeTarget.h"
#include "GLUtils.h"
#include "CubeWireframe.h"
#include "CylinderWireframe.h"
#include "AircraftWireframe.h"
#include <QDebug>
#include <QOpenGLContext>
#include <cmath>

WireframeTarget::WireframeTarget()
    : position_(0.0f, 0.0f, 0.0f),
      rotation_(),
      scale_(1.0f, 1.0f, 1.0f),
      color_(0.0f, 1.0f, 0.0f),
      visible_(true),
      illuminated_(false),
      lightDirection_(0.0f, 0.0f, 1.0f)
{
    // Vertex shader for solid surface rendering
    vertexShaderSource_ = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    // Fragment shader with diffuse + ambient lighting
    fragmentShaderSource_ = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;

        uniform vec3 objectColor;
        uniform vec3 lightPos;

        out vec4 FragColor;

        void main() {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

            // Result
            vec3 result = (ambient + diffuse) * objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";
}

// Clean up OpenGL resources - must be called with valid GL context
void WireframeTarget::cleanup() {
    // Check for valid OpenGL context
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning() << "WireframeTarget::cleanup() - no current context, skipping GL cleanup";
        delete shaderProgram_;
        shaderProgram_ = nullptr;
        return;
    }

    qDebug() << "WireframeTarget::cleanup() - cleaning up OpenGL resources";

    // Clean up main geometry resources
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
    delete shaderProgram_;
    shaderProgram_ = nullptr;

    qDebug() << "WireframeTarget::cleanup() complete";
}

WireframeTarget::~WireframeTarget() {
    // OpenGL resources should already be cleaned up via cleanup() called from
    // RadarGLWidget::cleanupGL() before context destruction.
    // Only clean up non-GL resources here.

    // If shader program still exists, delete it (safe without GL context)
    if (shaderProgram_) {
        delete shaderProgram_;
        shaderProgram_ = nullptr;
    }

    qDebug() << "WireframeTarget destructor called";
}

void WireframeTarget::initialize() {
    if (vao_.isCreated()) {
        qDebug() << "WireframeTarget already initialized, skipping";
        return;
    }

    if (!initializeOpenGLFunctions()) {
        qCritical() << "WireframeTarget: Failed to initialize OpenGL functions!";
        return;
    }

    // Clear pending GL errors
    GLUtils::clearGLErrors();

    setupShaders();

    vao_.create();
    vao_.bind();
    vao_.release();

    // Generate and upload initial geometry
    generateGeometry();
    uploadGeometryToGPU();

    // Check for errors after initialization
    GLUtils::checkGLError("WireframeTarget::initialize");
}

void WireframeTarget::setupShaders() {
    if (!vertexShaderSource_ || !fragmentShaderSource_) {
        qCritical() << "WireframeTarget: Shader sources not initialized!";
        return;
    }

    shaderProgram_ = new QOpenGLShaderProgram();

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_)) {
        qCritical() << "WireframeTarget: Failed to compile vertex shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_)) {
        qCritical() << "WireframeTarget: Failed to compile fragment shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->link()) {
        qCritical() << "WireframeTarget: Failed to link shader program:" << shaderProgram_->log();
        return;
    }

    qDebug() << "WireframeTarget shader program compiled and linked successfully";
}

void WireframeTarget::uploadGeometryToGPU() {
    if (!QOpenGLContext::currentContext()) {
        geometryDirty_ = true;
        return;
    }

    if (!vao_.isCreated()) {
        geometryDirty_ = true;
        return;
    }

    if (vertices_.empty() || indices_.empty()) {
        vertexCount_ = 0;
        indexCount_ = 0;
        return;
    }

    vao_.bind();

    // VBO setup
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

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // EBO setup
    if (eboId_ == 0) {
        glGenBuffers(1, &eboId_);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices_.size() * sizeof(GLuint),
                 indices_.data(),
                 GL_DYNAMIC_DRAW);

    vao_.release();

    vertexCount_ = static_cast<int>(vertices_.size() / 6);
    indexCount_ = static_cast<int>(indices_.size());
    geometryDirty_ = false;
}

void WireframeTarget::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& sceneModel) {
    if (!visible_ || indices_.empty()) {
        return;
    }

    if (!vao_.isCreated() || !shaderProgram_) {
        qWarning() << "WireframeTarget::render called with invalid OpenGL resources";
        return;
    }

    if (geometryDirty_) {
        uploadGeometryToGPU();
    }

    if (vboId_ == 0 || eboId_ == 0 || indexCount_ == 0) {
        return;
    }

    // Build combined model matrix: scene transform * local transform
    QMatrix4x4 localModel = buildModelMatrix();
    QMatrix4x4 combinedModel = sceneModel * localModel;

    // Render visible surface with color
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    shaderProgram_->bind();
    shaderProgram_->setUniformValue("projection", projection);
    shaderProgram_->setUniformValue("view", view);
    shaderProgram_->setUniformValue("model", combinedModel);
    shaderProgram_->setUniformValue("objectColor", color_);
    shaderProgram_->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

    vao_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);

    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

    vao_.release();
    shaderProgram_->release();

    glDisable(GL_CULL_FACE);
}

QMatrix4x4 WireframeTarget::buildModelMatrix() const {
    QMatrix4x4 model;
    model.setToIdentity();
    model.translate(position_);
    model.rotate(rotation_);
    model.scale(scale_);
    return model;
}

void WireframeTarget::addVertex(const QVector3D& position, const QVector3D& normal) {
    vertices_.push_back(position.x());
    vertices_.push_back(position.y());
    vertices_.push_back(position.z());
    vertices_.push_back(normal.x());
    vertices_.push_back(normal.y());
    vertices_.push_back(normal.z());
}

void WireframeTarget::addTriangle(GLuint v0, GLuint v1, GLuint v2) {
    indices_.push_back(v0);
    indices_.push_back(v1);
    indices_.push_back(v2);
}

void WireframeTarget::addQuad(GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    // Two triangles: (v0, v1, v2) and (v0, v2, v3)
    addTriangle(v0, v1, v2);
    addTriangle(v0, v2, v3);
}

void WireframeTarget::clearGeometry() {
    vertices_.clear();
    indices_.clear();
    vertexCount_ = 0;
    indexCount_ = 0;
}

// Transform setters
void WireframeTarget::setPosition(const QVector3D& position) {
    position_ = position;
}

void WireframeTarget::setPosition(float x, float y, float z) {
    position_ = QVector3D(x, y, z);
}

void WireframeTarget::setRotation(const QVector3D& eulerAngles) {
    rotation_ = QQuaternion::fromEulerAngles(eulerAngles);
}

void WireframeTarget::setRotation(float pitch, float yaw, float roll) {
    rotation_ = QQuaternion::fromEulerAngles(pitch, yaw, roll);
}

void WireframeTarget::setRotation(const QQuaternion& rotation) {
    rotation_ = rotation;
}

void WireframeTarget::setScale(float uniformScale) {
    scale_ = QVector3D(uniformScale, uniformScale, uniformScale);
}

void WireframeTarget::setScale(const QVector3D& scale) {
    scale_ = scale;
}

QVector3D WireframeTarget::getRotationEuler() const {
    return rotation_.toEulerAngles();
}

// Appearance setters
void WireframeTarget::setColor(const QVector3D& color) {
    color_ = color;
}

void WireframeTarget::setVisible(bool visible) {
    visible_ = visible;
}

// Illumination setters
void WireframeTarget::setIlluminated(bool illuminated) {
    illuminated_ = illuminated;
}

void WireframeTarget::setLightDirection(const QVector3D& lightDir) {
    lightDirection_ = lightDir.normalized();
}

// Factory method
WireframeTarget* WireframeTarget::createTarget(WireframeType type) {
    switch (type) {
    case WireframeType::Cube:
        return new CubeWireframe();
    case WireframeType::Cylinder:
        return new CylinderWireframe();
    case WireframeType::Aircraft:
        return new AircraftWireframe();
    default:
        return new CubeWireframe();
    }
}
