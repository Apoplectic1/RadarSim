// WireframeTarget.cpp

#include "WireframeTarget.h"
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

WireframeTarget::~WireframeTarget() {
    QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    if (!currentContext) {
        qWarning() << "No OpenGL context in WireframeTarget destructor, resources may leak";
        return;
    }

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
    if (shaderProgram_) {
        delete shaderProgram_;
        shaderProgram_ = nullptr;
    }

    // Clean up shadow volume resources
    if (shadowVao_.isCreated()) {
        shadowVao_.destroy();
    }
    if (shadowVboId_ != 0) {
        glDeleteBuffers(1, &shadowVboId_);
        shadowVboId_ = 0;
    }
    if (shadowEboId_ != 0) {
        glDeleteBuffers(1, &shadowEboId_);
        shadowEboId_ = 0;
    }
    if (shadowShaderProgram_) {
        delete shadowShaderProgram_;
        shadowShaderProgram_ = nullptr;
    }

    // Clean up depth cap resources
    if (depthCapVao_.isCreated()) {
        depthCapVao_.destroy();
    }
    if (depthCapVboId_ != 0) {
        glDeleteBuffers(1, &depthCapVboId_);
        depthCapVboId_ = 0;
    }
    if (depthCapEboId_ != 0) {
        glDeleteBuffers(1, &depthCapEboId_);
        depthCapEboId_ = 0;
    }

    qDebug() << "WireframeTarget destructor - cleaned up OpenGL resources";
}

void WireframeTarget::initialize() {
    if (vao_.isCreated()) {
        qDebug() << "WireframeTarget already initialized, skipping";
        return;
    }

    initializeOpenGLFunctions();
    setupShaders();

    vao_.create();
    vao_.bind();
    vao_.release();

    // Generate and upload initial geometry
    generateGeometry();
    uploadGeometryToGPU();
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

void WireframeTarget::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& sceneModel,
                             const QVector3D& radarPosition, float sphereRadius) {
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

    // === Pass 0: Depth cap for Z-fail algorithm ===
    // DISABLED: Replaced by fragment shader shadow cone test in RadarBeam
#if 0
    // Renders a sphere at the scene boundary to depth buffer only.
    // This provides depth values where the visible sphere would be, even when hidden.
    // The depth cap MUST be at the SAME radius as the scene sphere - if it's larger,
    // both shadow caps end up behind the depth cap and their stencil ops cancel out.
    if (!radarPosition.isNull()) {
        // Depth cap at same radius as scene sphere
        float depthCapRadius = sphereRadius;

        // Regenerate depth cap if sphere radius changed
        if (std::abs(lastDepthCapRadius_ - depthCapRadius) > 0.01f) {
            generateDepthCap(depthCapRadius);
            uploadDepthCapToGPU();
            lastDepthCapRadius_ = depthCapRadius;
        }

        // Render depth cap to depth buffer only
        // Depth cap is in world space (centered at origin), apply view * sceneModel
        QMatrix4x4 viewSceneModel = view * sceneModel;
        renderDepthCap(projection, viewSceneModel);
    }
#endif

    // === Pass 1: Render visible surface with color ===
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

    // === Pass 2: Shadow volume using stencil (Z-fail / Carmack's Reverse) ===
    // DISABLED: Replaced by fragment shader shadow cone test in RadarBeam
#if 0
    if (!radarPosition.isNull()) {
        // Generate shadow volume in WORLD space (using localModel only)
        // - radarPosition is already in world space (from sphericalToCartesian)
        // - localModel transforms target vertices to world space
        // - Scene rotation is applied at render time via sceneModel
        generateShadowVolume(radarPosition, localModel, sphereRadius * 2.0f);

        if (!shadowIndices_.empty()) {
            uploadShadowVolumeToGPU();

            // Setup for shadow volume rendering
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);

            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);

            // Render shadow volume with two-sided stencil (Z-fail)
            // Shadow vertices are in WORLD space, apply sceneModel for scene rotation
            // Back faces: increment on depth fail
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            glStencilOp(GL_KEEP, GL_INCR_WRAP, GL_KEEP);

            renderShadowVolume(projection, view, sceneModel);

            // Front faces: decrement on depth fail
            glCullFace(GL_BACK);
            glStencilOp(GL_KEEP, GL_DECR_WRAP, GL_KEEP);

            renderShadowVolume(projection, view, sceneModel);

            // Restore state - leave stencil test ON for beam
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glStencilMask(0x00);
            glStencilFunc(GL_EQUAL, 0, 0xFF);  // Only draw where stencil == 0
        }
    }
