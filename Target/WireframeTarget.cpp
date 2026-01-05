// WireframeTarget.cpp

#include "WireframeTarget.h"
#include "GLUtils.h"
#include "CubeWireframe.h"
#include "CylinderWireframe.h"
#include "AircraftWireframe.h"
#include "SphereWireframe.h"
#include "Constants.h"
#include <QDebug>
#include <QOpenGLContext>
#include <cmath>
#include <map>
#include <algorithm>

using namespace RS::Constants;

WireframeTarget::WireframeTarget()
    : position_(0.0f, 0.0f, 0.0f),
      rotation_(),
      scale_(1.0f, 1.0f, 1.0f),
      color_(Colors::kTargetGreen[0], Colors::kTargetGreen[1], Colors::kTargetGreen[2]),
      visible_(true)
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

    // Fragment shader with diffuse + ambient lighting + radar angle-based edge darkening
    fragmentShaderSource_ = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;

        uniform vec3 objectColor;
        uniform vec3 lightPos;
        uniform vec3 radarPos;

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

            // Radar angle-based edge darkening
            // Faces perpendicular to radar direction appear darker
            vec3 toRadar = normalize(radarPos - FragPos);
            float radarDot = abs(dot(norm, toRadar));  // 0 = perpendicular, 1 = facing
            float edgeDarken = smoothstep(0.0, 0.4, radarDot);

            // Result with edge darkening
            vec3 result = (ambient + diffuse) * objectColor * (0.6 + 0.4 * edgeDarken);
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
        shaderProgram_.reset();
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
    // Clean up edge VBO
    if (edgeVboId_ != 0) {
        glDeleteBuffers(1, &edgeVboId_);
        edgeVboId_ = 0;
    }
    shaderProgram_.reset();
}

WireframeTarget::~WireframeTarget() {
    // OpenGL resources should already be cleaned up via cleanup() called from
    // RadarGLWidget::cleanupGL() before context destruction.
    // Note: shaderProgram_ should be nullptr at this point
}

void WireframeTarget::initialize() {
    if (vao_.isCreated()) {
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
    if (vertexShaderSource_.empty() || fragmentShaderSource_.empty()) {
        qCritical() << "WireframeTarget: Shader sources not initialized!";
        return;
    }

    shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_.data())) {
        qCritical() << "WireframeTarget: Failed to compile vertex shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_.data())) {
        qCritical() << "WireframeTarget: Failed to compile fragment shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->link()) {
        qCritical() << "WireframeTarget: Failed to link shader program:" << shaderProgram_->log();
        return;
    }
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
    shaderProgram_->setUniformValue("lightPos", QVector3D(Lighting::kTargetLightPosition[0], Lighting::kTargetLightPosition[1], Lighting::kTargetLightPosition[2]));
    shaderProgram_->setUniformValue("radarPos", radarPos_);

    vao_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);

    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

    vao_.release();

    // Second pass: draw only crease edges (not internal triangle diagonals)
    // This gives clean edge definition without showing internal triangulation
    if (edgeVboId_ != 0 && edgeVertexCount_ > 0) {
        // Use polygon offset to prevent z-fighting with the solid surface
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        glLineWidth(1.5f);

        // Darker edge color (30% of original brightness)
        QVector3D edgeColor = color_ * 0.3f;
        shaderProgram_->setUniformValue("objectColor", edgeColor);
        // Set a neutral normal for edge lines (lighting less important for edges)
        shaderProgram_->setUniformValue("lightPos", QVector3D(0, 100, 100));

        // Bind edge VBO and set up vertex attribute
        glBindBuffer(GL_ARRAY_BUFFER, edgeVboId_);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        // Disable normal attribute for edge lines (not needed)
        glDisableVertexAttribArray(1);

        glDrawArrays(GL_LINES, 0, edgeVertexCount_);

        // Restore state
        glEnableVertexAttribArray(1);  // Re-enable normal attribute
        glDisable(GL_POLYGON_OFFSET_LINE);
        glLineWidth(1.0f);
    }

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

// Factory method
std::unique_ptr<WireframeTarget> WireframeTarget::createTarget(WireframeType type) {
    switch (type) {
    case WireframeType::Cube:
        return std::make_unique<CubeWireframe>();
    case WireframeType::Cylinder:
        return std::make_unique<CylinderWireframe>();
    case WireframeType::Aircraft:
        return std::make_unique<AircraftWireframe>();
    case WireframeType::Sphere:
        return std::make_unique<SphereWireframe>(3);  // 3 subdivisions = 1280 faces
    default:
        return std::make_unique<CubeWireframe>();
    }
}

