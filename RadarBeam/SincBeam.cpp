// ---- SincBeam.cpp ----
// Sinc² beam pattern with intensity falloff and side lobes

#include "SincBeam.h"
#include "Constants.h"
#include <QOpenGLContext>
#include <cmath>

using namespace RadarSim::Constants;

SincBeam::SincBeam(float sphereRadius, float beamWidthDegrees)
    : RadarBeam(sphereRadius, beamWidthDegrees)
{
    // Don't call initialize() here - requires valid OpenGL context
    // BeamController::createBeam() will call initialize() after construction
}

SincBeam::~SincBeam() {
    // Base class destructor handles cleanup
}

float SincBeam::getSincSquaredIntensity(float theta, float thetaMax) {
    if (thetaMax <= 0.0f) return 1.0f;

    // x = π * θ / θmax
    float x = kPiF * theta / thetaMax;

    // sinc(0) = 1 (avoid division by zero)
    if (std::abs(x) < 0.0001f) {
        return 1.0f;
    }

    // sinc²(x) = [sin(x) / x]²
    float sinc = std::sin(x) / x;
    return sinc * sinc;
}

void SincBeam::setupShaders() {
    // Custom shaders with per-vertex intensity attribute
    beamVertexShaderSource_ = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in float aIntensity;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec3 LocalPos;
        out float Intensity;

        void main() {
            LocalPos = aPos;
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            Intensity = aIntensity;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    beamFragmentShaderSource_ = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;
        in vec3 LocalPos;
        in float Intensity;

        uniform vec3 beamColor;
        uniform vec3 sideLobeColor;
        uniform float opacity;
        uniform vec3 viewPos;

        // GPU shadow map uniforms
        uniform sampler2D shadowMap;
        uniform bool gpuShadowEnabled;
        uniform vec3 radarPos;
        uniform vec3 beamAxis;
        uniform float beamWidthRad;
        uniform int numRings;

        out vec4 FragColor;

        // Convert LOCAL position to shadow map UV coordinates
        vec2 worldToShadowMapUV(vec3 localPos) {
            vec3 toFrag = normalize(localPos - radarPos);

            // Elevation from beam axis
            float halfAngle = beamWidthRad * 0.5;
            float cosElev = dot(toFrag, beamAxis);
            float elevation = acos(clamp(cosElev, -1.0, 1.0));
            float elevNorm = elevation / halfAngle;

            // Azimuth around beam axis
            vec3 perpComponent = toFrag - beamAxis * cosElev;
            float perpLen = length(perpComponent);
            if (perpLen < 0.001) return vec2(0.0, elevNorm);
            perpComponent /= perpLen;

            vec3 up = abs(beamAxis.z) < 0.99 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
            vec3 right = normalize(cross(beamAxis, up));
            up = normalize(cross(right, beamAxis));

            float azimuth = atan(dot(perpComponent, up), dot(perpComponent, right));
            if (azimuth < 0.0) azimuth += 2.0 * 3.14159265;
            float azNorm = azimuth / (2.0 * 3.14159265);

            float uvY = elevNorm - 0.5 / float(numRings);
            return vec2(azNorm, uvY);
        }

        void main() {
            // GPU shadow map lookup
            if (gpuShadowEnabled) {
                vec2 uv = worldToShadowMapUV(LocalPos);
                if (uv.y >= 0.0 && uv.y <= 1.0) {
                    float hitDistance = texture(shadowMap, uv).r;
                    if (hitDistance > 0.0) {
                        float fragDistance = length(LocalPos - radarPos);
                        if (fragDistance > hitDistance) {
                            discard;
                        }
                    }
                }
            }

            // Intensity-based color interpolation
            // Main lobe (Intensity > 0.1): use beamColor
            // Side lobes (Intensity < 0.1): blend toward sideLobeColor
            float lobeBlend = smoothstep(0.0, 0.15, Intensity);
            vec3 intensityColor = mix(sideLobeColor, beamColor, lobeBlend);

            // Fresnel effect for viewing angle
            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);
            float fresnel = 0.3 + 0.7 * pow(1.0 - abs(dot(norm, viewDir)), 2.0);

            // Rim lighting for edge visibility
            float rim = 1.0 - abs(dot(norm, viewDir));
            rim = smoothstep(0.4, 0.8, rim);

            // Final alpha modulated by intensity
            // Ensure some minimum visibility even for side lobes
            float intensityAlpha = max(Intensity, 0.02);
            float finalAlpha = opacity * (fresnel + rim * 0.3) * intensityAlpha;
            finalAlpha = clamp(finalAlpha, 0.01, 1.0);

            FragColor = vec4(intensityColor, finalAlpha);
        }
    )";

    // Call base class to compile shaders
    RadarBeam::setupShaders();
}

