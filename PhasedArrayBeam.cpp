// ---- PhasedArrayBeam.cpp ----

#include "PhasedArrayBeam.h"
#include "EllipticalBeam.h"
#include <cmath>

PhasedArrayBeam::PhasedArrayBeam(float sphereRadius, float mainLobeWidthDegrees)
    : RadarBeam(sphereRadius, mainLobeWidthDegrees),
    azimuthOffset_(0.0f),
    elevationOffset_(0.0f),
    horizontalElements_(16),
    verticalElements_(16),
    horizontalSpacing_(0.5f),
    verticalSpacing_(0.5f),
    showSideLobes_(true),
    sideLobeIntensity_(0.3f)
{
    // Default pattern function (uniform)
    patternFunction_ = [](float azimuth, float elevation) {
        return 1.0f;  // Uniform gain
        };

    // Initialize OpenGL resources
     // Note: This will be called again by SphereWidget but that's ok
    initialize();
}

PhasedArrayBeam::~PhasedArrayBeam() {
    if (sideLobeVAO.isCreated()) {
        sideLobeVAO.destroy();
    }
    if (sideLobeVBO.isCreated()) {
        sideLobeVBO.destroy();
    }
    if (sideLobeEBO.isCreated()) {
        sideLobeEBO.destroy();
    }
}

void PhasedArrayBeam::setMainLobeDirection(float azimuthOffset, float elevationOffset) {
    azimuthOffset_ = azimuthOffset;
    elevationOffset_ = elevationOffset;
    createBeamGeometry();
}

void PhasedArrayBeam::setElementCount(int horizontalElements, int verticalElements) {
    horizontalElements_ = horizontalElements;
    verticalElements_ = verticalElements;
    createBeamGeometry();
}

void PhasedArrayBeam::setElementSpacing(float horizontalSpacing, float verticalSpacing) {
    horizontalSpacing_ = horizontalSpacing;
    verticalSpacing_ = verticalSpacing;
    createBeamGeometry();
}

void PhasedArrayBeam::setSideLobeVisibility(bool visible) {
    showSideLobes_ = visible;
    createBeamGeometry();
}

void PhasedArrayBeam::setSideLobeIntensity(float intensity) {
    sideLobeIntensity_ = intensity;
    createBeamGeometry();
}

void PhasedArrayBeam::setCustomPattern(std::function<float(float, float)> patternFunc) {
    patternFunction_ = patternFunc;
    createBeamGeometry();
}

