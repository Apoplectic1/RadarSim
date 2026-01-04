// CameraController.cpp
#include "CameraController.h"
#include "Constants.h"
#include <cmath>

using namespace RS::Constants;

CameraController::CameraController(QObject* parent)
    : QObject(parent),
    distance_(Defaults::kCameraDistance),
    azimuth_(Defaults::kCameraAzimuth),
    elevation_(Defaults::kCameraElevation),
    focusPoint_(0.0f, 0.0f, 0.0f)
{
    // Initialize view matrix
    updateViewMatrix();
}

CameraController::~CameraController() = default;

QMatrix4x4 CameraController::getViewMatrix() const {
    return viewMatrix_;
}

void CameraController::resetView() {
    // Reset orbit camera to default position
    distance_ = Defaults::kCameraDistance;
    azimuth_ = Defaults::kCameraAzimuth;
    elevation_ = Defaults::kCameraElevation;
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

void CameraController::setDistance(float d) {
    distance_ = qBound(kCameraMinDistance, d, kCameraMaxDistance);
    updateViewMatrix();
    emit viewChanged();
}

void CameraController::setAzimuth(float a) {
    azimuth_ = a;
    updateViewMatrix();
    emit viewChanged();
}

void CameraController::setElevation(float e) {
    elevation_ = qBound(-kCameraMaxElevation, e, kCameraMaxElevation);
    updateViewMatrix();
    emit viewChanged();
}

void CameraController::setFocusPoint(const QVector3D& fp) {
    focusPoint_ = fp;
    updateViewMatrix();
    emit viewChanged();
}

void CameraController::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Left button for scene rotation (orbit camera)
        isDragging_ = true;
        lastMousePos_ = event->pos();
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
        // Calculate mouse movement delta
        QPoint delta = event->pos() - lastMousePos_;

        // Exit if no movement
        if (delta.isNull()) {
            return;
        }

        // Convert mouse delta to orbit angle changes (radians per pixel)
        float dAzimuth = -delta.x() * kCameraRotationSpeed;    // Horizontal drag = orbit left/right
        float dElevation = delta.y() * kCameraRotationSpeed;   // Vertical drag = orbit up/down

        // Update orbit angles
        azimuth_ += dAzimuth;
        elevation_ += dElevation;

        // Clamp elevation to avoid gimbal lock at poles (~85 degrees)
        elevation_ = qBound(-kCameraMaxElevation, elevation_, kCameraMaxElevation);

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
        isDragging_ = false;
    }
    else if (event->button() == Qt::MiddleButton) {
        isPanning_ = false;
    }
}

void CameraController::wheelEvent(QWheelEvent* event) {
    // Zoom changes camera distance (scroll up = zoom in = closer)
    float distanceChange = -event->angleDelta().y() * kCameraZoomSpeed;

    // Update distance with limits
    distance_ += distanceChange;
    distance_ = qBound(kCameraMinDistance, distance_, kCameraMaxDistance);

    // Update view matrix
    updateViewMatrix();

    emit viewChanged();
}

void CameraController::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        resetView();
    }
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
