// CameraController.h
#pragma once

#include <QObject>
#include <QMatrix4x4>
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
    QMatrix4x4 getModelMatrix() const { return modelMatrix_; }  // Always identity

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
    // Spherical orbit camera parameters
    float distance_ = 300.0f;           // Distance from focus point
    float azimuth_ = 0.0f;              // Horizontal angle (radians)
    float elevation_ = 0.4f;            // Vertical angle (radians), ~23 degrees
    QVector3D focusPoint_ = QVector3D(0.0f, 0.0f, 0.0f);
    QVector3D cameraPosition_;          // Computed from spherical coords

    // View matrices
    QMatrix4x4 viewMatrix_;
    QMatrix4x4 modelMatrix_;            // Always identity

    // Mouse tracking
    QPoint lastMousePos_;
    bool isDragging_ = false;
    bool isPanning_ = false;
    QPoint panStartPos_;

    // Inertia for spherical coordinates
    QTimer* inertiaTimer_ = nullptr;
    QElapsedTimer frameTimer_;
    float azimuthVelocity_ = 0.0f;
    float elevationVelocity_ = 0.0f;
    float velocityDecay_ = 0.95f;
    bool inertiaEnabled_ = false;

    // Helper methods
    void startInertia(float azimuthVel, float elevationVel);
    void stopInertia();
    void updateViewMatrix();
};
