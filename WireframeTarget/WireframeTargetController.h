// WireframeTargetController.h
#pragma once

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>

#include "WireframeTarget.h"
#include "WireframeShapes.h"

class WireframeTargetController : public QObject {
    Q_OBJECT

public:
    explicit WireframeTargetController(QObject* parent = nullptr);
    ~WireframeTargetController();

    // Lifecycle
    void initialize();
    void render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model,
                const QVector3D& radarPosition = QVector3D(), float sphereRadius = 100.0f);
    void rebuildGeometry();  // Called from paintGL with valid GL context

    // Target type management
    void setTargetType(WireframeType type);
    WireframeType getTargetType() const;

    // Transform controls
    void setPosition(const QVector3D& position);
    void setPosition(float x, float y, float z);
    QVector3D getPosition() const;

    void setRotation(const QVector3D& eulerAngles);
    void setRotation(float pitch, float yaw, float roll);
    QVector3D getRotation() const;

    void setScale(float scale);
    float getScale() const;

    // Appearance
    void setColor(const QVector3D& color);
    QVector3D getColor() const;

    void setVisible(bool visible);
    bool isVisible() const;

    // Future: Illumination API
    void setIlluminated(bool illuminated);
    bool isIlluminated() const;

    void setLightDirection(const QVector3D& direction);
    QVector3D getLightDirection() const;

    // Access to underlying target
    WireframeTarget* getTarget() const { return target_; }

signals:
    void targetTypeChanged(WireframeType type);
    void positionChanged(const QVector3D& position);
    void rotationChanged(const QVector3D& eulerAngles);
    void scaleChanged(float scale);
    void visibilityChanged(bool visible);

private:
    WireframeTarget* target_ = nullptr;

    // Deferred type change (following BeamController pattern)
    WireframeType currentType_ = WireframeType::Cube;
    WireframeType pendingType_ = WireframeType::Cube;
    bool typeChangePending_ = false;

    // Cached properties (for recreation)
    QVector3D position_ = QVector3D(0.0f, 0.0f, 0.0f);
    QVector3D rotation_ = QVector3D(0.0f, 0.0f, 0.0f);
    float scale_ = 20.0f;  // Default scale for visibility
    QVector3D color_ = QVector3D(0.0f, 1.0f, 0.0f);
    bool showTarget_ = true;
    bool illuminated_ = false;
    QVector3D lightDirection_ = QVector3D(0.0f, 0.0f, 1.0f);

    void createTarget();
};
