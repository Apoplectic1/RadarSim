// TargetControlsWidget.cpp - Self-contained target control panel widget

#include "TargetControlsWidget.h"
#include "TargetConfig.h"
#include "Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMouseEvent>

TargetControlsWidget::TargetControlsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
}

void TargetControlsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QGroupBox* controlsGroup = new QGroupBox("Target Controls", this);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(6, 0, 6, 0);
    mainLayout->addWidget(controlsGroup);

    // Position X (0.5 increments: slider -200 to 200 maps to -100 to 100)
    QLabel* posXLabel = new QLabel("Position X", controlsGroup);
    controlsLayout->addWidget(posXLabel);

    QHBoxLayout* posXLayout = new QHBoxLayout();
    posXLayout->setContentsMargins(0, 0, 0, 0);
    posXSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    posXSlider_->setRange(-200, 200);
    posXSlider_->setValue(0);

    posXSpinBox_ = new QDoubleSpinBox(controlsGroup);
    posXSpinBox_->setRange(-100.0, 100.0);
    posXSpinBox_->setSingleStep(0.5);
    posXSpinBox_->setDecimals(1);
    posXSpinBox_->setValue(0.0);
    posXSpinBox_->setMinimumWidth(60);

    posXLayout->addWidget(posXSlider_);
    posXLayout->addWidget(posXSpinBox_);
    controlsLayout->addLayout(posXLayout);

    // Position Y
    QLabel* posYLabel = new QLabel("Position Y", controlsGroup);
    controlsLayout->addWidget(posYLabel);

    QHBoxLayout* posYLayout = new QHBoxLayout();
    posYLayout->setContentsMargins(0, 0, 0, 0);
    posYSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    posYSlider_->setRange(-200, 200);
    posYSlider_->setValue(0);

    posYSpinBox_ = new QDoubleSpinBox(controlsGroup);
    posYSpinBox_->setRange(-100.0, 100.0);
    posYSpinBox_->setSingleStep(0.5);
    posYSpinBox_->setDecimals(1);
    posYSpinBox_->setValue(0.0);
    posYSpinBox_->setMinimumWidth(60);

    posYLayout->addWidget(posYSlider_);
    posYLayout->addWidget(posYSpinBox_);
    controlsLayout->addLayout(posYLayout);

    // Position Z
    QLabel* posZLabel = new QLabel("Position Z", controlsGroup);
    controlsLayout->addWidget(posZLabel);

    QHBoxLayout* posZLayout = new QHBoxLayout();
    posZLayout->setContentsMargins(0, 0, 0, 0);
    posZSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    posZSlider_->setRange(-200, 200);
    posZSlider_->setValue(0);

    posZSpinBox_ = new QDoubleSpinBox(controlsGroup);
    posZSpinBox_->setRange(-100.0, 100.0);
    posZSpinBox_->setSingleStep(0.5);
    posZSpinBox_->setDecimals(1);
    posZSpinBox_->setValue(0.0);
    posZSpinBox_->setMinimumWidth(60);

    posZLayout->addWidget(posZSlider_);
    posZLayout->addWidget(posZSpinBox_);
    controlsLayout->addLayout(posZLayout);

    // Pitch (0.5 degree increments: slider -360 to 360 maps to -180 to 180)
    QLabel* pitchLabel = new QLabel("Pitch (X rotation)", controlsGroup);
    controlsLayout->addWidget(pitchLabel);

    QHBoxLayout* pitchLayout = new QHBoxLayout();
    pitchLayout->setContentsMargins(0, 0, 0, 0);
    pitchSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    pitchSlider_->setRange(-360, 360);
    pitchSlider_->setValue(0);

    pitchSpinBox_ = new QDoubleSpinBox(controlsGroup);
    pitchSpinBox_->setRange(-180.0, 180.0);
    pitchSpinBox_->setSingleStep(0.5);
    pitchSpinBox_->setDecimals(1);
    pitchSpinBox_->setValue(0.0);
    pitchSpinBox_->setMinimumWidth(60);

    pitchLayout->addWidget(pitchSlider_);
    pitchLayout->addWidget(pitchSpinBox_);
    controlsLayout->addLayout(pitchLayout);

    // Yaw
    QLabel* yawLabel = new QLabel("Yaw (Y rotation)", controlsGroup);
    controlsLayout->addWidget(yawLabel);

    QHBoxLayout* yawLayout = new QHBoxLayout();
    yawLayout->setContentsMargins(0, 0, 0, 0);
    yawSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    yawSlider_->setRange(-360, 360);
    yawSlider_->setValue(0);

    yawSpinBox_ = new QDoubleSpinBox(controlsGroup);
    yawSpinBox_->setRange(-180.0, 180.0);
    yawSpinBox_->setSingleStep(0.5);
    yawSpinBox_->setDecimals(1);
    yawSpinBox_->setValue(0.0);
    yawSpinBox_->setMinimumWidth(60);

    yawLayout->addWidget(yawSlider_);
    yawLayout->addWidget(yawSpinBox_);
    controlsLayout->addLayout(yawLayout);

    // Roll
    QLabel* rollLabel = new QLabel("Roll (Z rotation)", controlsGroup);
    controlsLayout->addWidget(rollLabel);

    QHBoxLayout* rollLayout = new QHBoxLayout();
    rollLayout->setContentsMargins(0, 0, 0, 0);
    rollSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    rollSlider_->setRange(-360, 360);
    rollSlider_->setValue(0);

    rollSpinBox_ = new QDoubleSpinBox(controlsGroup);
    rollSpinBox_->setRange(-180.0, 180.0);
    rollSpinBox_->setSingleStep(0.5);
    rollSpinBox_->setDecimals(1);
    rollSpinBox_->setValue(0.0);
    rollSpinBox_->setMinimumWidth(60);

    rollLayout->addWidget(rollSlider_);
    rollLayout->addWidget(rollSpinBox_);
    controlsLayout->addLayout(rollLayout);

    // Scale (0.5 increments: slider 2 to 200 maps to 1 to 100)
    QLabel* scaleLabel = new QLabel("Scale", controlsGroup);
    controlsLayout->addWidget(scaleLabel);

    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    scaleSlider_->setRange(2, 200);
    scaleSlider_->setValue(40);  // 20 * 2

    scaleSpinBox_ = new QDoubleSpinBox(controlsGroup);
    scaleSpinBox_->setRange(1.0, 100.0);
    scaleSpinBox_->setSingleStep(0.5);
    scaleSpinBox_->setDecimals(1);
    scaleSpinBox_->setValue(20.0);
    scaleSpinBox_->setMinimumWidth(60);

    scaleLayout->addWidget(scaleSlider_);
    scaleLayout->addWidget(scaleSpinBox_);
    controlsLayout->addLayout(scaleLayout);

    // Install event filters for double-click reset
    posXSlider_->installEventFilter(this);
    posYSlider_->installEventFilter(this);
    posZSlider_->installEventFilter(this);
    pitchSlider_->installEventFilter(this);
    yawSlider_->installEventFilter(this);
    rollSlider_->installEventFilter(this);
    scaleSlider_->installEventFilter(this);
}

