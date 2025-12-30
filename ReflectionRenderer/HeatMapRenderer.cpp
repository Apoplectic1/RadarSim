// HeatMapRenderer.cpp - RCS heat map visualization on radar sphere
#include "HeatMapRenderer.h"
#include "GLUtils.h"
#include "Constants.h"
#include <QOpenGLContext>
#include <QDebug>
#include <cmath>
#include <algorithm>

using namespace RadarSim::Constants;

HeatMapRenderer::HeatMapRenderer(QObject* parent)
    : QObject(parent),
      latBins_(kHeatMapLatBins),
      lonBins_(kHeatMapLonBins)
{
    // Initialize bin storage
    binIntensity_.resize(latBins_ * lonBins_, 0.0f);
    binHitCount_.resize(latBins_ * lonBins_, 0);

    // Vertex shader for heat map sphere
    vertexShaderSource_ = R"(
        #version 430 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in float aIntensity;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out float Intensity;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            Intensity = aIntensity;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    // Fragment shader with intensity-based coloring
    fragmentShaderSource_ = R"(
        #version 430 core
        in vec3 FragPos;
        in vec3 Normal;
        in float Intensity;

        uniform vec3 viewPos;
        uniform float opacity;
        uniform float minIntensity;

        out vec4 FragColor;

        // Blue -> Yellow -> Red gradient
        vec3 intensityToColor(float t) {
            // Remap from [minIntensity, 1] to [0, 1] for color
            t = clamp((t - minIntensity) / (1.0 - minIntensity), 0.0, 1.0);

            if (t < 0.5) {
                float s = t * 2.0;
                return mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 0.0), s);
            } else {
                float s = (t - 0.5) * 2.0;
                return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), s);
            }
        }

        void main() {
            if (Intensity < minIntensity) {
                discard;
            }

            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);

            // Simple lighting
            float ambient = 0.4;
            float diffuse = max(dot(norm, viewDir), 0.0) * 0.6;
            float lighting = ambient + diffuse;

            vec3 color = intensityToColor(Intensity);
            vec3 result = color * lighting;

            // Alpha based on intensity for smooth fadeout
            float alpha = opacity * clamp((Intensity - minIntensity) / (0.3 - minIntensity), 0.3, 1.0);
            FragColor = vec4(result, alpha);
        }
    )";
}

HeatMapRenderer::~HeatMapRenderer() {
    // OpenGL cleanup should be done via cleanup() before context destruction
}

bool HeatMapRenderer::initialize() {
    if (initialized_) {
        return true;
    }

    if (!QOpenGLContext::currentContext()) {
        qWarning() << "HeatMapRenderer::initialize - No OpenGL context available";
        return false;
    }

    if (!initializeOpenGLFunctions()) {
        qCritical() << "HeatMapRenderer: Failed to initialize OpenGL functions!";
        return false;
    }

    GLUtils::clearGLErrors();

    setupShaders();
    generateSphereMesh();

    vao_.create();
    vao_.bind();
    vao_.release();

    GLUtils::checkGLError("HeatMapRenderer::initialize");

    initialized_ = true;
    return true;
}

void HeatMapRenderer::cleanup() {
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

void HeatMapRenderer::setupShaders() {
    if (vertexShaderSource_.empty() || fragmentShaderSource_.empty()) {
        qCritical() << "HeatMapRenderer: Shader sources not initialized!";
        return;
    }

    shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_.data())) {
        qCritical() << "HeatMapRenderer: Failed to compile vertex shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_.data())) {
        qCritical() << "HeatMapRenderer: Failed to compile fragment shader:" << shaderProgram_->log();
        return;
    }

    if (!shaderProgram_->link()) {
        qCritical() << "HeatMapRenderer: Failed to link shader program:" << shaderProgram_->log();
        return;
    }
}

void HeatMapRenderer::setSphereRadius(float radius) {
    if (sphereRadius_ != radius) {
        sphereRadius_ = radius;
        geometryDirty_ = true;
    }
}

