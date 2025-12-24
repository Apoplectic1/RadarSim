// CameraController.cpp
#include "CameraController.h"
#include <QDebug>
#include <cmath>

CameraController::CameraController(QObject* parent)
    : QObject(parent),
    distance_(300.0f),
    azimuth_(0.0f),
    elevation_(0.4f),  // ~23 degrees - slight overhead view
    focusPoint_(0.0f, 0.0f, 0.0f),
    inertiaTimer_(new QTimer(this)),
    inertiaEnabled_(false)
{
    // Initialize view matrix
    updateViewMatrix();

    // Connect inertia timer
    connect(inertiaTimer_, &QTimer::timeout, this, &CameraController::onInertiaTimerTimeout);

    // Start frame timer
    frameTimer_.start();
}

CameraController::~CameraController() {
    if (inertiaTimer_ && inertiaTimer_->isActive()) {
        inertiaTimer_->stop();
    }
}

QMatrix4x4 CameraController::getViewMatrix() const {
    return viewMatrix_;
}

void CameraController::resetView() {
    // Stop inertia
    stopInertia();

    // Reset orbit camera to default position
    distance_ = 300.0f;
    azimuth_ = 0.0f;
    elevation_ = 0.4f;  // ~23 degrees
    focusPoint_ = QVector3D(0.0f, 0.0f, 0.0f);

    // Update view matrix
    updateViewMatrix();

    emit viewChanged();
}

void CameraController::pan(const QPoint& delta) {
    // Scale factor for panning based on distance (further = faster pan)
    float panScale = distance_ * 0.002f;

    // Calculate camera right and up vectors for panning in screen space
    // Camera looks from cameraPosition_ toward focusPoint_
    QVector3D forward = (focusPoint_ - cameraPosition_).normalized();
    QVector3D worldUp(0.0f, 0.0f, 1.0f);  // Z-up
    QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    // Move focus point in screen space
    focusPoint_ -= right * delta.x() * panScale;
    focusPoint_ += up * delta.y() * panScale;

    // Update view matrix
    updateViewMatrix();

    emit viewChanged();
}

void CameraController::setInertiaEnabled(bool enabled) {
    if (inertiaEnabled_ != enabled) {
        inertiaEnabled_ = enabled;

        // If disabling, stop any ongoing inertia
        if (!enabled) {
            stopInertia();
        }

        emit inertiaEnabledChanged(enabled);
    }
}

void CameraController::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Left button for scene rotation (orbit camera)
        // Stop any ongoing inertia
        stopInertia();

        isDragging_ = true;
        lastMousePos_ = event->pos();

        // Reset frame timer for velocity calculation
        frameTimer_.restart();
    }
    else if (event->button() == Qt::MiddleButton) {
        // Middle button for panning (drag scene left/right/up/down)
        isPanning_ = true;
        panStartPos_ = event->pos();
    }
    else if (event->button() == Qt::RightButton) {
        // Right button reserved for context menu (handled by widget)
    }
}

void CameraController::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging_) {
        // Calculate elapsed time for velocity calculation
        float dt = frameTimer_.elapsed() / 1000.0f; // Convert to seconds
        frameTimer_.restart();

        // Avoid division by zero
        if (dt < 0.001f) dt = 0.016f; // Default to ~60 FPS

        // Calculate mouse movement delta
        QPoint delta = event->pos() - lastMousePos_;

        // Exit if no movement
        if (delta.isNull()) {
            return;
        }

        // Convert mouse delta to orbit angle changes (radians per pixel)
        float rotationSpeed = 0.005f;
        float dAzimuth = -delta.x() * rotationSpeed;    // Horizontal drag = orbit left/right
        float dElevation = delta.y() * rotationSpeed;   // Vertical drag = orbit up/down

        // Update orbit angles
        azimuth_ += dAzimuth;
        elevation_ += dElevation;

        // Clamp elevation to avoid gimbal lock at poles
        const float maxElevation = 1.5f;  // ~85 degrees
        elevation_ = qBound(-maxElevation, elevation_, maxElevation);

        // Store velocities for inertia calculation
        azimuthVelocity_ = dAzimuth / dt * 0.3f;    // Scale down for smoother inertia
        elevationVelocity_ = dElevation / dt * 0.3f;

        // Store current mouse position for next frame
        lastMousePos_ = event->pos();

        // Update view matrix
        updateViewMatrix();

        emit viewChanged();
    }
    else if (isPanning_) {
        // Calculate pan delta
        QPoint delta = event->pos() - panStartPos_;

        // Apply panning
        pan(delta);

        // Update starting position
        panStartPos_ = event->pos();
    }
}