void SincBeam::createBeamGeometry() {
    if (currentRadarPosition_.isNull()) {
        return;
    }

    // Calculate beam direction and end point
    QVector3D direction = calculateBeamDirection(currentRadarPosition_);
    QVector3D endPoint = calculateOppositePoint(currentRadarPosition_, direction);

    // Beam length from radar to sphere surface
    float beamLength = (endPoint - currentRadarPosition_).length();

    // Main lobe radius (at the base, for main lobe only)
    float halfAngleRad = beamWidthDegrees_ * kDegToRadF / 2.0f;
    float mainLobeRadius = tan(halfAngleRad) * beamLength;

    generateSincBeamVertices(currentRadarPosition_, direction, beamLength, mainLobeRadius);
}

void SincBeam::generateSincBeamVertices(const QVector3D& apex,
                                         const QVector3D& direction,
                                         float length,
                                         float mainLobeRadius) {
    vertices_.clear();
    indices_.clear();

    const int azimuthSegments = kBeamConeSegments;  // 32 segments around
    const int radialSegments = kSincBeamRadialSegments;  // 64 rings from center to edge

    // Extend geometry to cover side lobes (2.5× main lobe)
    float extendedRadius = mainLobeRadius * kSincSideLobeMultiplier;
    float halfAngleRad = beamWidthDegrees_ * kDegToRadF / 2.0f;

    // Build coordinate frame
    QVector3D normDir = direction.normalized();
    QVector3D up(0.0f, 1.0f, 0.0f);
    if (qAbs(QVector3D::dotProduct(normDir, up)) > kGimbalLockThreshold) {
        up = QVector3D(1.0f, 0.0f, 0.0f);
    }
    QVector3D right = QVector3D::crossProduct(normDir, up).normalized();
    up = QVector3D::crossProduct(right, normDir).normalized();

    // Helper to add a vertex with 7 floats: [x, y, z, nx, ny, nz, intensity]
    auto addVertex = [this](const QVector3D& pos, const QVector3D& normal, float intensity) {
        vertices_.push_back(pos.x());
        vertices_.push_back(pos.y());
        vertices_.push_back(pos.z());
        vertices_.push_back(normal.x());
        vertices_.push_back(normal.y());
        vertices_.push_back(normal.z());
        vertices_.push_back(intensity);
    };

    // Vertex 0: Apex (radar position) with intensity = 1.0
    addVertex(apex, normDir, 1.0f);

    // Generate concentric rings from center to extended edge
    for (int ring = 1; ring <= radialSegments; ++ring) {
        float t = float(ring) / float(radialSegments);
        float ringRadius = extendedRadius * t;

        // Calculate angle from beam axis at this radius
        float theta = atan(ringRadius / length);
        float intensity = getSincSquaredIntensity(theta, halfAngleRad);

        for (int seg = 0; seg < azimuthSegments; ++seg) {
            float azimuth = kTwoPiF * float(seg) / float(azimuthSegments);

            // Point on conceptual base plane
            QVector3D offset = right * (ringRadius * cos(azimuth))
                             + up * (ringRadius * sin(azimuth));
            QVector3D basePoint = apex + normDir * length + offset;

            // Project onto sphere surface
            QVector3D surfacePoint = basePoint.normalized() * sphereRadius_;

            // Calculate normal (blend between direction and radial)
            QVector3D toBase = (basePoint - apex).normalized();
            QVector3D normal = (normDir * 0.2f + toBase * 0.8f).normalized();

            addVertex(surfacePoint, normal, intensity);
        }
    }

    // Generate indices
    // Connect apex (vertex 0) to first ring
    for (int seg = 0; seg < azimuthSegments; ++seg) {
        int next = (seg + 1) % azimuthSegments;
        indices_.push_back(0);
        indices_.push_back(static_cast<unsigned int>(1 + seg));
        indices_.push_back(static_cast<unsigned int>(1 + next));
    }

    // Connect ring to ring
    for (int ring = 0; ring < radialSegments - 1; ++ring) {
        int innerStart = 1 + ring * azimuthSegments;
        int outerStart = 1 + (ring + 1) * azimuthSegments;

        for (int seg = 0; seg < azimuthSegments; ++seg) {
            int next = (seg + 1) % azimuthSegments;

            // Two triangles per quad
            indices_.push_back(static_cast<unsigned int>(innerStart + seg));
            indices_.push_back(static_cast<unsigned int>(outerStart + seg));
            indices_.push_back(static_cast<unsigned int>(innerStart + next));

            indices_.push_back(static_cast<unsigned int>(innerStart + next));
            indices_.push_back(static_cast<unsigned int>(outerStart + seg));
            indices_.push_back(static_cast<unsigned int>(outerStart + next));
        }
    }
}