void HeatMapRenderer::generateSphereMesh() {
    vertices_.clear();
    indices_.clear();

    int latDivisions = kHeatMapLatSegments;
    int lonDivisions = kHeatMapLonSegments;

    // Slightly larger radius to float above the sphere surface
    float renderRadius = sphereRadius_ * kHeatMapRadiusOffset;

    // Generate vertices (position + normal, 6 floats per vertex)
    for (int lat = 0; lat <= latDivisions; lat++) {
        float phi = kPiF * float(lat) / float(latDivisions);
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        for (int lon = 0; lon <= lonDivisions; lon++) {
            float theta = kTwoPiF * float(lon) / float(lonDivisions);
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            // Position (Z-up convention)
            float x = renderRadius * sinPhi * cosTheta;
            float y = renderRadius * sinPhi * sinTheta;
            float z = renderRadius * cosPhi;

            // Normal (normalized position for a sphere, pointing outward)
            float nx = sinPhi * cosTheta;
            float ny = sinPhi * sinTheta;
            float nz = cosPhi;

            // Add vertex data (position + normal)
            vertices_.push_back(x);
            vertices_.push_back(y);
            vertices_.push_back(z);
            vertices_.push_back(nx);
            vertices_.push_back(ny);
            vertices_.push_back(nz);
        }
    }

    // Generate indices
    for (int lat = 0; lat < latDivisions; lat++) {
        for (int lon = 0; lon < lonDivisions; lon++) {
            int first = lat * (lonDivisions + 1) + lon;
            int second = first + lonDivisions + 1;

            // First triangle
            indices_.push_back(first);
            indices_.push_back(second);
            indices_.push_back(first + 1);

            // Second triangle
            indices_.push_back(second);
            indices_.push_back(second + 1);
            indices_.push_back(first + 1);
        }
    }

    // Initialize intensities array (one per vertex)
    vertexCount_ = (latDivisions + 1) * (lonDivisions + 1);
    intensities_.resize(vertexCount_, 0.0f);
    indexCount_ = static_cast<int>(indices_.size());

    geometryDirty_ = true;
}

void HeatMapRenderer::clearBins() {
    std::fill(binIntensity_.begin(), binIntensity_.end(), 0.0f);
    std::fill(binHitCount_.begin(), binHitCount_.end(), 0);
}

int HeatMapRenderer::getBinIndex(float theta, float phi) const {
    // theta: azimuth [-pi, pi] -> [0, lonBins)
    // phi: elevation [-pi/2, pi/2] -> [0, latBins)
    //   +pi/2 (up/north pole) -> latBin 0
    //   -pi/2 (down/south pole) -> latBin latBins-1

    // Normalize theta to [0, 2*pi)
    float normTheta = theta;
    if (normTheta < 0) normTheta += kTwoPiF;

    // Convert to bin indices
    int lonBin = static_cast<int>(normTheta / kTwoPiF * lonBins_);
    // Map elevation +pi/2 to lat 0 (north), -pi/2 to lat max (south)
    int latBin = static_cast<int>((kPiF / 2.0f - phi) / kPiF * latBins_);

    // Clamp to valid range
    lonBin = std::clamp(lonBin, 0, lonBins_ - 1);
    latBin = std::clamp(latBin, 0, latBins_ - 1);

    return latBin * lonBins_ + lonBin;
}

void HeatMapRenderer::accumulateHit(const RCS::HitResult& hit) {
    // Skip misses
    if (hit.hitPoint.w() < 0.0f) {
        return;
    }

    float intensity = hit.reflection.w();
    if (intensity < minIntensity_) {
        return;
    }

    // Get reflection direction (where the energy goes)
    QVector3D dir = hit.reflection.toVector3D().normalized();

    // Convert direction to spherical coordinates
    float theta = atan2(dir.y(), dir.x());                          // azimuth [-pi, pi]
    float phi = asin(std::clamp(dir.z(), -1.0f, 1.0f));            // elevation [-pi/2, pi/2]

    int bin = getBinIndex(theta, phi);
    if (bin >= 0 && bin < static_cast<int>(binIntensity_.size())) {
        binIntensity_[bin] += intensity;
        binHitCount_[bin]++;
    }
}

void HeatMapRenderer::getVertexSphericalCoords(int vertexIndex, float& theta, float& phi) const {
    int latDivisions = kHeatMapLatSegments;
    int lonDivisions = kHeatMapLonSegments;

    int lat = vertexIndex / (lonDivisions + 1);
    int lon = vertexIndex % (lonDivisions + 1);

    // In sphere mesh: lat=0 is north pole (z=+r), lat=max is south pole (z=-r)
    // Convert to elevation: lat=0 -> +pi/2, lat=max -> -pi/2
    phi = kPiF / 2.0f - kPiF * float(lat) / float(latDivisions);

    // theta ranges from 0 to 2*pi
    theta = kTwoPiF * float(lon) / float(lonDivisions);
    if (theta > kPiF) theta -= kTwoPiF;  // Normalize to [-pi, pi]
}

