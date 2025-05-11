// ---- CameraController.h ----

#pragma once

#include <QObject>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QElapsedTimer>

class CameraController : public QObject {
    Q_OBJECT

public:
    explicit CameraController(QObject* parent = nullptr);
    ~CameraController();

    // Camera manipulation
    void resetView();
    void setInertiaEnabled(bool enabled);
    bool isInertiaEnabled() const { return inertiaEnabled_; }

    // View matrix access
    QMatrix4x4 getViewMatrix() const;
    QMatrix4x4 getModelMatrix() const;

    // Mouse event handlers
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

private:
    // Placeholder for future implementation
    // Will contain camera control code from SphereWidget
    bool inertiaEnabled_ = false;
};