void PhasedArrayBeam::createBeamGeometry() {
    // Clear previous data
    vertices_.clear();
    indices_.clear();

    // Number of segments around the base ellipse
    const int segments = 32;

    // Calculate beam direction
    QVector3D direction = calculateBeamDirection(currentRadarPosition_);
    QVector3D normDirection = direction.normalized();

    // Apply phased array offsets (convert from degrees to radians)
    if (azimuthOffset_ != 0.0f || elevationOffset_ != 0.0f) {
        // Create rotation quaternions for azimuth and elevation offsets
        float azRadians = azimuthOffset_ * M_PI / 180.0f;
        float elRadians = elevationOffset_ * M_PI / 180.0f;

        // Get perpendicular vectors
        QVector3D up(0.0f, 1.0f, 0.0f);
        if (qAbs(QVector3D::dotProduct(normDirection, up)) > 0.99f) {
            up = QVector3D(1.0f, 0.0f, 0.0f);
        }
        QVector3D right = QVector3D::crossProduct(normDirection, up).normalized();
        up = QVector3D::crossProduct(right, normDirection).normalized();

        // Apply azimuth rotation (around up vector)
        QQuaternion azimuthRotation = QQuaternion::fromAxisAndAngle(up, azimuthOffset_);
        // Apply elevation rotation (around right vector)
        QQuaternion elevationRotation = QQuaternion::fromAxisAndAngle(right, elevationOffset_);

        // Combine rotations and apply to direction
        QQuaternion totalRotation = azimuthRotation * elevationRotation;
        normDirection = totalRotation.rotatedVector(normDirection);
    }

    // Calculate beam end point
    QVector3D endPoint = calculateOppositePoint(currentRadarPosition_, normDirection);

    // Calculate beam length
    float beamLength = (endPoint - currentRadarPosition_).length();

    // Calculate horizontal and vertical radii for the elliptical base
    // We're using beam width for horizontal and a narrower width for vertical
    float horizontalRadius = tan(beamWidthDegrees_ * M_PI / 180.0f / 2.0f) * beamLength;
    float verticalRadius = tan(beamWidthDegrees_ * 0.5f * M_PI / 180.0f / 2.0f) * beamLength; // Half the width

    // Find perpendicular vectors to create the base ellipse
    QVector3D up(0.0f, 1.0f, 0.0f);
    if (qAbs(QVector3D::dotProduct(normDirection, up)) > 0.99f) {
        // If direction is nearly parallel to up, use a different vector
        up = QVector3D(1.0f, 0.0f, 0.0f);
    }

    QVector3D right = QVector3D::crossProduct(normDirection, up).normalized();
    up = QVector3D::crossProduct(right, normDirection).normalized();

    // Base center
    QVector3D baseCenter = currentRadarPosition_ + normDirection * beamLength;

    // Add apex vertex (with normal pointing to beam direction)
    vertices_.push_back(currentRadarPosition_.x());
    vertices_.push_back(currentRadarPosition_.y());
    vertices_.push_back(currentRadarPosition_.z());

    // Normal at apex
    vertices_.push_back(normDirection.x());
    vertices_.push_back(normDirection.y());
    vertices_.push_back(normDirection.z());

    // Add base vertices
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float cA = cos(angle);
        float sA = sin(angle);

        // Elliptical calculation
        QVector3D circlePoint = baseCenter + right * (cA * horizontalRadius) + up * (sA * verticalRadius);

        // Calculate normal (improved calculation for better lighting)
        // Get the vector from base center to the current point on the ellipse
        QVector3D toCircle = circlePoint - baseCenter;

        // Create a blend of the beam direction and the outward direction
        // Higher blend factor towards the direction creates a more focused-looking beam
        float blendFactor = 0.25f; // Adjust this value between 0.0 and 1.0 to control the blend
        QVector3D normal = (normDirection * blendFactor + toCircle.normalized() * (1.0f - blendFactor)).normalized();

        // Position
        vertices_.push_back(circlePoint.x());
        vertices_.push_back(circlePoint.y());
        vertices_.push_back(circlePoint.z());

        // Normal
        vertices_.push_back(normal.x());
        vertices_.push_back(normal.y());
        vertices_.push_back(normal.z());
    }

    // Create triangles (apex to each pair of adjacent base vertices)
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;

        // Indices (apex is 0, base starts at 1)
        indices_.push_back(0);  // Apex
        indices_.push_back(i + 1);  // Current base vertex
        indices_.push_back(next + 1);  // Next base vertex
    }

    // Setup VAO and VBO part remains the same...
    if (!beamVAO.isCreated()) {
        beamVAO.create();
    }
    beamVAO.bind();

    if (!beamVBO.isCreated()) {
        beamVBO.create();
    }
    beamVBO.bind();
    beamVBO.allocate(vertices_.data(), vertices_.size() * sizeof(float));

    if (!beamEBO.isCreated()) {
        beamEBO.create();
    }
    beamEBO.bind();
    beamEBO.allocate(indices_.data(), indices_.size() * sizeof(unsigned int));

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    beamVAO.release();

    // Then create side lobes if enabled
    if (showSideLobes_) {
        createSideLobes();
    }
}


void PhasedArrayBeam::createSideLobes() {
    // This is a placeholder for side lobe creation
    // In a real implementation, this would calculate side lobe positions
    // based on array parameters and create additional beam lobes

    // For now, we'll just create a simple side lobe pattern
    // In a complete implementation, this would use beam pattern calculations
    // based on the phased array parameters
}

void PhasedArrayBeam::calculateBeamPattern() {
    // Calculate a realistic beam pattern based on array parameters
    // This would use array theory to generate directional gain pattern

    // Placeholder for future implementation
}
