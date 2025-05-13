// RadarSceneWidget.cpp
#include "RadarSceneWidget.h"
#include <QActionGroup>
#include <QDebug>

RadarSceneWidget::RadarSceneWidget(QWidget* parent)
    : QWidget(parent),
    sphereWidget_(nullptr),
    layout_(new QVBoxLayout(this)),
    sphereRenderer_(nullptr),
    beamController_(nullptr),
    cameraController_(nullptr),
    modelManager_(nullptr)
{
    qDebug() << "Creating RadarSceneWidget";

    // Set up layout properties
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    // Create the SphereWidget and add it to the layout
    sphereWidget_ = new SphereWidget(this);
    layout_->addWidget(sphereWidget_);

    // Initialize components (but don't use them yet)
    initializeComponents();

    // Set the layout
    setLayout(layout_);

    qDebug() << "RadarSceneWidget constructor complete";
}

RadarSceneWidget::~RadarSceneWidget() {
    qDebug() << "RadarSceneWidget destructor called";

    // Clean up components
    delete sphereRenderer_;
    delete beamController_;
    delete cameraController_;
    delete modelManager_;
}

void RadarSceneWidget::initializeComponents() {
    qDebug() << "Initializing RadarSceneWidget components";

    // Create components
    sphereRenderer_ = new SphereRenderer(this);
    beamController_ = new BeamController(this);
    cameraController_ = new CameraController(this);
    modelManager_ = new ModelManager(this);

    // Eventually, these components will be initialized with OpenGL context
    // and will replace SphereWidget functionality

    qDebug() << "RadarSceneWidget components created";
}

// --- Forwarding methods to SphereWidget --- //

void RadarSceneWidget::setRadius(float radius) {
    if (sphereWidget_) {
        sphereWidget_->setRadius(radius);
        emit radarPositionChanged(radius, getTheta(), getPhi());
    }
}

void RadarSceneWidget::setAngles(float theta, float phi) {
    if (sphereWidget_) {
        sphereWidget_->setAngles(theta, phi);
        emit radarPositionChanged(sphereWidget_->getRadius(), theta, phi);
    }
}

float RadarSceneWidget::getTheta() const {
    return sphereWidget_ ? sphereWidget_->getTheta() : 0.0f;
}

float RadarSceneWidget::getPhi() const {
    return sphereWidget_ ? sphereWidget_->getPhi() : 0.0f;
}

void RadarSceneWidget::setBeamWidth(float degrees) {
    if (sphereWidget_) {
        sphereWidget_->setBeamWidth(degrees);
        emit beamWidthChanged(degrees);
    }
}

void RadarSceneWidget::setBeamType(BeamType type) {
    if (sphereWidget_) {
        sphereWidget_->setBeamType(type);
        emit beamTypeChanged(type);
    }
}

void RadarSceneWidget::setBeamColor(const QVector3D& color) {
    if (sphereWidget_) {
        sphereWidget_->setBeamColor(color);
    }
}

void RadarSceneWidget::setBeamOpacity(float opacity) {
    if (sphereWidget_) {
        sphereWidget_->setBeamOpacity(opacity);
    }
}

RadarBeam* RadarSceneWidget::getBeam() const {
    return sphereWidget_ ? sphereWidget_->getBeam() : nullptr;
}

void RadarSceneWidget::setSphereVisible(bool visible) {
    if (sphereWidget_) {
        sphereWidget_->setSphereVisible(visible);
        emit visibilityOptionChanged("sphere", visible);
    }
}

void RadarSceneWidget::setAxesVisible(bool visible) {
    if (sphereWidget_) {
        sphereWidget_->setAxesVisible(visible);
        emit visibilityOptionChanged("axes", visible);
    }
}

void RadarSceneWidget::setGridLinesVisible(bool visible) {
    if (sphereWidget_) {
        sphereWidget_->setGridLinesVisible(visible);
        emit visibilityOptionChanged("gridLines", visible);
    }
}

void RadarSceneWidget::setInertiaEnabled(bool enabled) {
    if (sphereWidget_) {
        sphereWidget_->setInertiaEnabled(enabled);
        emit visibilityOptionChanged("inertia", enabled);
    }
}

bool RadarSceneWidget::isSphereVisible() const {
    return sphereWidget_ ? sphereWidget_->isSphereVisible() : false;
}

bool RadarSceneWidget::areAxesVisible() const {
    return sphereWidget_ ? sphereWidget_->areAxesVisible() : false;
}

bool RadarSceneWidget::areGridLinesVisible() const {
    return sphereWidget_ ? sphereWidget_->areGridLinesVisible() : false;
}

bool RadarSceneWidget::isInertiaEnabled() const {
    return sphereWidget_ ? sphereWidget_->isInertiaEnabled() : false;
}