// Edge detection - find crease edges where adjacent face normals differ significantly
void WireframeTarget::detectEdges() {
    edges_.clear();

    if (indices_.empty()) {
        return;
    }

    // Build edge-to-triangle adjacency map
    // Key: ordered vertex pair (min, max), Value: list of triangle indices
    std::map<std::pair<uint32_t, uint32_t>, std::vector<int>> edgeTriangles;

    for (size_t i = 0; i < indices_.size(); i += 3) {
        int triIdx = static_cast<int>(i / 3);
        uint32_t v0 = indices_[i], v1 = indices_[i + 1], v2 = indices_[i + 2];

        // Add each edge of this triangle (ordered so min vertex first)
        auto addEdge = [&](uint32_t a, uint32_t b) {
            auto key = std::make_pair(std::min(a, b), std::max(a, b));
            edgeTriangles[key].push_back(triIdx);
        };
        addEdge(v0, v1);
        addEdge(v1, v2);
        addEdge(v2, v0);
    }

    // Crease threshold: cos(10°) ≈ 0.985
    // Edges where adjacent faces differ by more than ~10° are "creases"
    constexpr float kCreaseThreshold = 0.985f;

    for (const auto& [edge, tris] : edgeTriangles) {
        GeometricEdge ge;
        ge.v0 = edge.first;
        ge.v1 = edge.second;

        if (tris.size() == 1) {
            // Boundary edge (only one face) - always draw
            ge.isCrease = true;
            ge.creaseAngle = static_cast<float>(M_PI);  // 180° for boundary
        } else if (tris.size() == 2) {
            // Compare normals of adjacent triangles
            QVector3D n0 = getTriangleNormal(tris[0]);
            QVector3D n1 = getTriangleNormal(tris[1]);
            float dot = QVector3D::dotProduct(n0, n1);
            ge.creaseAngle = std::acos(std::clamp(dot, -1.0f, 1.0f));
            ge.isCrease = (dot < kCreaseThreshold);
        } else {
            // Non-manifold edge (3+ faces) - draw it as a warning
            ge.isCrease = true;
            ge.creaseAngle = 0.0f;
        }

        edges_.push_back(ge);
    }
}

// Get face normal for a triangle by index
QVector3D WireframeTarget::getTriangleNormal(int triIdx) const {
    int base = triIdx * 3;
    if (base + 2 >= static_cast<int>(indices_.size())) {
        return QVector3D(0, 1, 0);  // Fallback
    }

    // Get vertex positions from vertices_ array (stride 6: x,y,z,nx,ny,nz)
    auto getPos = [this](uint32_t idx) {
        size_t offset = idx * 6;
        if (offset + 2 >= vertices_.size()) {
            return QVector3D(0, 0, 0);
        }
        return QVector3D(vertices_[offset], vertices_[offset + 1], vertices_[offset + 2]);
    };

    QVector3D p0 = getPos(indices_[base]);
    QVector3D p1 = getPos(indices_[base + 1]);
    QVector3D p2 = getPos(indices_[base + 2]);

    QVector3D normal = QVector3D::crossProduct(p1 - p0, p2 - p0);
    float len = normal.length();
    if (len > 0.0001f) {
        return normal / len;
    }
    return QVector3D(0, 1, 0);  // Degenerate triangle fallback
}

// Generate edge line vertices for rendering (only crease edges)
void WireframeTarget::generateEdgeGeometry() {
    edgeVertices_.clear();

    for (const auto& edge : edges_) {
        if (!edge.isCrease) {
            continue;  // Skip internal coplanar edges
        }

        // Get vertex positions (stride 6: x,y,z,nx,ny,nz)
        size_t off0 = edge.v0 * 6;
        size_t off1 = edge.v1 * 6;

        if (off0 + 2 >= vertices_.size() || off1 + 2 >= vertices_.size()) {
            continue;
        }

        // Add line segment (position only, 3 floats per vertex)
        edgeVertices_.push_back(vertices_[off0]);
        edgeVertices_.push_back(vertices_[off0 + 1]);
        edgeVertices_.push_back(vertices_[off0 + 2]);
        edgeVertices_.push_back(vertices_[off1]);
        edgeVertices_.push_back(vertices_[off1 + 1]);
        edgeVertices_.push_back(vertices_[off1 + 2]);
    }

    edgeVertexCount_ = static_cast<int>(edgeVertices_.size() / 3);
}

// Upload edge line geometry to GPU
void WireframeTarget::uploadEdgeGeometry() {
    if (!QOpenGLContext::currentContext()) {
        return;
    }

    if (edgeVertices_.empty()) {
        edgeVertexCount_ = 0;
        return;
    }

    // Create edge VBO if needed
    if (edgeVboId_ == 0) {
        glGenBuffers(1, &edgeVboId_);
    }

    glBindBuffer(GL_ARRAY_BUFFER, edgeVboId_);
    glBufferData(GL_ARRAY_BUFFER,
                 edgeVertices_.size() * sizeof(float),
                 edgeVertices_.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
