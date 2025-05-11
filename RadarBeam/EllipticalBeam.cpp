// ---- EllipticalBeam.cpp ----

#include "EllipticalBeam.h"

EllipticalBeam::EllipticalBeam(float sphereRadius, float horizontalWidthDegrees, float verticalWidthDegrees)
    : RadarBeam(sphereRadius, (horizontalWidthDegrees + verticalWidthDegrees) / 2.0f),
    horizontalWidthDegrees_(horizontalWidthDegrees),
    verticalWidthDegrees_(verticalWidthDegrees)
{
    // Make sure initialize is called to set up OpenGL resources
    // Note: This will be called again by SphereWidget but that's ok
    initialize();
}

EllipticalBeam::~EllipticalBeam() {
	// Base class destructor will handle cleanup
}

void EllipticalBeam::setHorizontalWidth(float degrees) {
	horizontalWidthDegrees_ = degrees;
	createBeamGeometry();
}

void EllipticalBeam::setVerticalWidth(float degrees) {
	verticalWidthDegrees_ = degrees;
	createBeamGeometry();
}

void EllipticalBeam::createBeamGeometry() {
    // Clear previous data
    vertices_.clear();
    indices_.clear();

    // Number of segments around the base ellipse
    const int segments = 32;

    // Calculate beam direction
    QVector3D direction = calculateBeamDirection(currentRadarPosition_);
    QVector3D normDirection = direction.normalized();

    // Calculate beam end point
    QVector3D endPoint = calculateOppositePoint(currentRadarPosition_, direction);

    // Calculate beam length
    float beamLength = (endPoint - currentRadarPosition_).length();

    // Calculate horizontal and vertical radii for the elliptical base
    float horizontalRadius = tan(horizontalWidthDegrees_ * M_PI / 180.0f / 2.0f) * beamLength;
    float verticalRadius = tan(verticalWidthDegrees_ * M_PI / 180.0f / 2.0f) * beamLength;

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
    // Use the beam direction as the normal vector for the apex
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
        // This creates more consistent lighting during rotation
        float blendFactor = 0.3f; // Adjust this value between 0.0 and 1.0 to control the blend
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

    // Set up VAO and VBO for the beam
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
}
