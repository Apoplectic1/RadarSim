// RadarControlsWidget.cpp - Self-contained radar control panel widget

#include "RadarControlsWidget.h"
#include "SceneConfig.h"
#include "Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMouseEvent>

RadarControlsWidget::RadarControlsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
}

void RadarControlsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QGroupBox* controlsGroup = new QGroupBox("Radar Controls", this);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(6, 0, 6, 0);
    mainLayout->addWidget(controlsGroup);

    // Radius controls
    QLabel* radiusLabel = new QLabel("Sphere Radius", controlsGroup);
    controlsLayout->addWidget(radiusLabel);

    QHBoxLayout* radiusLayout = new QHBoxLayout();
    radiusLayout->setContentsMargins(0, 0, 0, 0);
    radiusSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    radiusSlider_->setRange(50, 300);
    radiusSlider_->setValue(100);

    radiusSpinBox_ = new QSpinBox(controlsGroup);
    radiusSpinBox_->setRange(50, 300);
    radiusSpinBox_->setValue(100);
    radiusSpinBox_->setMinimumWidth(60);

    radiusLayout->addWidget(radiusSlider_);
    radiusLayout->addWidget(radiusSpinBox_);
    controlsLayout->addLayout(radiusLayout);

    // Azimuth controls (0.5 degree increments: slider 0-718 maps to 0-359 degrees)
    QLabel* thetaLabel = new QLabel("Radar Azimuth (\316\270)", controlsGroup);  // θ character
    controlsLayout->addWidget(thetaLabel);

    QHBoxLayout* thetaLayout = new QHBoxLayout();
    thetaLayout->setContentsMargins(0, 0, 0, 0);
    thetaSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    thetaSlider_->setRange(0, 718);  // 0-359 degrees in 0.5 steps
    double initialAzimuth = 45.0;
    thetaSlider_->setValue(static_cast<int>((359.0 - initialAzimuth) * 2));

    thetaSpinBox_ = new QDoubleSpinBox(controlsGroup);
    thetaSpinBox_->setRange(0.0, 359.0);
    thetaSpinBox_->setSingleStep(0.5);
    thetaSpinBox_->setDecimals(1);
    thetaSpinBox_->setValue(initialAzimuth);
    thetaSpinBox_->setMinimumWidth(60);

    thetaLayout->addWidget(thetaSlider_);
    thetaLayout->addWidget(thetaSpinBox_);
    controlsLayout->addLayout(thetaLayout);

    // Elevation controls (0.5 degree increments: slider -180 to 180 maps to -90 to 90 degrees)
    QLabel* phiLabel = new QLabel("Radar Elevation (\317\206)", controlsGroup);  // φ character
    controlsLayout->addWidget(phiLabel);

    QHBoxLayout* phiLayout = new QHBoxLayout();
    phiLayout->setContentsMargins(0, 0, 0, 0);
    phiSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    phiSlider_->setRange(-180, 180);  // -90 to 90 degrees in 0.5 steps
    phiSlider_->setValue(90);  // 45 degrees * 2

    phiSpinBox_ = new QDoubleSpinBox(controlsGroup);
    phiSpinBox_->setRange(-90.0, 90.0);
    phiSpinBox_->setSingleStep(0.5);
    phiSpinBox_->setDecimals(1);
    phiSpinBox_->setValue(45.0);
    phiSpinBox_->setMinimumWidth(60);

    phiLayout->addWidget(phiSlider_);
    phiLayout->addWidget(phiSpinBox_);
    controlsLayout->addLayout(phiLayout);

    // Install event filters for double-click reset
    radiusSlider_->installEventFilter(this);
    thetaSlider_->installEventFilter(this);
    phiSlider_->installEventFilter(this);
}

void RadarControlsWidget::connectSignals() {
    // Connect radius slider and spin box
    connect(radiusSlider_, &QSlider::valueChanged, this, &RadarControlsWidget::onRadiusSliderChanged);
    connect(radiusSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarControlsWidget::onRadiusSpinBoxChanged);

    // Connect theta (azimuth) slider and spin box
    connect(thetaSlider_, &QSlider::valueChanged, this, &RadarControlsWidget::onThetaSliderChanged);
    connect(thetaSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarControlsWidget::onThetaSpinBoxChanged);

    // Connect phi (elevation) slider and spin box
    connect(phiSlider_, &QSlider::valueChanged, this, &RadarControlsWidget::onPhiSliderChanged);
    connect(phiSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarControlsWidget::onPhiSpinBoxChanged);
}

// Getters
int RadarControlsWidget::getRadius() const {
    return radiusSpinBox_->value();
}

float RadarControlsWidget::getTheta() const {
    return static_cast<float>(thetaSpinBox_->value());
}