#endif

    glDisable(GL_CULL_FACE);

    // Mark radarPosition and sphereRadius as unused (shadow now handled in beam fragment shader)
    (void)radarPosition;
    (void)sphereRadius;
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

// Shadow volume implementation
void WireframeTarget::setupShadowShaders() {
    if (shadowShaderProgram_) return;

    shadowShaderProgram_ = new QOpenGLShaderProgram();

    // Vertex shader with separate matrices (matching target shader)
    const char* vertSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    // Fragment shader - outputs nothing (stencil only)
    const char* fragSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.0);
        }
    )";

    shadowShaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc);
    shadowShaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc);
    shadowShaderProgram_->link();
}

void WireframeTarget::generateShadowVolume(const QVector3D& lightPos, const QMatrix4x4& modelMatrix, float extrudeDistance) {
    shadowVertices_.clear();
    shadowIndices_.clear();

    // Use provided extrude distance (typically 2x sphere radius)
    const float extrudeDist = extrudeDistance;

    // Process each triangle
    int numTriangles = indices_.size() / 3;
    for (int t = 0; t < numTriangles; t++) {
        // Get vertex indices
        GLuint i0 = indices_[t * 3 + 0];
        GLuint i1 = indices_[t * 3 + 1];
        GLuint i2 = indices_[t * 3 + 2];

        // Get vertex positions (skip normals - stride of 6)
        QVector3D v0(vertices_[i0 * 6 + 0], vertices_[i0 * 6 + 1], vertices_[i0 * 6 + 2]);
        QVector3D v1(vertices_[i1 * 6 + 0], vertices_[i1 * 6 + 1], vertices_[i1 * 6 + 2]);
        QVector3D v2(vertices_[i2 * 6 + 0], vertices_[i2 * 6 + 1], vertices_[i2 * 6 + 2]);

        // Transform to world space
        QVector3D w0 = modelMatrix.map(v0);
        QVector3D w1 = modelMatrix.map(v1);
        QVector3D w2 = modelMatrix.map(v2);

        // Calculate triangle normal
        QVector3D edge1 = w1 - w0;
        QVector3D edge2 = w2 - w0;
        QVector3D triNormal = QVector3D::crossProduct(edge1, edge2).normalized();

        // Check if triangle faces the light
        QVector3D triCenter = (w0 + w1 + w2) / 3.0f;
        QVector3D toLight = (lightPos - triCenter).normalized();

        if (QVector3D::dotProduct(triNormal, toLight) > 0) {
            // Triangle faces the light - create shadow volume

            // Extrude vertices away from light
            QVector3D dir0 = (w0 - lightPos).normalized();
            QVector3D dir1 = (w1 - lightPos).normalized();
            QVector3D dir2 = (w2 - lightPos).normalized();

            QVector3D e0 = w0 + dir0 * extrudeDist;
            QVector3D e1 = w1 + dir1 * extrudeDist;
            QVector3D e2 = w2 + dir2 * extrudeDist;

            GLuint baseIdx = static_cast<GLuint>(shadowVertices_.size() / 3);

            // Add 6 vertices: 3 original + 3 extruded
            shadowVertices_.push_back(w0.x()); shadowVertices_.push_back(w0.y()); shadowVertices_.push_back(w0.z());
            shadowVertices_.push_back(w1.x()); shadowVertices_.push_back(w1.y()); shadowVertices_.push_back(w1.z());
            shadowVertices_.push_back(w2.x()); shadowVertices_.push_back(w2.y()); shadowVertices_.push_back(w2.z());
            shadowVertices_.push_back(e0.x()); shadowVertices_.push_back(e0.y()); shadowVertices_.push_back(e0.z());
            shadowVertices_.push_back(e1.x()); shadowVertices_.push_back(e1.y()); shadowVertices_.push_back(e1.z());
            shadowVertices_.push_back(e2.x()); shadowVertices_.push_back(e2.y()); shadowVertices_.push_back(e2.z());

            // Front cap (original triangle)
            shadowIndices_.push_back(baseIdx + 0);
            shadowIndices_.push_back(baseIdx + 1);
            shadowIndices_.push_back(baseIdx + 2);

            // Back cap (extruded triangle, reversed winding)
            shadowIndices_.push_back(baseIdx + 5);
            shadowIndices_.push_back(baseIdx + 4);
            shadowIndices_.push_back(baseIdx + 3);

            // Side quads (3 edges)
            // Edge 0-1
            shadowIndices_.push_back(baseIdx + 0);
            shadowIndices_.push_back(baseIdx + 3);
            shadowIndices_.push_back(baseIdx + 4);
            shadowIndices_.push_back(baseIdx + 0);
            shadowIndices_.push_back(baseIdx + 4);
            shadowIndices_.push_back(baseIdx + 1);

            // Edge 1-2
            shadowIndices_.push_back(baseIdx + 1);
            shadowIndices_.push_back(baseIdx + 4);
            shadowIndices_.push_back(baseIdx + 5);
            shadowIndices_.push_back(baseIdx + 1);
            shadowIndices_.push_back(baseIdx + 5);
            shadowIndices_.push_back(baseIdx + 2);

            // Edge 2-0
            shadowIndices_.push_back(baseIdx + 2);
            shadowIndices_.push_back(baseIdx + 5);
            shadowIndices_.push_back(baseIdx + 3);
            shadowIndices_.push_back(baseIdx + 2);
            shadowIndices_.push_back(baseIdx + 3);
            shadowIndices_.push_back(baseIdx + 0);
        }
    }

    shadowIndexCount_ = static_cast<int>(shadowIndices_.size());
}

