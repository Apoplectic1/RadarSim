// ---- SincBeam.cpp ----
// Airy beam pattern (circular aperture) with intensity falloff and side lobes

#include "SincBeam.h"
#include "Constants.h"
#include <QOpenGLContext>
#include <cmath>

using namespace RadarSim::Constants;

// First null of Bessel J1 occurs at x ≈ 3.8317
constexpr float kAiryFirstNull = 3.8317f;

SincBeam::SincBeam(float sphereRadius, float beamWidthDegrees)
    : RadarBeam(sphereRadius, beamWidthDegrees)
{
    // Don't call initialize() here - requires valid OpenGL context
    // BeamController::createBeam() will call initialize() after construction
}

SincBeam::~SincBeam() {
    // Base class destructor handles cleanup
}

// Bessel function J₁(x) - polynomial approximation from Numerical Recipes
float SincBeam::besselJ1(float x) {
    float ax = std::abs(x);

    // For very small x, J1(x) ≈ x/2
    if (ax < 0.001f) {
        return x * 0.5f;
    }

    if (ax < 8.0f) {
        // Polynomial approximation for |x| < 8
        float y = x * x;
        float ans1 = x * (72362614232.0f + y * (-7895059235.0f + y * (242396853.1f
            + y * (-2972611.439f + y * (15704.48260f + y * (-30.16036606f))))));
        float ans2 = 144725228442.0f + y * (2300535178.0f + y * (18583304.74f
            + y * (99447.43394f + y * (376.9991397f + y * 1.0f))));
        return ans1 / ans2;
    } else {
        // Asymptotic approximation for |x| >= 8
        float z = 8.0f / ax;
        float y = z * z;
        float xx = ax - 2.356194491f;  // x - 3π/4
        float ans1 = 1.0f + y * (0.183105e-2f + y * (-0.3516396496e-4f + y * (0.2457520174e-5f
            + y * (-0.240337019e-6f))));
        float ans2 = 0.04687499995f + y * (-0.2002690873e-3f + y * (0.8449199096e-5f
            + y * (-0.88228987e-6f + y * 0.105787412e-6f)));
        float ans = std::sqrt(0.636619772f / ax) * (std::cos(xx) * ans1 - z * std::sin(xx) * ans2);
        return (x < 0.0f) ? -ans : ans;
    }
}

// Airy pattern: [2·J₁(x)/x]² where x = kAiryFirstNull * θ/θmax
// This places the first null at θ = θmax
float SincBeam::getAiryIntensity(float theta, float thetaMax) {
    if (thetaMax <= 0.0f) return 1.0f;

    // Scale so first null occurs at theta = thetaMax
    float x = kAiryFirstNull * theta / thetaMax;

    // At center (x=0), Airy pattern = 1.0
    if (std::abs(x) < 0.0001f) {
        return 1.0f;
    }

    // Airy pattern: [2·J₁(x)/x]²
    float j1 = besselJ1(x);
    float airy = 2.0f * j1 / x;
    return airy * airy;
}

