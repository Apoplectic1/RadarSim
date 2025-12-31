// RCSCompute.cpp - GPU ray tracing implementation
#include "RCSCompute.h"
#include "GLUtils.h"
#include "Constants.h"
#include <QOpenGLContext>
#include <QDebug>
#include <cmath>
#include <cstring>

using namespace RS::Constants;

namespace RCS {

// Compute shader source: Ray Generation
static const char* rayGenShaderSource = R"(
#version 430 core
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct Ray {
    vec4 origin;      // xyz = origin, w = tmin
    vec4 direction;   // xyz = direction, w = tmax
};

layout(std430, binding = 0) buffer RayBuffer {
    Ray rays[];
};

uniform vec3 radarPosition;
uniform vec3 beamDirection;
uniform float beamWidthRad;
uniform float maxDistance;
uniform int numRays;
uniform int raysPerRing;
uniform int numRings;

void main() {
    uint rayId = gl_GlobalInvocationID.x;
    if (rayId >= numRays) return;

    // Calculate ring and position within ring
    uint ring = rayId / raysPerRing;
    uint posInRing = rayId % raysPerRing;

    // Angle within beam cone (0 = center, beamWidthRad/2 = edge)
    // beamWidthRad is the full cone angle, so half-angle is the max from center
    float halfAngle = beamWidthRad * 0.5;
    float ringAngle = halfAngle * float(ring + 1) / float(numRings);
    float azimuth = 2.0 * 3.14159265 * float(posInRing) / float(raysPerRing);

    // Calculate ray direction using beam coordinate system
    vec3 forward = normalize(beamDirection);

    // Choose up vector avoiding gimbal lock
    vec3 up = abs(forward.z) < 0.99 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(forward, up));
    up = normalize(cross(right, forward));

    // Cone sampling
    float sinAngle = sin(ringAngle);
    float cosAngle = cos(ringAngle);

    vec3 localDir = vec3(
        sinAngle * cos(azimuth),
        sinAngle * sin(azimuth),
        cosAngle
    );

    vec3 worldDir = localDir.x * right + localDir.y * up + localDir.z * forward;

    rays[rayId].origin = vec4(radarPosition, 0.001);      // tmin = 0.001
    rays[rayId].direction = vec4(normalize(worldDir), maxDistance);  // tmax
}
)";

// Compute shader source: BVH Traversal and Ray-Triangle Intersection
static const char* traceShaderSource = R"(
#version 430 core
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct Ray {
    vec4 origin;
    vec4 direction;
};

struct BVHNode {
    vec4 boundsMin;  // w = left child (or -firstTri-1 if leaf)
    vec4 boundsMax;  // w = right child (or triCount if leaf)
};

struct HitResult {
    vec4 hitPoint;   // xyz = position, w = distance
    vec4 normal;     // xyz = normal, w = material
    vec4 reflection; // xyz = reflection direction, w = intensity
    uint triangleId;
    uint rayId;
    uint targetId;
    float rcsContribution;
};

layout(std430, binding = 0) readonly buffer RayBuffer { Ray rays[]; };
layout(std430, binding = 1) readonly buffer BVHBuffer { BVHNode nodes[]; };
layout(std430, binding = 2) readonly buffer TriBuffer { vec4 triangles[]; };  // 3 vec4s per triangle
layout(std430, binding = 3) buffer HitBuffer { HitResult hits[]; };
layout(std430, binding = 4) buffer CounterBuffer { uint hitCounter; };

uniform int numRays;
uniform int numNodes;

// Ray-AABB intersection
bool intersectAABB(vec3 origin, vec3 invDir, vec3 bmin, vec3 bmax, float tmax) {
    vec3 t1 = (bmin - origin) * invDir;
    vec3 t2 = (bmax - origin) * invDir;
    vec3 tmin_v = min(t1, t2);
    vec3 tmax_v = max(t1, t2);
    float tenter = max(max(tmin_v.x, tmin_v.y), tmin_v.z);
    float texit = min(min(tmax_v.x, tmax_v.y), tmax_v.z);
    return tenter <= texit && texit >= 0.0 && tenter < tmax;
}

// Ray-triangle intersection (Moller-Trumbore)
bool intersectTriangle(vec3 origin, vec3 dir, vec3 v0, vec3 v1, vec3 v2,
                       out float t, out vec3 normal) {
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 h = cross(dir, e2);
    float a = dot(e1, h);
    if (abs(a) < 1e-8) return false;

    float f = 1.0 / a;
    vec3 s = origin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return false;

    vec3 q = cross(s, e1);
    float v = f * dot(dir, q);
    if (v < 0.0 || u + v > 1.0) return false;

    t = f * dot(e2, q);
    if (t < 0.001) return false;

    normal = normalize(cross(e1, e2));
    return true;
}