void WireframeTarget::uploadShadowVolumeToGPU() {
    if (shadowVertices_.empty() || shadowIndices_.empty()) {
        return;
    }

    // Setup shadow shader if needed
    if (!shadowShaderProgram_) {
        setupShadowShaders();
    }

    // Create VAO if needed
    if (!shadowVao_.isCreated()) {
        shadowVao_.create();
    }

    shadowVao_.bind();

    // VBO
    if (shadowVboId_ == 0) {
        glGenBuffers(1, &shadowVboId_);
    }
    glBindBuffer(GL_ARRAY_BUFFER, shadowVboId_);
    glBufferData(GL_ARRAY_BUFFER,
                 shadowVertices_.size() * sizeof(float),
                 shadowVertices_.data(),
                 GL_DYNAMIC_DRAW);

    // Position attribute only (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // EBO
    if (shadowEboId_ == 0) {
        glGenBuffers(1, &shadowEboId_);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadowEboId_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 shadowIndices_.size() * sizeof(GLuint),
                 shadowIndices_.data(),
                 GL_DYNAMIC_DRAW);

    shadowVao_.release();
}

void WireframeTarget::renderShadowVolume(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    if (shadowIndexCount_ == 0 || !shadowShaderProgram_) {
        return;
    }

    shadowShaderProgram_->bind();

    // Set separate matrices (matching target shader setup)
    shadowShaderProgram_->setUniformValue("projection", projection);
    shadowShaderProgram_->setUniformValue("view", view);
    shadowShaderProgram_->setUniformValue("model", model);

    shadowVao_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, shadowVboId_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadowEboId_);
    glDrawElements(GL_TRIANGLES, shadowIndexCount_, GL_UNSIGNED_INT, nullptr);
    shadowVao_.release();

    shadowShaderProgram_->release();
}

