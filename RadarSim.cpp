// ---- RadarSim.cpp ----

#include "RadarSim.h"
#include "SphereWidget.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

RadarSim::RadarSim(QWidget* parent)
    : QMainWindow(parent),
    sphereView_(new SphereWidget(this)),
    radiusSlider_(nullptr),
    thetaSlider_(nullptr),
    phiSlider_(nullptr),
    radiusSpinBox_(nullptr),
    thetaSpinBox_(nullptr),
    phiSpinBox_(nullptr)
{
    setupUI();
    connectSignals();
}

void RadarSim::setupUI() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    setCentralWidget(central);

    // 3D view
    sphereView_->setMinimumSize(800, 800);
    layout->addWidget(sphereView_);

    // Radius
    layout->addWidget(new QLabel("Sphere Radius", central));
    auto* radiusLayout = new QHBoxLayout();
    radiusSlider_ = new QSlider(Qt::Horizontal, central);
    radiusSlider_->setRange(50, 300);
    radiusSlider_->setValue(100);
    radiusSpinBox_ = new QSpinBox(central);
    radiusSpinBox_->setRange(50, 300);
    radiusSpinBox_->setValue(100);
    radiusLayout->addWidget(radiusSlider_);
    radiusLayout->addWidget(radiusSpinBox_);
    layout->addLayout(radiusLayout);

    // Azimuth
    // Azimuth
    layout->addWidget(new QLabel("Radar Azimuth (θ)", central));
    auto* thetaLayout = new QHBoxLayout();
    thetaSlider_ = new QSlider(Qt::Horizontal, central);
    thetaSlider_->setRange(0, 359);

    // Initialize to a value that works well with the reversed sense
    int initialAzimuth = 45;
    thetaSlider_->setValue(359 - initialAzimuth); // Use reversed value for slider

    thetaSpinBox_ = new QSpinBox(central);
    thetaSpinBox_->setRange(0, 359);
    thetaSpinBox_->setValue(initialAzimuth); // Use direct value for spin box

    thetaLayout->addWidget(thetaSlider_);
    thetaLayout->addWidget(thetaSpinBox_);
    layout->addLayout(thetaLayout);

    // Elevation
    layout->addWidget(new QLabel("Radar Elevation (φ)", central));
    auto* phiLayout = new QHBoxLayout();
    phiSlider_ = new QSlider(Qt::Horizontal, central);
    phiSlider_->setRange(-90, 90);
    phiSlider_->setValue(45);
    phiSpinBox_ = new QSpinBox(central);
    phiSpinBox_->setRange(-90, 90);
    phiSpinBox_->setValue(45);
    phiLayout->addWidget(phiSlider_);
    phiLayout->addWidget(phiSpinBox_);
    layout->addLayout(phiLayout);
}

void RadarSim::connectSignals() {
    // Connect radius slider and spin box
    connect(radiusSlider_, &QSlider::valueChanged, this, &RadarSim::onRadiusSliderValueChanged);
    connect(radiusSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onRadiusSpinBoxValueChanged);

    // Connect theta (azimuth) slider and spin box
    connect(thetaSlider_, &QSlider::valueChanged, this, &RadarSim::onThetaSliderValueChanged);
    connect(thetaSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onThetaSpinBoxValueChanged);

    // Connect phi (elevation) slider and spin box
    connect(phiSlider_, &QSlider::valueChanged, this, &RadarSim::onPhiSliderValueChanged);
    connect(phiSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onPhiSpinBoxValueChanged);
}

// Slot implementations
void RadarSim::onRadiusSliderValueChanged(int value) {
    // Block signals to prevent circular updates
    radiusSpinBox_->blockSignals(true);
    radiusSpinBox_->setValue(value);
    radiusSpinBox_->blockSignals(false);

    // Update the sphere widget
    sphereView_->setRadius(value);
}

void RadarSim::onThetaSliderValueChanged(int value) {
    // Block signals to prevent circular updates
    thetaSpinBox_->blockSignals(true);

    // Reverse the sense for Azimuth slider
    int reversedValue = 359 - value;

    // Update the spin box with the reversed value
    thetaSpinBox_->setValue(reversedValue);
    thetaSpinBox_->blockSignals(false);

    // Update the sphere widget with the reversed value
    sphereView_->setAngles(reversedValue, sphereView_->getPhi());
}

void RadarSim::onPhiSliderValueChanged(int value) {
    // Block signals to prevent circular updates
    phiSpinBox_->blockSignals(true);
    phiSpinBox_->setValue(value);
    phiSpinBox_->blockSignals(false);

    // Update the sphere widget
    sphereView_->setAngles(sphereView_->getTheta(), value);
}

void RadarSim::onRadiusSpinBoxValueChanged(int value) {
    // Block signals to prevent circular updates
    radiusSlider_->blockSignals(true);
    radiusSlider_->setValue(value);
    radiusSlider_->blockSignals(false);

    // Update the sphere widget
    sphereView_->setRadius(value);
}

void RadarSim::onThetaSpinBoxValueChanged(int value) {
    // Block signals to prevent circular updates
    thetaSlider_->blockSignals(true);

    // Set slider to reversed value (for display purposes)
    thetaSlider_->setValue(359 - value);
    thetaSlider_->blockSignals(false);

    // Update the sphere widget with the direct spin box value
    sphereView_->setAngles(value, sphereView_->getPhi());
}

void RadarSim::onPhiSpinBoxValueChanged(int value) {
    // Block signals to prevent circular updates
    phiSlider_->blockSignals(true);
    phiSlider_->setValue(value);
    phiSlider_->blockSignals(false);

    // Update the sphere widget
    sphereView_->setAngles(sphereView_->getTheta(), value);
}