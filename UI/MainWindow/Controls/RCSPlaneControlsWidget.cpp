// RCSPlaneControlsWidget.cpp - Self-contained RCS plane control panel widget

#include "RCSPlaneControlsWidget.h"
#include "SceneConfig.h"
#include "RCSSampler.h"  // For CutType enum
#include "Constants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMouseEvent>
#include <cmath>

namespace {
    // Find the closest index in the thickness scale for a given thickness value
    int findClosestThicknessIndex(float thickness) {
        int closest = 0;
        float minDiff = std::abs(RS::Constants::kThicknessScale[0] - thickness);
        for (int i = 1; i < RS::Constants::kThicknessScaleSize; ++i) {
            float diff = std::abs(RS::Constants::kThicknessScale[i] - thickness);
            if (diff < minDiff) {
                minDiff = diff;
                closest = i;
            }
        }
        return closest;
    }
}

RCSPlaneControlsWidget::RCSPlaneControlsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
}

void RCSPlaneControlsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QGroupBox* controlsGroup = new QGroupBox("2D RCS Plane", this);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(6, 0, 6, 0);
    mainLayout->addWidget(controlsGroup);

    // Cut Type selector
    QLabel* cutTypeLabel = new QLabel("Cut Type", controlsGroup);
    controlsLayout->addWidget(cutTypeLabel);

    cutTypeComboBox_ = new QComboBox(controlsGroup);
    cutTypeComboBox_->addItem("Azimuth Cut");      // Index 0
    cutTypeComboBox_->addItem("Elevation Cut");    // Index 1
    cutTypeComboBox_->setCurrentIndex(0);
    controlsLayout->addWidget(cutTypeComboBox_);

    // Plane Offset control
    QLabel* offsetLabel = new QLabel("Plane Offset (\302\260)", controlsGroup);  // ° character
    controlsLayout->addWidget(offsetLabel);

    QHBoxLayout* offsetLayout = new QHBoxLayout();
    offsetLayout->setContentsMargins(0, 0, 0, 0);
    planeOffsetSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    planeOffsetSlider_->setRange(-90, 90);
    planeOffsetSlider_->setValue(0);

    planeOffsetSpinBox_ = new QSpinBox(controlsGroup);
    planeOffsetSpinBox_->setRange(-90, 90);
    planeOffsetSpinBox_->setValue(0);
    planeOffsetSpinBox_->setMinimumWidth(60);

    offsetLayout->addWidget(planeOffsetSlider_);
    offsetLayout->addWidget(planeOffsetSpinBox_);
    controlsLayout->addLayout(offsetLayout);

    // Slice Thickness control (non-linear scale: 0.1° steps 0.5-3°, 1° steps 3-40°)
    QLabel* thicknessLabel = new QLabel("Slice Thickness (\302\261\302\260)", controlsGroup);  // ±° characters
    controlsLayout->addWidget(thicknessLabel);

    QHBoxLayout* thicknessLayout = new QHBoxLayout();
    thicknessLayout->setContentsMargins(0, 0, 0, 0);
    sliceThicknessSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    sliceThicknessSlider_->setRange(0, RS::Constants::kThicknessScaleSize - 1);
    sliceThicknessSlider_->setValue(RS::Constants::kDefaultThicknessIndex);

    sliceThicknessSpinBox_ = new QDoubleSpinBox(controlsGroup);
    sliceThicknessSpinBox_->setRange(0.5, 40.0);
    sliceThicknessSpinBox_->setDecimals(1);
    sliceThicknessSpinBox_->setSingleStep(0.1);
    sliceThicknessSpinBox_->setValue(RS::Constants::kThicknessScale[RS::Constants::kDefaultThicknessIndex]);
    sliceThicknessSpinBox_->setMinimumWidth(60);

    thicknessLayout->addWidget(sliceThicknessSlider_);
    thicknessLayout->addWidget(sliceThicknessSpinBox_);
    controlsLayout->addLayout(thicknessLayout);

    // Show Fill checkbox
    showFillCheckBox_ = new QCheckBox("Show Plane Fill", controlsGroup);
    showFillCheckBox_->setChecked(true);
    controlsLayout->addWidget(showFillCheckBox_);

    // Install event filters for double-click reset
    planeOffsetSlider_->installEventFilter(this);
    sliceThicknessSlider_->installEventFilter(this);
}

void RCSPlaneControlsWidget::connectSignals() {
    connect(cutTypeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RCSPlaneControlsWidget::onCutTypeChanged);
    connect(planeOffsetSlider_, &QSlider::valueChanged,
            this, &RCSPlaneControlsWidget::onPlaneOffsetSliderChanged);
    connect(planeOffsetSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RCSPlaneControlsWidget::onPlaneOffsetSpinBoxChanged);
    connect(sliceThicknessSlider_, &QSlider::valueChanged,
            this, &RCSPlaneControlsWidget::onSliceThicknessSliderChanged);
    connect(showFillCheckBox_, &QCheckBox::toggled,
            this, &RCSPlaneControlsWidget::onShowFillChanged);
}