void TargetControlsWidget::connectSignals() {
    // Position controls
    connect(posXSlider_, &QSlider::valueChanged, this, &TargetControlsWidget::onPosXSliderChanged);
    connect(posXSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TargetControlsWidget::onPosXSpinBoxChanged);
    connect(posYSlider_, &QSlider::valueChanged, this, &TargetControlsWidget::onPosYSliderChanged);
    connect(posYSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TargetControlsWidget::onPosYSpinBoxChanged);
    connect(posZSlider_, &QSlider::valueChanged, this, &TargetControlsWidget::onPosZSliderChanged);
    connect(posZSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TargetControlsWidget::onPosZSpinBoxChanged);

    // Rotation controls
    connect(pitchSlider_, &QSlider::valueChanged, this, &TargetControlsWidget::onPitchSliderChanged);
    connect(pitchSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TargetControlsWidget::onPitchSpinBoxChanged);
    connect(yawSlider_, &QSlider::valueChanged, this, &TargetControlsWidget::onYawSliderChanged);
    connect(yawSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TargetControlsWidget::onYawSpinBoxChanged);
    connect(rollSlider_, &QSlider::valueChanged, this, &TargetControlsWidget::onRollSliderChanged);
    connect(rollSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TargetControlsWidget::onRollSpinBoxChanged);

    // Scale control
    connect(scaleSlider_, &QSlider::valueChanged, this, &TargetControlsWidget::onScaleSliderChanged);
    connect(scaleSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TargetControlsWidget::onScaleSpinBoxChanged);
}

// Getters
QVector3D TargetControlsWidget::getPosition() const {
    return QVector3D(
        static_cast<float>(posXSpinBox_->value()),
        static_cast<float>(posYSpinBox_->value()),
        static_cast<float>(posZSpinBox_->value())
    );
}

QVector3D TargetControlsWidget::getRotation() const {
    return QVector3D(
        static_cast<float>(pitchSpinBox_->value()),
        static_cast<float>(yawSpinBox_->value()),
        static_cast<float>(rollSpinBox_->value())
    );
}

