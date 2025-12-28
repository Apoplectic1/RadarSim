// ---- PhasedArrayBeam.cpp ----

#include "PhasedArrayBeam.h"
#include "Constants.h"
#include <cmath>
#include <QQuaternion>

using namespace RadarSim::Constants;

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

    // Don't call initialize() here - it requires a valid OpenGL context
    // BeamController::createBeam() will call initialize() after construction
}

PhasedArrayBeam::~PhasedArrayBeam() {
    if (sideLobeVAO_.isCreated()) {
        sideLobeVAO_.destroy();
    }
    if (sideLobeVBO_.isCreated()) {
        sideLobeVBO_.destroy();
    }
    if (sideLobeEBO_.isCreated()) {
        sideLobeEBO_.destroy();
    }
}

void PhasedArrayBeam::setMainLobeDirection(float azimuthOffset, float elevationOffset) {
    azimuthOffset_ = azimuthOffset;
    elevationOffset_ = elevationOffset;
    createBeamGeometry();
    uploadGeometryToGPU();
}

void PhasedArrayBeam::setElementCount(int horizontalElements, int verticalElements) {
    horizontalElements_ = horizontalElements;
    verticalElements_ = verticalElements;
    createBeamGeometry();
    uploadGeometryToGPU();
}

void PhasedArrayBeam::setElementSpacing(float horizontalSpacing, float verticalSpacing) {
    horizontalSpacing_ = horizontalSpacing;
    verticalSpacing_ = verticalSpacing;
    createBeamGeometry();
    uploadGeometryToGPU();
}

void PhasedArrayBeam::setSideLobeVisibility(bool visible) {
    showSideLobes_ = visible;
    createBeamGeometry();
    uploadGeometryToGPU();
}

void PhasedArrayBeam::setSideLobeIntensity(float intensity) {
    sideLobeIntensity_ = intensity;
    createBeamGeometry();
    uploadGeometryToGPU();
}

void PhasedArrayBeam::setCustomPattern(std::function<float(float, float)> patternFunc) {
    patternFunction_ = patternFunc;
    createBeamGeometry();
    uploadGeometryToGPU();
}