void HeatMapRenderer::computeVertexIntensities() {
    int latDivisions = kHeatMapLatSegments;
    int lonDivisions = kHeatMapLonSegments;

    for (int vertexIdx = 0; vertexIdx < vertexCount_; vertexIdx++) {
        float theta, phi;
        getVertexSphericalCoords(vertexIdx, theta, phi);

        int bin = getBinIndex(theta, phi);

        // Get intensity from bin (average if hits > 0)
        float intensity = 0.0f;
        if (bin >= 0 && bin < static_cast<int>(binHitCount_.size()) && binHitCount_[bin] > 0) {
            intensity = binIntensity_[bin] / static_cast<float>(binHitCount_[bin]);
        }

        // Optional: bilinear interpolation for smoother appearance
        // For now, use direct bin lookup

        intensities_[vertexIdx] = intensity;
    }
}

void HeatMapRenderer::updateFromHits(const std::vector<RCS::HitResult>& hits, float sphereRadius) {
    // Update sphere radius if changed
    if (sphereRadius != sphereRadius_) {
        sphereRadius_ = sphereRadius;
        generateSphereMesh();
    }

    // Clear and accumulate
    clearBins();
    for (const auto& hit : hits) {
        accumulateHit(hit);
    }

    // Compute per-vertex intensities
    computeVertexIntensities();

    geometryDirty_ = true;
}

void HeatMapRenderer::uploadGeometry() {
    if (!initialized_ || !QOpenGLContext::currentContext()) {
        return;
    }

    if (vertices_.empty() || indices_.empty()) {
        return;
    }

    vao_.bind();

    // Create interleaved buffer: position (3) + normal (3) + intensity (1) = 7 floats per vertex
    std::vector<float> interleavedData;
    interleavedData.reserve(vertexCount_ * 7);

    for (int i = 0; i < vertexCount_; i++) {
        // Position (3 floats)
        interleavedData.push_back(vertices_[i * 6 + 0]);
        interleavedData.push_back(vertices_[i * 6 + 1]);
        interleavedData.push_back(vertices_[i * 6 + 2]);
        // Normal (3 floats)
        interleavedData.push_back(vertices_[i * 6 + 3]);
        interleavedData.push_back(vertices_[i * 6 + 4]);
        interleavedData.push_back(vertices_[i * 6 + 5]);
        // Intensity (1 float)
        interleavedData.push_back(intensities_[i]);
    }

    // VBO
    if (vboId_ == 0) {
        glGenBuffers(1, &vboId_);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBufferData(GL_ARRAY_BUFFER,
                 interleavedData.size() * sizeof(float),
                 interleavedData.data(),
                 GL_DYNAMIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Intensity attribute (location 2)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
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

void HeatMapRenderer::uploadIntensities() {
    // For now, we rebuild the entire VBO in uploadGeometry()
    // This could be optimized to update only the intensity portion
    geometryDirty_ = true;
}

void HeatMapRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view,
                              const QMatrix4x4& model) {
    if (!visible_ || !shaderProgram_ || !initialized_) {
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
    glDisable(GL_CULL_FACE);  // Show both sides

    shaderProgram_->bind();
    shaderProgram_->setUniformValue("projection", projection);
    shaderProgram_->setUniformValue("view", view);
    shaderProgram_->setUniformValue("model", model);
    shaderProgram_->setUniformValue("opacity", opacity_);
    shaderProgram_->setUniformValue("minIntensity", minIntensity_);

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

QVector3D HeatMapRenderer::intensityToColor(float intensity) {
    // Blue -> Yellow -> Red gradient (same as ReflectionRenderer)
    if (intensity < kLobeColorThreshold) {
        float t = intensity / kLobeColorThreshold;
        return QVector3D(
            Colors::kLobeLowIntensity[0] * (1.0f - t) + Colors::kLobeMidIntensity[0] * t,
            Colors::kLobeLowIntensity[1] * (1.0f - t) + Colors::kLobeMidIntensity[1] * t,
            Colors::kLobeLowIntensity[2] * (1.0f - t) + Colors::kLobeMidIntensity[2] * t
        );
    } else {
        float t = (intensity - kLobeColorThreshold) / (1.0f - kLobeColorThreshold);
        return QVector3D(
            Colors::kLobeMidIntensity[0] * (1.0f - t) + Colors::kLobeHighIntensity[0] * t,
            Colors::kLobeMidIntensity[1] * (1.0f - t) + Colors::kLobeHighIntensity[1] * t,
            Colors::kLobeMidIntensity[2] * (1.0f - t) + Colors::kLobeHighIntensity[2] * t
        );
    }
}
