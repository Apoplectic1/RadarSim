// --- CameraController.cpp ----

#include "CameraController.h"

CameraController::CameraController(QObject* parent)
    : QObject(parent),
    inertiaEnabled_(false)
{
}

CameraController::~CameraController() {
}

void CameraController::resetView() {
    // Will be implemented later
}

void CameraController::setInertiaEnabled(bool enabled) {
    inertiaEnabled_ = enabled;
}

QMatrix4x4 CameraController::getViewMatrix() const {
    // Placeholder
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.translate(0, 0, -300.0f);
    return matrix;
}

QMatrix4x4 CameraController::getModelMatrix() const {
    // Placeholder
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    return matrix;
}

void CameraController::mousePressEvent(QMouseEvent* event) {
    // Will be implemented later
}

void CameraController::mouseMoveEvent(QMouseEvent* event) {
    // Will be implemented later
}

void CameraController::mouseReleaseEvent(QMouseEvent* event) {
    // Will be implemented later
}

void CameraController::wheelEvent(QWheelEvent* event) {
    // Will be implemented later
}

void CameraController::mouseDoubleClickEvent(QMouseEvent* event) {
    // Will be implemented later
}