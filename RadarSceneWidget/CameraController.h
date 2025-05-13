// CameraController.h
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

    // View/model transformation
    QMatrix4x4 getViewMatrix() const;
    QMatrix4x4 getModelMatrix() const;

    // Camera control
    void resetView();
    void pan(const QPoint& delta);

    // Inertia control
    void setInertiaEnabled(bool enabled);
    bool isInertiaEnabled() const { return inertiaEnabled_; }

    // Mouse interaction
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

signals:
    void viewChanged();
    void inertiaEnabledChanged(bool enabled);

private slots:
    void onInertiaTimerTimeout();

private:
    // View parameters
    QQuaternion rotation_ = QQuaternion();
    QVector3D translation_ = QVector3D(0, 0, 0);
    float zoomFactor_ = 1.0f;

    // View matrices
    QMatrix4x4 viewMatrix_;
    QMatrix4x4 modelMatrix_;

    // Mouse tracking
    QPoint lastMousePos_;
    bool isDragging_ = false;
    bool isPanning_ = false;
    QPoint panStartPos_;

    // Inertia
    QTimer* inertiaTimer_ = nullptr;
    QElapsedTimer frameTimer_;
    QVector3D rotationAxis_;
    float rotationVelocity_ = 0.0f;
    float rotationDecay_ = 0.95f;
    bool inertiaEnabled_ = false;

    // Helper methods
    void startInertia(QVector3D axis, float velocity);
    void stopInertia();
    void updateMatrices();
};