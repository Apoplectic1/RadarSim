// CameraController.h
#pragma once

#include <QObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>

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

    // Camera state getters
    float getDistance() const { return distance_; }
    float getAzimuth() const { return azimuth_; }
    float getElevation() const { return elevation_; }
    QVector3D getFocusPoint() const { return focusPoint_; }

    // Camera state setters
    void setDistance(float d);
    void setAzimuth(float a);
    void setElevation(float e);
    void setFocusPoint(const QVector3D& fp);

    // Mouse interaction
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

signals:
    void viewChanged();

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

    // Helper methods
    void updateViewMatrix();
};