void main() {
    uint rayId = gl_GlobalInvocationID.x;
    if (rayId >= numRays) return;

    Ray ray = rays[rayId];
    vec3 origin = ray.origin.xyz;
    vec3 dir = ray.direction.xyz;
    vec3 invDir = 1.0 / dir;
    float tmax = ray.direction.w;

    // Initialize hit result
    HitResult hit;
    hit.hitPoint = vec4(0.0, 0.0, 0.0, -1.0);  // -1 = no hit
    hit.normal = vec4(0.0);
    hit.reflection = vec4(0.0);
    hit.triangleId = 0xFFFFFFFF;
    hit.rayId = rayId;
    hit.targetId = 0u;  // Single target for now
    hit.rcsContribution = 0.0;

    if (numNodes == 0) {
        hits[rayId] = hit;
        return;
    }

    // Stack-based BVH traversal
    int stack[64];
    int stackPtr = 0;
    stack[stackPtr++] = 0;  // Start at root

    float closestT = tmax;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        if (nodeIdx < 0 || nodeIdx >= numNodes) continue;

        BVHNode node = nodes[nodeIdx];

        if (!intersectAABB(origin, invDir, node.boundsMin.xyz, node.boundsMax.xyz, closestT)) {
            continue;
        }

        int leftInfo = int(node.boundsMin.w);

        if (leftInfo < 0) {
            // Leaf node - test triangles
            int firstTri = -leftInfo - 1;
            int numTris = int(node.boundsMax.w);

            for (int i = 0; i < numTris; i++) {
                int triIdx = (firstTri + i) * 3;  // 3 vec4s per triangle
                vec3 v0 = triangles[triIdx + 0].xyz;
                vec3 v1 = triangles[triIdx + 1].xyz;
                vec3 v2 = triangles[triIdx + 2].xyz;

                float t;
                vec3 n;
                if (intersectTriangle(origin, dir, v0, v1, v2, t, n) && t < closestT) {
                    closestT = t;
                    hit.hitPoint = vec4(origin + dir * t, t);
                    hit.normal = vec4(n, 0.0);
                    hit.triangleId = uint(firstTri + i);
                }
            }
        } else {
            // Internal node - push children
            int rightChild = int(node.boundsMax.w);
            if (stackPtr < 62) {
                stack[stackPtr++] = rightChild;
                stack[stackPtr++] = leftInfo;  // Left child (process first)
            }
        }
    }

    // Calculate reflection and intensity if we hit something
    if (hit.hitPoint.w > 0.0) {
        vec3 incident = normalize(dir);
        vec3 n = normalize(hit.normal.xyz);

        // Check if surface is facing the radar (front-facing)
        // dot(n, -incident) > 0 means normal points toward radar
        float facing = dot(n, -incident);

        if (facing > 0.0) {
            // Front-facing surface - calculate reflection
            vec3 reflectDir = reflect(incident, n);

            // BRDF-based intensity calculation
            float k_d = 0.3;  // Diffuse coefficient
            float k_s = 0.7;  // Specular coefficient
            float shininess = 32.0;

            float cosTheta = facing;  // Already computed above
            float diffuse = k_d * cosTheta;
            float specular = k_s * pow(cosTheta, shininess);
            float intensity = clamp(diffuse + specular, 0.0, 1.0);

            hit.reflection = vec4(reflectDir, intensity);
        } else {
            // Back-facing surface - no reflection (intensity = 0)
            hit.reflection = vec4(0.0, 0.0, 0.0, 0.0);
        }

        // Increment hit counter
        atomicAdd(hitCounter, 1u);
    }

    // Write result
    hits[rayId] = hit;
}
)";

// Compute shader source: Shadow Map Generation
static const char* shadowMapShaderSource = R"(
#version 430 core
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct HitResult {
    vec4 hitPoint;   // xyz = position, w = distance (-1 = miss)
    vec4 normal;     // xyz = normal, w = material
    vec4 reflection; // xyz = reflection direction, w = intensity
    uint triangleId;
    uint rayId;
    uint targetId;
    float rcsContribution;
};

layout(std430, binding = 3) readonly buffer HitBuffer { HitResult hits[]; };
layout(r32f, binding = 0) uniform image2D shadowMap;