float TargetControlsWidget::getScale() const {
    return static_cast<float>(scaleSpinBox_->value());
}

// Public slots
void TargetControlsWidget::setPosition(float x, float y, float z) {
    posXSlider_->blockSignals(true);
    posXSpinBox_->blockSignals(true);
    posXSlider_->setValue(static_cast<int>(x * 2));
    posXSpinBox_->setValue(static_cast<double>(x));
    posXSlider_->blockSignals(false);
    posXSpinBox_->blockSignals(false);

    posYSlider_->blockSignals(true);
    posYSpinBox_->blockSignals(true);
    posYSlider_->setValue(static_cast<int>(y * 2));
    posYSpinBox_->setValue(static_cast<double>(y));
    posYSlider_->blockSignals(false);
    posYSpinBox_->blockSignals(false);

    posZSlider_->blockSignals(true);
    posZSpinBox_->blockSignals(true);
    posZSlider_->setValue(static_cast<int>(z * 2));
    posZSpinBox_->setValue(static_cast<double>(z));
    posZSlider_->blockSignals(false);
    posZSpinBox_->blockSignals(false);
}

void TargetControlsWidget::setRotation(float pitch, float yaw, float roll) {
    pitchSlider_->blockSignals(true);
    pitchSpinBox_->blockSignals(true);
    pitchSlider_->setValue(static_cast<int>(pitch * 2));
    pitchSpinBox_->setValue(static_cast<double>(pitch));
    pitchSlider_->blockSignals(false);
    pitchSpinBox_->blockSignals(false);

    yawSlider_->blockSignals(true);
    yawSpinBox_->blockSignals(true);
    yawSlider_->setValue(static_cast<int>(yaw * 2));
    yawSpinBox_->setValue(static_cast<double>(yaw));
    yawSlider_->blockSignals(false);
    yawSpinBox_->blockSignals(false);

    rollSlider_->blockSignals(true);
    rollSpinBox_->blockSignals(true);
    rollSlider_->setValue(static_cast<int>(roll * 2));
    rollSpinBox_->setValue(static_cast<double>(roll));
    rollSlider_->blockSignals(false);
    rollSpinBox_->blockSignals(false);
}

void TargetControlsWidget::setScale(float scale) {
    scaleSlider_->blockSignals(true);
    scaleSpinBox_->blockSignals(true);
    scaleSlider_->setValue(static_cast<int>(scale * 2));
    scaleSpinBox_->setValue(static_cast<double>(scale));
    scaleSlider_->blockSignals(false);
    scaleSpinBox_->blockSignals(false);
}

// Settings persistence
void TargetControlsWidget::readSettings(RSConfig::TargetConfig& config) const {
    config.position = getPosition();
    config.rotation = getRotation();
    config.scale = getScale();
}

void TargetControlsWidget::applySettings(const RSConfig::TargetConfig& config) {
    setPosition(config.position.x(), config.position.y(), config.position.z());
    setRotation(config.rotation.x(), config.rotation.y(), config.rotation.z());
    setScale(config.scale);
}

// Private slots - Position
void TargetControlsWidget::onPosXSliderChanged(int value) {
    double actualValue = value / 2.0;
    posXSpinBox_->blockSignals(true);
    posXSpinBox_->setValue(actualValue);
    posXSpinBox_->blockSignals(false);
    QVector3D pos = getPosition();
    emit positionChanged(pos.x(), pos.y(), pos.z());
}

void TargetControlsWidget::onPosXSpinBoxChanged(double value) {
    posXSlider_->blockSignals(true);
    posXSlider_->setValue(static_cast<int>(value * 2));
    posXSlider_->blockSignals(false);
    QVector3D pos = getPosition();
    emit positionChanged(pos.x(), pos.y(), pos.z());
}

void TargetControlsWidget::onPosYSliderChanged(int value) {
    double actualValue = value / 2.0;
    posYSpinBox_->blockSignals(true);
    posYSpinBox_->setValue(actualValue);
    posYSpinBox_->blockSignals(false);
    QVector3D pos = getPosition();
    emit positionChanged(pos.x(), pos.y(), pos.z());
}

void TargetControlsWidget::onPosYSpinBoxChanged(double value) {
    posYSlider_->blockSignals(true);
    posYSlider_->setValue(static_cast<int>(value * 2));
    posYSlider_->blockSignals(false);
    QVector3D pos = getPosition();
    emit positionChanged(pos.x(), pos.y(), pos.z());
}