// Getters
CutType RCSPlaneControlsWidget::getCutType() const {
    return static_cast<CutType>(cutTypeComboBox_->currentIndex());
}

int RCSPlaneControlsWidget::getPlaneOffset() const {
    return planeOffsetSpinBox_->value();
}

float RCSPlaneControlsWidget::getSliceThickness() const {
    int index = sliceThicknessSlider_->value();
    return RS::Constants::kThicknessScale[index];
}

bool RCSPlaneControlsWidget::isShowFillEnabled() const {
    return showFillCheckBox_->isChecked();
}

// Public slots
void RCSPlaneControlsWidget::setCutType(CutType type) {
    cutTypeComboBox_->blockSignals(true);
    cutTypeComboBox_->setCurrentIndex(static_cast<int>(type));
    cutTypeComboBox_->blockSignals(false);
}

void RCSPlaneControlsWidget::setPlaneOffset(int degrees) {
    planeOffsetSlider_->blockSignals(true);
    planeOffsetSpinBox_->blockSignals(true);
    planeOffsetSlider_->setValue(degrees);
    planeOffsetSpinBox_->setValue(degrees);
    planeOffsetSlider_->blockSignals(false);
    planeOffsetSpinBox_->blockSignals(false);
}

void RCSPlaneControlsWidget::setSliceThickness(float degrees) {
    int index = findClosestThicknessIndex(degrees);
    float actualThickness = RS::Constants::kThicknessScale[index];

    sliceThicknessSlider_->blockSignals(true);
    sliceThicknessSpinBox_->blockSignals(true);
    sliceThicknessSlider_->setValue(index);
    sliceThicknessSpinBox_->setValue(static_cast<double>(actualThickness));
    sliceThicknessSlider_->blockSignals(false);
    sliceThicknessSpinBox_->blockSignals(false);
}

void RCSPlaneControlsWidget::setShowFill(bool show) {
    showFillCheckBox_->blockSignals(true);
    showFillCheckBox_->setChecked(show);
    showFillCheckBox_->blockSignals(false);
}

// Settings persistence
void RCSPlaneControlsWidget::readSettings(RSConfig::SceneConfig& config) const {
    config.rcsCutType = static_cast<int>(getCutType());
    config.rcsPlaneOffset = static_cast<float>(getPlaneOffset());
    config.rcsSliceThickness = getSliceThickness();
    config.rcsPlaneShowFill = isShowFillEnabled();
}

void RCSPlaneControlsWidget::applySettings(const RSConfig::SceneConfig& config) {
    setCutType(static_cast<CutType>(config.rcsCutType));
    setPlaneOffset(static_cast<int>(config.rcsPlaneOffset));
    setSliceThickness(config.rcsSliceThickness);
    setShowFill(config.rcsPlaneShowFill);
}

// Private slots
void RCSPlaneControlsWidget::onCutTypeChanged(int index) {
    emit cutTypeChanged(static_cast<CutType>(index));
}

void RCSPlaneControlsWidget::onPlaneOffsetSliderChanged(int value) {
    planeOffsetSpinBox_->blockSignals(true);
    planeOffsetSpinBox_->setValue(value);
    planeOffsetSpinBox_->blockSignals(false);
    emit planeOffsetChanged(static_cast<float>(value));
}

void RCSPlaneControlsWidget::onPlaneOffsetSpinBoxChanged(int value) {
    planeOffsetSlider_->blockSignals(true);
    planeOffsetSlider_->setValue(value);
    planeOffsetSlider_->blockSignals(false);
    emit planeOffsetChanged(static_cast<float>(value));
}

void RCSPlaneControlsWidget::onSliceThicknessSliderChanged(int index) {
    float thickness = RS::Constants::kThicknessScale[index];
    sliceThicknessSpinBox_->blockSignals(true);
    sliceThicknessSpinBox_->setValue(static_cast<double>(thickness));
    sliceThicknessSpinBox_->blockSignals(false);
    emit sliceThicknessChanged(thickness);
}

void RCSPlaneControlsWidget::onShowFillChanged(bool checked) {
    emit showFillChanged(checked);
}

// Event filter for double-click reset
bool RCSPlaneControlsWidget::eventFilter(QObject* obj, QEvent* event) {
    using namespace RS::Constants::Defaults;

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (obj == planeOffsetSlider_) {
                planeOffsetSlider_->setValue(static_cast<int>(kRCSPlaneOffset));
                return true;
            }
            if (obj == sliceThicknessSlider_) {
                sliceThicknessSlider_->setValue(RS::Constants::kDefaultThicknessIndex);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