void PhasedArrayBeam::createBeamGeometry() {
    // Clear previous data
    vertices_.clear();
    indices_.clear();

    // Skip geometry creation if position hasn't been set yet
    if (currentRadarPosition_.isNull()) {
        return;
    }

    // Number of segments around the base ellipse
    const int segments = kBeamConeSegments;
    const int capRings = kBeamCapRings;  // Number of rings for the spherical cap

    // Calculate beam direction
    QVector3D direction = calculateBeamDirection(currentRadarPosition_);
    QVector3D normDirection = direction.normalized();

    // Apply phased array offsets (convert from degrees to radians)
    if (azimuthOffset_ != 0.0f || elevationOffset_ != 0.0f) {
        // Create rotation quaternions for azimuth and elevation offsets
        float azRadians = azimuthOffset_ * kDegToRadF;
        float elRadians = elevationOffset_ * kDegToRadF;

        // Get perpendicular vectors
        QVector3D up(0.0f, 1.0f, 0.0f);
        if (qAbs(QVector3D::dotProduct(normDirection, up)) > kGimbalLockThreshold) {
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
    float horizontalRadius = tan(beamWidthDegrees_ * kDegToRadF / 2.0f) * beamLength;
    float verticalRadius = tan(beamWidthDegrees_ * 0.5f * kDegToRadF / 2.0f) * beamLength; // Half the width

    // Find perpendicular vectors to create the base ellipse
    QVector3D up(0.0f, 1.0f, 0.0f);
    if (qAbs(QVector3D::dotProduct(normDirection, up)) > kGimbalLockThreshold) {
        // If direction is nearly parallel to up, use a different vector
        up = QVector3D(1.0f, 0.0f, 0.0f);
    }

    QVector3D right = QVector3D::crossProduct(normDirection, up).normalized();
    up = QVector3D::crossProduct(right, normDirection).normalized();

    // Base center (conceptual center of cone base before projection)
    QVector3D baseCenter = currentRadarPosition_ + normDirection * beamLength;

    // Cap center is where the beam axis exits the sphere on the opposite side
    // This should be the antipodal point from the radar position
    QVector3D capCenter = -currentRadarPosition_.normalized() * sphereRadius_;

    // Add apex vertex (with normal pointing to beam direction)
    vertices_.push_back(currentRadarPosition_.x());
    vertices_.push_back(currentRadarPosition_.y());
    vertices_.push_back(currentRadarPosition_.z());
    vertices_.push_back(normDirection.x());
    vertices_.push_back(normDirection.y());
    vertices_.push_back(normDirection.z());

    // Store the outer rim vertices on the sphere (where cone meets sphere)
    std::vector<QVector3D> outerRimPoints;
    for (int i = 0; i < segments; i++) {
        float angle = kTwoPiF * i / segments;
        float cA = cos(angle);
        float sA = sin(angle);

        // Elliptical calculation
        QVector3D circlePoint = baseCenter + right * (cA * horizontalRadius) + up * (sA * verticalRadius);

        // Project point onto sphere surface
        circlePoint = circlePoint.normalized() * sphereRadius_;
        outerRimPoints.push_back(circlePoint);

        // Calculate normal for cone surface
        QVector3D toCircle = circlePoint - baseCenter;
        QVector3D normal = (normDirection * kNormalBlendFactor + toCircle.normalized() * (1.0f - kNormalBlendFactor)).normalized();

        // Position
        vertices_.push_back(circlePoint.x());
        vertices_.push_back(circlePoint.y());
        vertices_.push_back(circlePoint.z());

        // Normal
        vertices_.push_back(normal.x());
        vertices_.push_back(normal.y());
        vertices_.push_back(normal.z());
    }

    // Create cone side triangles (apex to each pair of adjacent rim vertices)
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;
        indices_.push_back(0);  // Apex
        indices_.push_back(i + 1);  // Current rim vertex
        indices_.push_back(next + 1);  // Next rim vertex
    }

    // Add the spherical cap to fill in the "dish"
    int capStartVertex = segments + 1;

    // Add vertices for inner rings of the cap
    for (int ring = 1; ring <= capRings; ring++) {
        float t = (float)ring / (float)capRings;

        for (int i = 0; i < segments; i++) {
            QVector3D outerPoint = outerRimPoints[i];
            QVector3D interpolated = (outerPoint * (1.0f - t) + capCenter * t).normalized() * sphereRadius_;
            QVector3D normal = -interpolated.normalized();

            vertices_.push_back(interpolated.x());
            vertices_.push_back(interpolated.y());
            vertices_.push_back(interpolated.z());
            vertices_.push_back(normal.x());
            vertices_.push_back(normal.y());
            vertices_.push_back(normal.z());
        }
    }

    // Create triangles for the cap - first ring
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;
        int outerCurr = i + 1;
        int outerNext = next + 1;
        int innerCurr = capStartVertex + i;
        int innerNext = capStartVertex + next;

        indices_.push_back(outerCurr);
        indices_.push_back(innerCurr);
        indices_.push_back(outerNext);

        indices_.push_back(outerNext);
        indices_.push_back(innerCurr);
        indices_.push_back(innerNext);
    }

    // Remaining rings
    for (int ring = 1; ring < capRings; ring++) {
        int outerRingStart = capStartVertex + (ring - 1) * segments;
        int innerRingStart = capStartVertex + ring * segments;

        for (int i = 0; i < segments; i++) {
            int next = (i + 1) % segments;
            int outerCurr = outerRingStart + i;
            int outerNext = outerRingStart + next;
            int innerCurr = innerRingStart + i;
            int innerNext = innerRingStart + next;

            indices_.push_back(outerCurr);
            indices_.push_back(innerCurr);
            indices_.push_back(outerNext);

            indices_.push_back(outerNext);
            indices_.push_back(innerCurr);
            indices_.push_back(innerNext);
        }
    }

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