uniform int numRays;
uniform int raysPerRing;
uniform int numRings;

void main() {
    uint rayId = gl_GlobalInvocationID.x;
    if (rayId >= numRays) return;

    HitResult hit = hits[rayId];

    // Calculate texel coordinates directly from ray index
    // Shadow map is raysPerRing x numRings, so ray (ring, posInRing) -> texel (posInRing, ring)
    uint ring = rayId / uint(raysPerRing);
    uint posInRing = rayId % uint(raysPerRing);

    // Direct 1:1 mapping to texels
    ivec2 texCoord = ivec2(int(posInRing), int(ring));

    // Bounds check
    ivec2 texSize = imageSize(shadowMap);
    if (texCoord.x >= texSize.x || texCoord.y >= texSize.y) return;

    // Store hit distance (positive = hit at that distance, negative = no hit)
    // Fragment shader will compare its distance to determine if it's in shadow
    float hitDistance = hit.hitPoint.w;  // -1 for miss, positive for hit distance
    imageStore(shadowMap, texCoord, vec4(hitDistance, 0.0, 0.0, 1.0));
}
)";


RCSCompute::RCSCompute(QObject* parent)
    : QObject(parent)
{
}

RCSCompute::~RCSCompute() {
    cleanup();
}

bool RCSCompute::initialize() {
    if (initialized_) {
        return true;
    }

    if (!QOpenGLContext::currentContext()) {
        qWarning() << "RCSCompute::initialize - No OpenGL context available";
        return false;
    }

    if (!initializeOpenGLFunctions()) {
        qCritical() << "RCSCompute: Failed to initialize OpenGL functions!";
        return false;
    }

    // Clear pending GL errors
    GLUtils::clearGLErrors();

    // Check for compute shader support
    GLint maxComputeWorkGroupCount[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxComputeWorkGroupCount[0]);
    if (maxComputeWorkGroupCount[0] == 0) {
        qWarning() << "RCSCompute: Compute shaders not supported";
        return false;
    }

    if (!compileShaders()) {
        return false;
    }

    createBuffers();

    // Check for errors after initialization
    GLUtils::checkGLError("RCSCompute::initialize");

    initialized_ = true;
    return true;
}

void RCSCompute::cleanup() {
    if (!initialized_) return;

    if (rayBuffer_) { glDeleteBuffers(1, &rayBuffer_); rayBuffer_ = 0; }
    if (bvhBuffer_) { glDeleteBuffers(1, &bvhBuffer_); bvhBuffer_ = 0; }
    if (triangleBuffer_) { glDeleteBuffers(1, &triangleBuffer_); triangleBuffer_ = 0; }
    if (hitBuffer_) { glDeleteBuffers(1, &hitBuffer_); hitBuffer_ = 0; }
    if (counterBuffer_) { glDeleteBuffers(1, &counterBuffer_); counterBuffer_ = 0; }

    if (shadowMapTexture_) { glDeleteTextures(1, &shadowMapTexture_); shadowMapTexture_ = 0; }

    rayGenShader_.reset();
    traceShader_.reset();
    shadowMapShader_.reset();

    initialized_ = false;
}

bool RCSCompute::compileShaders() {
    // Ray generation shader
    rayGenShader_ = std::make_unique<QOpenGLShaderProgram>();
    if (!rayGenShader_->addShaderFromSourceCode(QOpenGLShader::Compute, rayGenShaderSource)) {
        qWarning() << "Failed to compile ray generation shader:" << rayGenShader_->log();
        return false;
    }
    if (!rayGenShader_->link()) {
        qWarning() << "Failed to link ray generation shader:" << rayGenShader_->log();
        return false;
    }

    // Trace shader
    traceShader_ = std::make_unique<QOpenGLShaderProgram>();
    if (!traceShader_->addShaderFromSourceCode(QOpenGLShader::Compute, traceShaderSource)) {
        qWarning() << "Failed to compile trace shader:" << traceShader_->log();
        return false;
    }
    if (!traceShader_->link()) {
        qWarning() << "Failed to link trace shader:" << traceShader_->log();
        return false;
    }

    // Shadow map generation shader
    shadowMapShader_ = std::make_unique<QOpenGLShaderProgram>();
    if (!shadowMapShader_->addShaderFromSourceCode(QOpenGLShader::Compute, shadowMapShaderSource)) {
        qWarning() << "Failed to compile shadow map shader:" << shadowMapShader_->log();
        return false;
    }
    if (!shadowMapShader_->link()) {
        qWarning() << "Failed to link shadow map shader:" << shadowMapShader_->log();
        return false;
    }

    return true;
}

