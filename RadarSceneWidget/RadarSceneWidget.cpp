// --- RadarSceneWidget.cpp ----

#include "RadarSceneWidget.h"

RadarSceneWidget::RadarSceneWidget(QWidget* parent) : QWidget(parent)
{
    // Create a layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create SphereWidget with proper parent
    sphereWidget_ = new SphereWidget(this);
    layout->addWidget(sphereWidget_);

    m_RadarEntity = std::make_shared<Entity>("RadarEntity");
    m_SphereRenderer = m_RadarEntity->AddComponent<SphereRenderer>(1.0f, 36);
    m_SphereRenderer->SetShaderProgram(&m_program); // Use your existing shader program
    m_SphereRenderer->SetColor(QVector4D(0.0f, 1.0f, 0.0f, 0.5f));
}

RadarSceneWidget::~RadarSceneWidget() {
    // Components will be deleted automatically as children
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

void RadarSceneWidget::setSphereVisible(bool visible)
{
    if (m_SphereRenderer) {
        m_SphereRenderer->SetVisible(visible);
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