// RadarSceneWidget.cpp - Component-based radar scene container
#include "RadarSceneWidget.h"

RadarSceneWidget::RadarSceneWidget(QWidget* parent)
    : QWidget(parent),
    radarGLWidget_(nullptr),
    layout_(new QVBoxLayout(this)),
    sphereRenderer_(nullptr),
    beamController_(nullptr),
    cameraController_(nullptr),
    modelManager_(nullptr),
    wireframeController_(nullptr)
{
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    radarGLWidget_ = new RadarGLWidget(this);
    createComponents();

    connect(radarGLWidget_, &RadarGLWidget::radiusChanged,
        this, &RadarSceneWidget::onRadiusChanged);
    connect(radarGLWidget_, &RadarGLWidget::anglesChanged,
        this, &RadarSceneWidget::onAnglesChanged);

    layout_->addWidget(radarGLWidget_);
    setLayout(layout_);
}

RadarSceneWidget::~RadarSceneWidget() {
    delete sphereRenderer_;
    delete beamController_;
    delete cameraController_;
    delete modelManager_;
    delete wireframeController_;
}

void RadarSceneWidget::createComponents() {
    sphereRenderer_ = new SphereRenderer(this);
    beamController_ = new BeamController(this);
    cameraController_ = new CameraController(this);
    modelManager_ = new ModelManager(this);
    wireframeController_ = new WireframeTargetController(this);

    radarGLWidget_->initialize(sphereRenderer_, beamController_,
        cameraController_, modelManager_, wireframeController_);
}

void RadarSceneWidget::updateScene() {
    if (radarGLWidget_) {
        radarGLWidget_->update();
    }
}

void RadarSceneWidget::onRadiusChanged(float radius) {
    emit radarPositionChanged(radius, getTheta(), getPhi());
}

void RadarSceneWidget::onAnglesChanged(float theta, float phi) {
    emit radarPositionChanged(getRadius(), theta, phi);
}

// --- Forwarding methods --- //

void RadarSceneWidget::setRadius(float radius) {
    radarGLWidget_->setRadius(radius);
    emit radarPositionChanged(radius, getTheta(), getPhi());
}

float RadarSceneWidget::getRadius() const {
    return radarGLWidget_->getRadius();
}

void RadarSceneWidget::setAngles(float theta, float phi) {
    radarGLWidget_->setAngles(theta, phi);
    emit radarPositionChanged(getRadius(), theta, phi);
}

float RadarSceneWidget::getTheta() const {
    return radarGLWidget_->getTheta();
}

float RadarSceneWidget::getPhi() const {
    return radarGLWidget_->getPhi();
}

void RadarSceneWidget::setBeamWidth(float degrees) {
    if (beamController_) {
        beamController_->setBeamWidth(degrees);
        radarGLWidget_->update();
    }
    emit beamWidthChanged(degrees);
}

void RadarSceneWidget::setBeamType(BeamType type) {
    if (beamController_) {
        beamController_->setBeamType(type);
        radarGLWidget_->update();
    }
    emit beamTypeChanged(type);
}

void RadarSceneWidget::setBeamColor(const QVector3D& color) {
    if (beamController_) {
        beamController_->setBeamColor(color);
        radarGLWidget_->update();
    }
}

void RadarSceneWidget::setBeamOpacity(float opacity) {
    if (beamController_) {
        beamController_->setBeamOpacity(opacity);
        radarGLWidget_->update();
    }
}

RadarBeam* RadarSceneWidget::getBeam() const {
    return beamController_ ? beamController_->getBeam() : nullptr;
}

void RadarSceneWidget::setSphereVisible(bool visible) {
    if (sphereRenderer_) {
        sphereRenderer_->setSphereVisible(visible);
        radarGLWidget_->update();
    }
    emit visibilityOptionChanged("sphere", visible);
}

void RadarSceneWidget::setAxesVisible(bool visible) {
    if (sphereRenderer_) {
        sphereRenderer_->setAxesVisible(visible);
        radarGLWidget_->update();
    }
    emit visibilityOptionChanged("axes", visible);
}

void RadarSceneWidget::setGridLinesVisible(bool visible) {
    if (sphereRenderer_) {
        sphereRenderer_->setGridLinesVisible(visible);
        radarGLWidget_->update();
    }
    emit visibilityOptionChanged("gridLines", visible);
}

void RadarSceneWidget::setInertiaEnabled(bool enabled) {
    if (cameraController_) {
        cameraController_->setInertiaEnabled(enabled);
    }
    emit visibilityOptionChanged("inertia", enabled);
}

bool RadarSceneWidget::isSphereVisible() const {
    return sphereRenderer_ ? sphereRenderer_->isSphereVisible() : false;
}

bool RadarSceneWidget::areAxesVisible() const {
    return sphereRenderer_ ? sphereRenderer_->areAxesVisible() : false;
}

bool RadarSceneWidget::areGridLinesVisible() const {
    return sphereRenderer_ ? sphereRenderer_->areGridLinesVisible() : false;
}

bool RadarSceneWidget::isInertiaEnabled() const {
    return cameraController_ ? cameraController_->isInertiaEnabled() : false;
}