void RCSCompute::createBuffers() {
    // Ray buffer
    glGenBuffers(1, &rayBuffer_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rayBuffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numRays_ * sizeof(Ray), nullptr, GL_DYNAMIC_DRAW);

    // BVH buffer (will be resized when geometry is set)
    glGenBuffers(1, &bvhBuffer_);

    // Triangle buffer (will be resized when geometry is set)
    glGenBuffers(1, &triangleBuffer_);

    // Hit buffer
    glGenBuffers(1, &hitBuffer_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, hitBuffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numRays_ * sizeof(HitResult), nullptr, GL_DYNAMIC_DRAW);

    // Atomic counter buffer
    glGenBuffers(1, &counterBuffer_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterBuffer_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Shadow map texture for beam visualization
    // Match resolution exactly to ray distribution for 1:1 texel mapping:
    // - X = raysPerRing (azimuth)
    // - Y = numRings (elevation)
    int raysPerRing = kRaysPerRing;
    int numRings = (numRays_ + raysPerRing - 1) / raysPerRing;
    shadowMapResolution_ = raysPerRing;  // Store for getShadowMapResolution()

    // Initialize shadow map with -1 (no hit = all visible) to avoid undefined content
    std::vector<float> initialData(raysPerRing * numRings, -1.0f);

    glGenTextures(1, &shadowMapTexture_);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, raysPerRing, numRings,
                 0, GL_RED, GL_FLOAT, initialData.data());
    // Use NEAREST filtering for exact texel lookup
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);      // Azimuth wraps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // Elevation clamps
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RCSCompute::setTargetGeometry(const std::vector<float>& vertices,
                                    const std::vector<uint32_t>& indices,
                                    const QMatrix4x4& modelMatrix) {
    bvhBuilder_.build(vertices, indices, modelMatrix);
    bvhDirty_ = true;
}

void RCSCompute::setRadarPosition(const QVector3D& position) {
    radarPosition_ = position;
}

void RCSCompute::setBeamDirection(const QVector3D& direction) {
    beamDirection_ = direction.normalized();
}

void RCSCompute::setBeamWidth(float widthDegrees) {
    beamWidthDegrees_ = widthDegrees;
}

void RCSCompute::setSphereRadius(float radius) {
    sphereRadius_ = radius;
}

void RCSCompute::setNumRays(int numRays) {
    if (numRays_ != numRays) {
        numRays_ = numRays;

        // Resize buffers
        if (rayBuffer_) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, rayBuffer_);
            glBufferData(GL_SHADER_STORAGE_BUFFER, numRays_ * sizeof(Ray), nullptr, GL_DYNAMIC_DRAW);
        }
        if (hitBuffer_) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, hitBuffer_);
            glBufferData(GL_SHADER_STORAGE_BUFFER, numRays_ * sizeof(HitResult), nullptr, GL_DYNAMIC_DRAW);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void RCSCompute::uploadBVH() {
    if (!bvhDirty_) return;

    const auto& nodes = bvhBuilder_.getNodes();
    const auto& triangles = bvhBuilder_.getTriangles();

    if (!nodes.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhBuffer_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, nodes.size() * sizeof(BVHNode), nodes.data(), GL_DYNAMIC_DRAW);
    }

    if (!triangles.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleBuffer_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_DRAW);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Memory barrier to ensure buffer updates are visible to compute shaders
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    bvhDirty_ = false;
}

void RCSCompute::dispatchRayGeneration() {
    rayGenShader_->bind();

    // Set uniforms
    rayGenShader_->setUniformValue("radarPosition", radarPosition_);
    rayGenShader_->setUniformValue("beamDirection", beamDirection_);
    rayGenShader_->setUniformValue("beamWidthRad", beamWidthDegrees_ * kDegToRadF);
    rayGenShader_->setUniformValue("maxDistance", sphereRadius_ * kMaxRayDistanceMultiplier);
    rayGenShader_->setUniformValue("numRays", numRays_);

    // Calculate ring parameters
    int raysPerRing = kRaysPerRing;
    int numRings = (numRays_ + raysPerRing - 1) / raysPerRing;
    rayGenShader_->setUniformValue("raysPerRing", raysPerRing);
    rayGenShader_->setUniformValue("numRings", numRings);

    // Bind ray buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rayBuffer_);

    // Dispatch
    int numGroups = (numRays_ + kComputeWorkgroupSize - 1) / kComputeWorkgroupSize;
    glDispatchCompute(numGroups, 1, 1);

    // Memory barrier
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    rayGenShader_->release();
}