// Depth cap implementation - sphere at scene boundary for Z-fail algorithm
void WireframeTarget::generateDepthCap(float radius) {
    depthCapVertices_.clear();
    depthCapIndices_.clear();

    // Generate a sphere with lat/long subdivision
    const int latSegments = 16;
    const int longSegments = 24;

    // Generate vertices
    for (int lat = 0; lat <= latSegments; ++lat) {
        float theta = float(lat) * float(M_PI) / float(latSegments);
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int lon = 0; lon <= longSegments; ++lon) {
            float phi = float(lon) * 2.0f * float(M_PI) / float(longSegments);
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            // Position on sphere surface
            depthCapVertices_.push_back(x * radius);
            depthCapVertices_.push_back(y * radius);
            depthCapVertices_.push_back(z * radius);
        }
    }

    // Generate indices for triangles
    for (int lat = 0; lat < latSegments; ++lat) {
        for (int lon = 0; lon < longSegments; ++lon) {
            int first = lat * (longSegments + 1) + lon;
            int second = first + longSegments + 1;

            // Two triangles per quad, wound for outside-facing (standard winding)
            // This makes the sphere visible from outside, which is what we need
            // for Z-fail to work - shadow back caps fail against this outer surface
            depthCapIndices_.push_back(first);
            depthCapIndices_.push_back(second);
            depthCapIndices_.push_back(first + 1);

            depthCapIndices_.push_back(second);
            depthCapIndices_.push_back(second + 1);
            depthCapIndices_.push_back(first + 1);
        }
    }

    depthCapIndexCount_ = static_cast<int>(depthCapIndices_.size());
}

void WireframeTarget::uploadDepthCapToGPU() {
    if (depthCapVertices_.empty() || depthCapIndices_.empty()) {
        return;
    }

    // Setup shadow shader if needed (reuse for depth cap)
    if (!shadowShaderProgram_) {
        setupShadowShaders();
    }

    // Create VAO if needed
    if (!depthCapVao_.isCreated()) {
        depthCapVao_.create();
    }

    depthCapVao_.bind();

    // VBO
    if (depthCapVboId_ == 0) {
        glGenBuffers(1, &depthCapVboId_);
    }
    glBindBuffer(GL_ARRAY_BUFFER, depthCapVboId_);
    glBufferData(GL_ARRAY_BUFFER,
                 depthCapVertices_.size() * sizeof(float),
                 depthCapVertices_.data(),
                 GL_STATIC_DRAW);

    // Position attribute only (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // EBO
    if (depthCapEboId_ == 0) {
        glGenBuffers(1, &depthCapEboId_);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, depthCapEboId_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 depthCapIndices_.size() * sizeof(GLuint),
                 depthCapIndices_.data(),
                 GL_STATIC_DRAW);

    depthCapVao_.release();
}

void WireframeTarget::renderDepthCap(const QMatrix4x4& projection, const QMatrix4x4& viewModel) {
    if (depthCapIndexCount_ == 0 || !shadowShaderProgram_) {
        return;
    }

    // Render to depth buffer only - no color, no stencil modification
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);

    // Render sphere with back-face culling
    // With outside-facing winding, we render front faces (near hemisphere from camera)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    shadowShaderProgram_->bind();

    // viewModel already contains view * sceneModel, so model is identity
    QMatrix4x4 identity;
    identity.setToIdentity();
    shadowShaderProgram_->setUniformValue("projection", projection);
    shadowShaderProgram_->setUniformValue("view", identity);
    shadowShaderProgram_->setUniformValue("model", viewModel);

    depthCapVao_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, depthCapVboId_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, depthCapEboId_);
    glDrawElements(GL_TRIANGLES, depthCapIndexCount_, GL_UNSIGNED_INT, nullptr);
    depthCapVao_.release();

    shadowShaderProgram_->release();

    // Restore state
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_CULL_FACE);
}