void CameraController::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (isDragging_) {
            isDragging_ = false;

            // Only start inertia if it's enabled and velocity is significant
            float totalVelocity = std::sqrt(azimuthVelocity_ * azimuthVelocity_ +
                                             elevationVelocity_ * elevationVelocity_);
            if (inertiaEnabled_ && totalVelocity > 0.001f) {
                startInertia(azimuthVelocity_, elevationVelocity_);
            }
        }
    }
    else if (event->button() == Qt::MiddleButton) {
        isPanning_ = false;
    }
}

void CameraController::wheelEvent(QWheelEvent* event) {
    // Zoom changes camera distance (scroll up = zoom in = closer)
    float zoomSpeed = 0.5f;
    float distanceChange = -event->angleDelta().y() * zoomSpeed;

    // Update distance with limits
    float minDistance = 50.0f;
    float maxDistance = 1000.0f;

    distance_ += distanceChange;
    distance_ = qBound(minDistance, distance_, maxDistance);

    // Update view matrix
    updateViewMatrix();

    emit viewChanged();
}

void CameraController::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        resetView();
    }
}

void CameraController::onInertiaTimerTimeout() {
    // Check if velocities are still significant
    float totalVelocity = std::sqrt(azimuthVelocity_ * azimuthVelocity_ +
                                     elevationVelocity_ * elevationVelocity_);
    if (totalVelocity > 0.0001f) {
        // Apply orbit velocity
        azimuth_ += azimuthVelocity_;
        elevation_ += elevationVelocity_;

        // Clamp elevation to avoid gimbal lock
        const float maxElevation = 1.5f;
        elevation_ = qBound(-maxElevation, elevation_, maxElevation);

        // Decay the velocities
        azimuthVelocity_ *= velocityDecay_;
        elevationVelocity_ *= velocityDecay_;

        // Update view matrix
        updateViewMatrix();

        emit viewChanged();
    }
    else {
        // Stop the timer when velocity is negligible
        stopInertia();
    }
}

void CameraController::startInertia(float azimuthVel, float elevationVel) {
    azimuthVelocity_ = azimuthVel;
    elevationVelocity_ = elevationVel;

    // Start timer if not already running
    if (!inertiaTimer_->isActive()) {
        inertiaTimer_->start(16); // ~60 FPS
    }
}

void CameraController::stopInertia() {
    inertiaTimer_->stop();
    azimuthVelocity_ = 0.0f;
    elevationVelocity_ = 0.0f;
}

void CameraController::updateViewMatrix() {
    // Compute camera position from spherical coordinates
    // Using Z-up convention (mathematical standard)
    float x = distance_ * std::cos(elevation_) * std::cos(azimuth_);
    float y = distance_ * std::cos(elevation_) * std::sin(azimuth_);
    float z = distance_ * std::sin(elevation_);
    cameraPosition_ = QVector3D(x, y, z) + focusPoint_;

    // Build view matrix using lookAt
    // Camera looks at focus point with Z-up
    viewMatrix_.setToIdentity();
    viewMatrix_.lookAt(cameraPosition_, focusPoint_, QVector3D(0.0f, 0.0f, 1.0f));

    // Model matrix is always identity - all scene rotation is in view matrix
    modelMatrix_.setToIdentity();
}