float RadarControlsWidget::getPhi() const {
    return static_cast<float>(phiSpinBox_->value());
}

// Public slots
void RadarControlsWidget::setRadius(int radius) {
    radiusSlider_->blockSignals(true);
    radiusSpinBox_->blockSignals(true);
    radiusSlider_->setValue(radius);
    radiusSpinBox_->setValue(radius);
    radiusSlider_->blockSignals(false);
    radiusSpinBox_->blockSignals(false);
}

void RadarControlsWidget::setAngles(float theta, float phi) {
    thetaSlider_->blockSignals(true);
    thetaSpinBox_->blockSignals(true);
    // Convert degrees to half-degrees and reverse for slider
    thetaSlider_->setValue(static_cast<int>((359.0 - theta) * 2));
    thetaSpinBox_->setValue(static_cast<double>(theta));
    thetaSlider_->blockSignals(false);
    thetaSpinBox_->blockSignals(false);

    phiSlider_->blockSignals(true);
    phiSpinBox_->blockSignals(true);
    // Convert degrees to half-degrees for slider
    phiSlider_->setValue(static_cast<int>(phi * 2));
    phiSpinBox_->setValue(static_cast<double>(phi));
    phiSlider_->blockSignals(false);
    phiSpinBox_->blockSignals(false);
}

// Settings persistence
void RadarControlsWidget::readSettings(RSConfig::SceneConfig& config) const {
    config.sphereRadius = static_cast<float>(radiusSpinBox_->value());
    config.radarTheta = static_cast<float>(thetaSpinBox_->value());
    config.radarPhi = static_cast<float>(phiSpinBox_->value());
}

void RadarControlsWidget::applySettings(const RSConfig::SceneConfig& config) {
    setRadius(static_cast<int>(config.sphereRadius));
    setAngles(config.radarTheta, config.radarPhi);
}

// Private slots
void RadarControlsWidget::onRadiusSliderChanged(int value) {
    radiusSpinBox_->blockSignals(true);
    radiusSpinBox_->setValue(value);
    radiusSpinBox_->blockSignals(false);

    emit radiusChanged(value);
}

void RadarControlsWidget::onRadiusSpinBoxChanged(int value) {
    radiusSlider_->blockSignals(true);
    radiusSlider_->setValue(value);
    radiusSlider_->blockSignals(false);

    emit radiusChanged(value);
}

void RadarControlsWidget::onThetaSliderChanged(int value) {
    thetaSpinBox_->blockSignals(true);
    // Slider value is in half-degrees (0-718), convert to degrees (0-359)
    // Also reverse the sense for Azimuth slider
    double degrees = (718 - value) / 2.0;
    thetaSpinBox_->setValue(degrees);
    thetaSpinBox_->blockSignals(false);

    emit anglesChanged(static_cast<float>(degrees), getPhi());
}

void RadarControlsWidget::onThetaSpinBoxChanged(double value) {
    thetaSlider_->blockSignals(true);
    // Convert degrees to half-degrees and reverse for slider
    int sliderValue = static_cast<int>((359.0 - value) * 2);
    thetaSlider_->setValue(sliderValue);
    thetaSlider_->blockSignals(false);

    emit anglesChanged(static_cast<float>(value), getPhi());
}

void RadarControlsWidget::onPhiSliderChanged(int value) {
    phiSpinBox_->blockSignals(true);
    // Slider value is in half-degrees (-180 to 180), convert to degrees (-90 to 90)
    double degrees = value / 2.0;
    phiSpinBox_->setValue(degrees);
    phiSpinBox_->blockSignals(false);

    emit anglesChanged(getTheta(), static_cast<float>(degrees));
}

void RadarControlsWidget::onPhiSpinBoxChanged(double value) {
    phiSlider_->blockSignals(true);
    // Convert degrees to half-degrees for slider
    int sliderValue = static_cast<int>(value * 2);
    phiSlider_->setValue(sliderValue);
    phiSlider_->blockSignals(false);

    emit anglesChanged(getTheta(), static_cast<float>(value));
}

// Event filter for double-click reset
bool RadarControlsWidget::eventFilter(QObject* obj, QEvent* event) {
    using namespace RS::Constants::Defaults;

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (obj == radiusSlider_) {
                radiusSlider_->setValue(static_cast<int>(kSphereRadius));
                return true;
            }
            if (obj == thetaSlider_) {
                // Slider is reversed and in half-degrees
                thetaSlider_->setValue(static_cast<int>((359.0f - kRadarTheta) * 2));
                return true;
            }
            if (obj == phiSlider_) {
                // Slider is in half-degrees
                phiSlider_->setValue(static_cast<int>(kRadarPhi * 2));
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