void TargetControlsWidget::onPosZSliderChanged(int value) {
    double actualValue = value / 2.0;
    posZSpinBox_->blockSignals(true);
    posZSpinBox_->setValue(actualValue);
    posZSpinBox_->blockSignals(false);
    QVector3D pos = getPosition();
    emit positionChanged(pos.x(), pos.y(), pos.z());
}

void TargetControlsWidget::onPosZSpinBoxChanged(double value) {
    posZSlider_->blockSignals(true);
    posZSlider_->setValue(static_cast<int>(value * 2));
    posZSlider_->blockSignals(false);
    QVector3D pos = getPosition();
    emit positionChanged(pos.x(), pos.y(), pos.z());
}

// Private slots - Rotation
void TargetControlsWidget::onPitchSliderChanged(int value) {
    double actualValue = value / 2.0;
    pitchSpinBox_->blockSignals(true);
    pitchSpinBox_->setValue(actualValue);
    pitchSpinBox_->blockSignals(false);
    QVector3D rot = getRotation();
    emit rotationChanged(rot.x(), rot.y(), rot.z());
}

void TargetControlsWidget::onPitchSpinBoxChanged(double value) {
    pitchSlider_->blockSignals(true);
    pitchSlider_->setValue(static_cast<int>(value * 2));
    pitchSlider_->blockSignals(false);
    QVector3D rot = getRotation();
    emit rotationChanged(rot.x(), rot.y(), rot.z());
}

void TargetControlsWidget::onYawSliderChanged(int value) {
    double actualValue = value / 2.0;
    yawSpinBox_->blockSignals(true);
    yawSpinBox_->setValue(actualValue);
    yawSpinBox_->blockSignals(false);
    QVector3D rot = getRotation();
    emit rotationChanged(rot.x(), rot.y(), rot.z());
}

void TargetControlsWidget::onYawSpinBoxChanged(double value) {
    yawSlider_->blockSignals(true);
    yawSlider_->setValue(static_cast<int>(value * 2));
    yawSlider_->blockSignals(false);
    QVector3D rot = getRotation();
    emit rotationChanged(rot.x(), rot.y(), rot.z());
}

void TargetControlsWidget::onRollSliderChanged(int value) {
    double actualValue = value / 2.0;
    rollSpinBox_->blockSignals(true);
    rollSpinBox_->setValue(actualValue);
    rollSpinBox_->blockSignals(false);
    QVector3D rot = getRotation();
    emit rotationChanged(rot.x(), rot.y(), rot.z());
}

void TargetControlsWidget::onRollSpinBoxChanged(double value) {
    rollSlider_->blockSignals(true);
    rollSlider_->setValue(static_cast<int>(value * 2));
    rollSlider_->blockSignals(false);
    QVector3D rot = getRotation();
    emit rotationChanged(rot.x(), rot.y(), rot.z());
}

// Private slots - Scale
void TargetControlsWidget::onScaleSliderChanged(int value) {
    double actualValue = value / 2.0;
    scaleSpinBox_->blockSignals(true);
    scaleSpinBox_->setValue(actualValue);
    scaleSpinBox_->blockSignals(false);
    emit scaleChanged(static_cast<float>(actualValue));
}

void TargetControlsWidget::onScaleSpinBoxChanged(double value) {
    scaleSlider_->blockSignals(true);
    scaleSlider_->setValue(static_cast<int>(value * 2));
    scaleSlider_->blockSignals(false);
    emit scaleChanged(static_cast<float>(value));
}

// Event filter for double-click reset
bool TargetControlsWidget::eventFilter(QObject* obj, QEvent* event) {
    using namespace RS::Constants::Defaults;

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // Position (sliders use 2x for 0.5 increments)
            if (obj == posXSlider_) {
                posXSlider_->setValue(static_cast<int>(kTargetPositionX * 2));
                return true;
            }
            if (obj == posYSlider_) {
                posYSlider_->setValue(static_cast<int>(kTargetPositionY * 2));
                return true;
            }
            if (obj == posZSlider_) {
                posZSlider_->setValue(static_cast<int>(kTargetPositionZ * 2));
                return true;
            }
            // Rotation (sliders use 2x for 0.5 degree increments)
            if (obj == pitchSlider_) {
                pitchSlider_->setValue(static_cast<int>(kTargetPitch * 2));
                return true;
            }
            if (obj == yawSlider_) {
                yawSlider_->setValue(static_cast<int>(kTargetYaw * 2));
                return true;
            }
            if (obj == rollSlider_) {
                rollSlider_->setValue(static_cast<int>(kTargetRoll * 2));
                return true;
            }
            // Scale (slider uses 2x for 0.5 increments)
            if (obj == scaleSlider_) {
                scaleSlider_->setValue(static_cast<int>(kTargetScale * 2));
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
