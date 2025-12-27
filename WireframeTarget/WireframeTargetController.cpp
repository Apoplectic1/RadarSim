// WireframeTargetController.cpp

#include "WireframeTargetController.h"
#include <QDebug>

WireframeTargetController::WireframeTargetController(QObject* parent)
    : QObject(parent),
      currentType_(WireframeType::Cube),
      pendingType_(WireframeType::Cube),
      typeChangePending_(false),
      position_(0.0f, 0.0f, 0.0f),
      rotation_(0.0f, 0.0f, 0.0f),
      scale_(20.0f),
      color_(0.0f, 1.0f, 0.0f),
      showTarget_(true),
      illuminated_(false),
      lightDirection_(0.0f, 0.0f, 1.0f)
{
}

WireframeTargetController::~WireframeTargetController() = default;

void WireframeTargetController::cleanup() {
    if (target_) {
        target_->cleanup();
    }
    qDebug() << "WireframeTargetController::cleanup() - cleaned up OpenGL resources";
}

void WireframeTargetController::initialize() {
    qDebug() << "Initializing WireframeTargetController";

    // Create initial target
    createTarget();

    qDebug() << "WireframeTargetController initialization complete";
}

void WireframeTargetController::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    if (!showTarget_ || !target_) {
        return;
    }
    target_->render(projection, view, model);
}

void WireframeTargetController::rebuildGeometry() {
    // Handle deferred type change (requires GL context for proper cleanup)
    if (typeChangePending_) {
        currentType_ = pendingType_;
        typeChangePending_ = false;

        // Re-create target with new type (now we have GL context)
        createTarget();

        emit targetTypeChanged(currentType_);
    }

    if (target_) {
        target_->uploadGeometryToGPU();
    }
}

void WireframeTargetController::setTargetType(WireframeType type) {
    if (currentType_ != type) {
        // Defer the actual target creation to when we have a GL context
        pendingType_ = type;
        typeChangePending_ = true;
    }
}

WireframeType WireframeTargetController::getTargetType() const {
    return currentType_;
}

void WireframeTargetController::setPosition(const QVector3D& position) {
    if (position_ != position) {
        position_ = position;
        if (target_) {
            target_->setPosition(position);
        }
        emit positionChanged(position);
    }
}

void WireframeTargetController::setPosition(float x, float y, float z) {
    setPosition(QVector3D(x, y, z));
}

QVector3D WireframeTargetController::getPosition() const {
    return position_;
}

void WireframeTargetController::setRotation(const QVector3D& eulerAngles) {
    if (rotation_ != eulerAngles) {
        rotation_ = eulerAngles;
        if (target_) {
            target_->setRotation(eulerAngles);
        }
        emit rotationChanged(eulerAngles);
    }
}

void WireframeTargetController::setRotation(float pitch, float yaw, float roll) {
    setRotation(QVector3D(pitch, yaw, roll));
}

QVector3D WireframeTargetController::getRotation() const {
    return rotation_;
}

void WireframeTargetController::setScale(float scale) {
    if (scale_ != scale) {
        scale_ = scale;
        if (target_) {
            target_->setScale(scale);
        }
        emit scaleChanged(scale);
    }
}

float WireframeTargetController::getScale() const {
    return scale_;
}

float WireframeTargetController::getBoundingRadius() const {
    // Approximate bounding sphere radius based on scale
    // For most shapes, scale is a reasonable approximation
    return scale_ * 1.0f;
}

void WireframeTargetController::setColor(const QVector3D& color) {
    color_ = color;
    if (target_) {
        target_->setColor(color);
    }
}

QVector3D WireframeTargetController::getColor() const {
    return color_;
}

void WireframeTargetController::setVisible(bool visible) {
    if (showTarget_ != visible) {
        showTarget_ = visible;
        if (target_) {
            target_->setVisible(visible);
        }
        emit visibilityChanged(visible);
    }
}

bool WireframeTargetController::isVisible() const {
    return showTarget_;
}

void WireframeTargetController::setIlluminated(bool illuminated) {
    illuminated_ = illuminated;
    if (target_) {
        target_->setIlluminated(illuminated);
    }
}

bool WireframeTargetController::isIlluminated() const {
    return illuminated_;
}

void WireframeTargetController::setLightDirection(const QVector3D& direction) {
    lightDirection_ = direction;
    if (target_) {
        target_->setLightDirection(direction);
    }
}

QVector3D WireframeTargetController::getLightDirection() const {
    return lightDirection_;
}

void WireframeTargetController::createTarget() {
    // Create new target using factory (unique_ptr automatically deletes old target)
    target_.reset(WireframeTarget::createTarget(currentType_));

    if (target_) {
        target_->initialize();

        // Apply cached properties
        target_->setPosition(position_);
        target_->setRotation(rotation_);
        target_->setScale(scale_);
        target_->setColor(color_);
        target_->setVisible(showTarget_);
        target_->setIlluminated(illuminated_);
        target_->setLightDirection(lightDirection_);
    }
}