void SincBeam::uploadGeometryToGPU() {
    if (vertices_.empty() || indices_.empty()) {
        return;
    }

    // Check if we have a valid GL context
    if (!QOpenGLContext::currentContext()) {
        geometryDirty_ = true;
        return;
    }

    // Ensure VAO exists and bind it
    if (!beamVAO_.isCreated()) {
        beamVAO_.create();
    }
    beamVAO_.bind();

    // Create VBO if needed
    if (vboId_ == 0) {
        glGenBuffers(1, &vboId_);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBufferData(GL_ARRAY_BUFFER,
        vertices_.size() * sizeof(float),
        vertices_.data(),
        GL_DYNAMIC_DRAW);

    // Setup vertex attributes with 7-float stride (pos + normal + intensity)
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Intensity attribute (location 2) - NEW for SincBeam
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Create EBO if needed
    if (eboId_ == 0) {
        glGenBuffers(1, &eboId_);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices_.size() * sizeof(unsigned int),
        indices_.data(),
        GL_DYNAMIC_DRAW);

    beamVAO_.release();
    geometryDirty_ = false;
}

void SincBeam::render(QOpenGLShaderProgram* program, const QMatrix4x4& projection,
                      const QMatrix4x4& view, const QMatrix4x4& model) {
    if (!visible_ || vertices_.empty()) {
        return;
    }

    // Check if OpenGL resources are valid
    if (!beamVAO_.isCreated() || !beamShaderProgram_) {
        qWarning() << "SincBeam::render called with invalid OpenGL resources";
        return;
    }

    // Upload geometry if it was deferred
    if (geometryDirty_) {
        uploadGeometryToGPU();
    }

    // Verify buffers exist
    if (vboId_ == 0 || eboId_ == 0) {
        qWarning() << "SincBeam::render - buffers not created";
        return;
    }

    if (indices_.empty()) {
        return;
    }

    // Save current OpenGL state
    GLboolean depthMaskPrevious;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskPrevious);
    GLint blendSrcPrevious, blendDstPrevious;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcPrevious);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstPrevious);
    GLboolean blendPrevious = glIsEnabled(GL_BLEND);

    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth testing but disable depth writing for transparent objects
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);

    glDisable(GL_STENCIL_TEST);

    // Enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Use the beam shader program
    beamShaderProgram_->bind();

    // Set standard uniforms
    beamShaderProgram_->setUniformValue("projection", projection);
    beamShaderProgram_->setUniformValue("view", view);
    beamShaderProgram_->setUniformValue("model", model);
    beamShaderProgram_->setUniformValue("beamColor", color_);
    beamShaderProgram_->setUniformValue("opacity", opacity_);

    // Set sideLobeColor uniform (SincBeam-specific)
    QVector3D sideLobeColor(Colors::kSincSideLobeColor[0],
                           Colors::kSincSideLobeColor[1],
                           Colors::kSincSideLobeColor[2]);
    beamShaderProgram_->setUniformValue("sideLobeColor", sideLobeColor);

    // Extract camera position from view matrix for Fresnel effect
    QMatrix4x4 inverseView = view.inverted();
    QVector3D cameraPos = inverseView.column(3).toVector3D();
    beamShaderProgram_->setUniformValue("viewPos", cameraPos);

    // Set GPU shadow map uniforms
    beamShaderProgram_->setUniformValue("radarPos", currentRadarPosition_);
    beamShaderProgram_->setUniformValue("gpuShadowEnabled", gpuShadowEnabled_);
    beamShaderProgram_->setUniformValue("beamAxis", beamAxis_);
    beamShaderProgram_->setUniformValue("beamWidthRad", beamWidthRadians_);
    beamShaderProgram_->setUniformValue("numRings", numRings_);

    // Bind shadow map texture if enabled
    if (gpuShadowEnabled_ && gpuShadowMapTexture_ != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gpuShadowMapTexture_);
        beamShaderProgram_->setUniformValue("shadowMap", 0);
    }

    // Bind VAO and draw
    beamVAO_.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);

    // Release resources
    if (beamVAO_.isCreated() && beamVAO_.objectId() != 0) {
        beamVAO_.release();
    }
    beamShaderProgram_->release();

    // Restore previous OpenGL state
    glDepthMask(depthMaskPrevious);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    if (!blendPrevious) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(blendSrcPrevious, blendDstPrevious);
}