void RCSCompute::dispatchTracing() {
    traceShader_->bind();

    // Set uniforms
    traceShader_->setUniformValue("numRays", numRays_);
    traceShader_->setUniformValue("numNodes", bvhBuilder_.getNodeCount());

    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rayBuffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bvhBuffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triangleBuffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, hitBuffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, counterBuffer_);

    // Reset counter
    GLuint zero = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterBuffer_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

    // Clear hit buffer to ensure no stale data (set all hitPoint.w to -1)
    // Use pre-allocated buffer to avoid per-frame allocation
    if (static_cast<int>(hitClearBuffer_.size()) != numRays_) {
        hitClearBuffer_.resize(numRays_);
        for (auto& hit : hitClearBuffer_) {
            hit.hitPoint = QVector4D(0, 0, 0, -1.0f);  // w=-1 means no hit
            hit.reflection = QVector4D(0, 0, 0, 0);
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, hitBuffer_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numRays_ * sizeof(HitResult), hitClearBuffer_.data());
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Dispatch
    int numGroups = (numRays_ + kComputeWorkgroupSize - 1) / kComputeWorkgroupSize;
    glDispatchCompute(numGroups, 1, 1);

    // Memory barrier
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    traceShader_->release();
}

void RCSCompute::readResults() {
    // Read hit counter
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterBuffer_);
    GLuint* counter = static_cast<GLuint*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT));
    if (counter) {
        hitCount_ = static_cast<int>(*counter);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void RCSCompute::readHitBuffer() {
    if (!hitBuffer_ || numRays_ <= 0) {
        hitResults_.clear();
        return;
    }

    hitResults_.resize(numRays_);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, hitBuffer_);
    void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                  numRays_ * sizeof(HitResult), GL_MAP_READ_BIT);
    if (ptr) {
        std::memcpy(hitResults_.data(), ptr, numRays_ * sizeof(HitResult));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void RCSCompute::clearShadowMap() {
    if (!shadowMapTexture_) return;

    // Get texture dimensions
    int raysPerRing = kRaysPerRing;
    int numRings = (numRays_ + raysPerRing - 1) / raysPerRing;
    int texWidth = raysPerRing;
    int texHeight = numRings;

    // Clear shadow map to -1 (no hit = all visible)
    // This is more reliable than glClearTexImage which may not be available
    std::vector<float> clearData(texWidth * texHeight, -1.0f);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RED, GL_FLOAT, clearData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RCSCompute::dispatchShadowMapGeneration() {
    if (!shadowMapShader_ || !shadowMapTexture_) return;

    shadowMapShader_->bind();

    // Set uniforms
    shadowMapShader_->setUniformValue("numRays", numRays_);

    // Calculate ring parameters (same as ray generation)
    int raysPerRing = kRaysPerRing;
    int numRings = (numRays_ + raysPerRing - 1) / raysPerRing;
    shadowMapShader_->setUniformValue("raysPerRing", raysPerRing);
    shadowMapShader_->setUniformValue("numRings", numRings);

    // Bind hit buffer (read) and shadow map (write)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, hitBuffer_);
    glBindImageTexture(0, shadowMapTexture_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    // Dispatch
    int numGroups = (numRays_ + kComputeWorkgroupSize - 1) / kComputeWorkgroupSize;
    glDispatchCompute(numGroups, 1, 1);

    // Memory barrier for texture writes - include TEXTURE_FETCH for sampling in fragment shader
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // Unbind image to allow texture sampling
    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    shadowMapShader_->release();
}

float RCSCompute::getBeamWidthRadians() const {
    return beamWidthDegrees_ * kDegToRadF;
}

void RCSCompute::compute() {
    if (!initialized_) {
        qWarning() << "RCSCompute::compute - Not initialized";
        return;
    }

    // Upload BVH if needed
    uploadBVH();

    // Clear shadow map before tracing
    clearShadowMap();

    // Generate rays
    dispatchRayGeneration();

    // Trace rays
    dispatchTracing();

    // Generate shadow map from hit results
    dispatchShadowMapGeneration();

    // Read results
    readResults();

    // Mark shadow map as ready for beam rendering
    shadowMapReady_ = true;
}

float RCSCompute::getOcclusionRatio() const {
    if (numRays_ == 0) return 0.0f;
    return static_cast<float>(hitCount_) / static_cast<float>(numRays_);
}

} // namespace RCS
