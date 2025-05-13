//// CameraController.cpp
#include "CameraController.h"
#include <QDebug>

CameraController::CameraController(QObject* parent)
    : QObject(parent),
    inertiaTimer_(new QTimer(this)),
    inertiaEnabled_(false)
{
    // Initialize matrices
    updateMatrices();

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

QMatrix4x4 CameraController::getModelMatrix() const {
    return modelMatrix_;
}

void CameraController::resetView() {
    // Stop inertia
    stopInertia();

    // Reset transform parameters
    rotation_ = QQuaternion();
    translation_ = QVector3D(0, 0, 0);
    zoomFactor_ = 1.0f;

    // Update matrices
    updateMatrices();

    emit viewChanged();
}

void CameraController::pan(const QPoint& delta) {
    // Scale factor for panning (adjust as needed)
    float panScale = 0.5f / zoomFactor_;

    // Update translation (invert Y for natural panning)
    translation_.setX(translation_.x() + delta.x() * panScale);
    translation_.setY(translation_.y() - delta.y() * panScale);

    // Update matrices
    updateMatrices();

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
        // Stop any ongoing inertia
        stopInertia();

        isDragging_ = true;
        lastMousePos_ = event->pos();

        // Reset frame timer for velocity calculation
        frameTimer_.restart();
    }
    else if (event->button() == Qt::RightButton) {
        // Right button for panning
        isPanning_ = true;
        panStartPos_ = event->pos();
    }
    else if (event->button() == Qt::MiddleButton) {
        // Middle button to reset view
        resetView();
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

        // Convert to rotation (scale down the rotation speed)
        float rotationSpeed = 0.3f;
        float angleX = delta.x() * rotationSpeed;
        float angleY = delta.y() * rotationSpeed;

        // Create combined rotation axis and angle
        QVector3D axis(angleY, angleX, 0.0f);
        float angle = axis.length();
        axis.normalize();

        // Store axis and velocity for inertia calculation
        rotationAxis_ = axis;
        rotationVelocity_ = angle / dt * 0.1f; // Scale down for smoother inertia

        // Create quaternion from axis and angle
        QQuaternion newRotation = QQuaternion::fromAxisAndAngle(axis, angle);

        // Update rotation
        rotation_ = newRotation * rotation_;

        // Store current mouse position for next frame
        lastMousePos_ = event->pos();

        // Update matrices
        updateMatrices();

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
    if (event->button() == Qt::LeftButton && isDragging_) {
        isDragging_ = false;

        // Only start inertia if it's enabled and velocity is significant
        if (inertiaEnabled_ && rotationVelocity_ > 0.2f) {
            startInertia(rotationAxis_, rotationVelocity_);
        }
    }
    else if (event->button() == Qt::RightButton) {
        isPanning_ = false;
    }
}

void CameraController::wheelEvent(QWheelEvent* event) {
    // Calculate zoom change based on wheel delta
    float zoomSpeed = 0.001f;
    float zoomChange = event->angleDelta().y() * zoomSpeed;

    // Update zoom factor with limits
    float minZoom = 0.5f;
    float maxZoom = 2.0f;

    zoomFactor_ += zoomChange;
    zoomFactor_ = qBound(minZoom, zoomFactor_, maxZoom);

    // Update matrices
    updateMatrices();

    emit viewChanged();
}

void CameraController::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        resetView();
    }
}

void CameraController::onInertiaTimerTimeout() {
    // Apply rotation based on velocity and time
    if (rotationVelocity_ > 0.05f) {
        // Apply rotation based on current velocity
        QQuaternion inertiaRotation = QQuaternion::fromAxisAndAngle(
            rotationAxis_, rotationVelocity_);

        // Update the rotation
        rotation_ = inertiaRotation * rotation_;

        // Decay the velocity
        rotationVelocity_ *= rotationDecay_;

        // Update matrices
        updateMatrices();

        emit viewChanged();
    }
    else {
        // Stop the timer when velocity is negligible
        stopInertia();
    }
}

void CameraController::startInertia(QVector3D axis, float velocity) {
    rotationAxis_ = axis.normalized();
    rotationVelocity_ = velocity;

    // Start timer if not already running
    if (!inertiaTimer_->isActive()) {
        inertiaTimer_->start(16); // ~60 FPS
    }
}

void CameraController::stopInertia() {
    inertiaTimer_->stop();
    rotationVelocity_ = 0.0f;
}

void CameraController::updateMatrices() {
    // Update view matrix
    viewMatrix_.setToIdentity();
    viewMatrix_.translate(0, 0, -300.0f); // Fixed distance camera

    // Update model matrix
    modelMatrix_.setToIdentity();
    modelMatrix_.scale(zoomFactor_);
    modelMatrix_.translate(translation_);
    modelMatrix_.rotate(rotation_);
}