// Legacy sinc² calculation (kept for RCS ray weighting compatibility)
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

        // Footprint-only mode uniforms
        uniform bool footprintOnly;
        uniform float sphereRadius;

        // Visibility constants (passed from C++)
        uniform float fresnelBase;
        uniform float fresnelRange;
        uniform float rimLow;
        uniform float rimHigh;
        uniform float rimStrength;
        uniform float intensityAlphaMin;
        uniform float opacityMult;
        uniform float alphaMin;
        uniform float alphaMax;

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
            // Footprint-only mode: only show fragments on/near the sphere surface
            if (footprintOnly) {
                float distFromOrigin = length(LocalPos);
                float surfaceThreshold = sphereRadius * 0.05;  // 5% tolerance
                if (distFromOrigin < sphereRadius - surfaceThreshold) {
                    discard;  // Fragment is inside sphere, not on surface
                }
            }

            // Track intersection highlight
            float intersectionGlow = 0.0;

            // GPU shadow map lookup
            if (gpuShadowEnabled) {
                vec2 uv = worldToShadowMapUV(LocalPos);
                if (uv.y >= 0.0 && uv.y <= 1.0) {
                    float hitDistance = texture(shadowMap, uv).r;
                    if (hitDistance > 0.0) {
                        float fragDistance = length(LocalPos - radarPos);

                        // Discard fragments behind the target
                        if (fragDistance > hitDistance) {
                            discard;
                        }

                        // Intersection highlight: glow when close to hit surface
                        // Highlight zone is ~5% of hit distance (adjustable)
                        float highlightZone = hitDistance * 0.08;
                        float distToHit = hitDistance - fragDistance;
                        if (distToHit < highlightZone) {
                            // Smooth falloff from 1.0 at surface to 0.0 at zone edge
                            intersectionGlow = 1.0 - (distToHit / highlightZone);
                            intersectionGlow = intersectionGlow * intersectionGlow; // Quadratic for sharper edge
                        }
                    }
                }
            }

            // Intensity-based color - use intensity directly for brightness
            // Nulls (Intensity ~ 0) should appear dark, peaks should be bright
            // Use pow(x, 0.4) for brighter overall appearance while preserving pattern
            float brightnessFactor = pow(Intensity, 0.4);

            // Color: interpolate from dark (nulls) to beamColor (peaks)
            // Boost the low end for better visibility
            vec3 intensityColor = mix(sideLobeColor * 0.5, beamColor, brightnessFactor);

            // Apply intersection highlight - bright white/yellow glow at target surface
            vec3 highlightColor = vec3(1.0, 0.95, 0.7); // Warm white highlight
            intensityColor = mix(intensityColor, highlightColor, intersectionGlow * 0.8);

            // Fresnel effect for viewing angle
            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);
            float fresnel = fresnelBase + fresnelRange * pow(1.0 - abs(dot(norm, viewDir)), 2.0);

            // Rim lighting for edge visibility
            float rim = 1.0 - abs(dot(norm, viewDir));
            rim = smoothstep(rimLow, rimHigh, rim);

            // Final alpha: higher minimum for better visibility
            // Boost alpha at intersection for more visible highlight
            float intensityAlpha = mix(intensityAlphaMin, 1.0, pow(Intensity, 0.4));
            float finalAlpha = opacity * opacityMult * (fresnel + rim * rimStrength) * intensityAlpha;
            finalAlpha = clamp(finalAlpha + intersectionGlow * 0.4, alphaMin, alphaMax);

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
        float t = float(ring) / float(radialSegments);  // 0 to 1 from center to edge
        float ringRadius = extendedRadius * t;

        // Calculate angle from beam axis at this radius
        float theta = atan(ringRadius / length);
        float airyIntensity = getAiryIntensity(theta, halfAngleRad);

        // Edge fade: smooth falloff in outer 25% of geometry (3x to 4x main lobe)
        // This prevents the hard cutoff at the geometry boundary
        float edgeFade = 1.0f;
        if (t > 0.75f) {
            edgeFade = 1.0f - (t - 0.75f) / 0.25f;  // Linear fade from 1 to 0
            edgeFade = edgeFade * edgeFade;  // Quadratic for smoother fade
        }

        float intensity = airyIntensity * edgeFade;

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

    // SincBeam: disable face culling because extended geometry (2.5x for side lobes)
    // wraps around sphere, causing triangle winding to flip on front hemisphere
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);

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

    // Set footprint-only mode uniforms
    beamShaderProgram_->setUniformValue("footprintOnly", footprintOnly_);
    beamShaderProgram_->setUniformValue("sphereRadius", sphereRadius_);

    // Set visibility constants from Constants.h
    beamShaderProgram_->setUniformValue("fresnelBase", kSincFresnelBase);
    beamShaderProgram_->setUniformValue("fresnelRange", kSincFresnelRange);
    beamShaderProgram_->setUniformValue("rimLow", kSincRimLow);
    beamShaderProgram_->setUniformValue("rimHigh", kSincRimHigh);
    beamShaderProgram_->setUniformValue("rimStrength", kSincRimStrength);
    beamShaderProgram_->setUniformValue("intensityAlphaMin", kSincIntensityAlphaMin);
    beamShaderProgram_->setUniformValue("opacityMult", kSincOpacityMult);
    beamShaderProgram_->setUniformValue("alphaMin", kSincAlphaMin);
    beamShaderProgram_->setUniformValue("alphaMax", kSincAlphaMax);

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
    glDisable(GL_STENCIL_TEST);
    if (!blendPrevious) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(blendSrcPrevious, blendDstPrevious);
}
