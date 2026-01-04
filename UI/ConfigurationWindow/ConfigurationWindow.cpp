// ConfigurationWindow.cpp - Floating configuration window implementation

#include "ConfigurationWindow.h"
#include <QFormLayout>

ConfigurationWindow::ConfigurationWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)  // Qt::Window makes it a separate top-level window
{
    setWindowTitle("Configuration");
    setupUI();
}

void ConfigurationWindow::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Add all group boxes
    mainLayout->addWidget(createProfileGroup());
    mainLayout->addWidget(createDisplayGroup());
    mainLayout->addWidget(createBeamGroup());
    mainLayout->addWidget(createVisualizationGroup());
    mainLayout->addWidget(createTargetGroup());
    mainLayout->addStretch();

    // Set a reasonable default size
    setMinimumWidth(280);
    resize(300, 450);
}

QGroupBox* ConfigurationWindow::createProfileGroup()
{
    QGroupBox* group = new QGroupBox("Configuration Profiles", this);
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(6);

    // Profile selector
    QHBoxLayout* profileLayout = new QHBoxLayout();
    profileLayout->addWidget(new QLabel("Profile:", group));
    profileComboBox_ = new QComboBox(group);
    profileComboBox_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    profileLayout->addWidget(profileComboBox_);
    layout->addLayout(profileLayout);

    // Buttons row
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    saveButton_ = new QPushButton("Save", group);
    saveAsButton_ = new QPushButton("Save As...", group);
    deleteButton_ = new QPushButton("Delete", group);
    resetButton_ = new QPushButton("Reset to Defaults", group);

    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(saveAsButton_);
    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addWidget(resetButton_);
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(profileComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigurationWindow::profileSelected);
    connect(saveButton_, &QPushButton::clicked, this, &ConfigurationWindow::saveRequested);
    connect(saveAsButton_, &QPushButton::clicked, this, &ConfigurationWindow::saveAsRequested);
    connect(deleteButton_, &QPushButton::clicked, this, &ConfigurationWindow::deleteRequested);
    connect(resetButton_, &QPushButton::clicked, this, &ConfigurationWindow::resetRequested);

    return group;
}

QGroupBox* ConfigurationWindow::createDisplayGroup()
{
    QGroupBox* group = new QGroupBox("Display Options", this);
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(4);

    showAxesCheckBox_ = new QCheckBox("Show Axes", group);
    showAxesCheckBox_->setChecked(true);
    layout->addWidget(showAxesCheckBox_);

    showSphereCheckBox_ = new QCheckBox("Show Sphere", group);
    showSphereCheckBox_->setChecked(true);
    layout->addWidget(showSphereCheckBox_);

    showGridCheckBox_ = new QCheckBox("Show Grid Lines", group);
    showGridCheckBox_->setChecked(true);
    layout->addWidget(showGridCheckBox_);

    // Connect signals
    connect(showAxesCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::axesVisibilityChanged);
    connect(showSphereCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::sphereVisibilityChanged);
    connect(showGridCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::gridVisibilityChanged);

    return group;
}

QGroupBox* ConfigurationWindow::createBeamGroup()
{
    QGroupBox* group = new QGroupBox("Beam Settings", this);
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(4);

    showBeamCheckBox_ = new QCheckBox("Show Beam", group);
    showBeamCheckBox_->setChecked(true);
    layout->addWidget(showBeamCheckBox_);

    showShadowCheckBox_ = new QCheckBox("Show Shadow", group);
    showShadowCheckBox_->setChecked(true);
    layout->addWidget(showShadowCheckBox_);

    // Beam type selector
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Type:", group));
    beamTypeComboBox_ = new QComboBox(group);
    beamTypeComboBox_->addItem("Conical", static_cast<int>(BeamType::Conical));
    beamTypeComboBox_->addItem("Sinc (Airy)", static_cast<int>(BeamType::Sinc));
    beamTypeComboBox_->addItem("Phased Array", static_cast<int>(BeamType::Phased));
    beamTypeComboBox_->setCurrentIndex(1);  // Default to Sinc
    typeLayout->addWidget(beamTypeComboBox_);
    layout->addLayout(typeLayout);

    // Connect signals
    connect(showBeamCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::beamVisibilityChanged);
    connect(showShadowCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::beamShadowChanged);
    connect(beamTypeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                BeamType type = static_cast<BeamType>(beamTypeComboBox_->itemData(index).toInt());
                emit beamTypeChanged(type);
            });

    return group;
}

QGroupBox* ConfigurationWindow::createVisualizationGroup()
{
    QGroupBox* group = new QGroupBox("Visualization", this);
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(4);

    showReflectionLobesCheckBox_ = new QCheckBox("Show Reflection Lobes", group);
    showReflectionLobesCheckBox_->setChecked(false);
    layout->addWidget(showReflectionLobesCheckBox_);

    showHeatMapCheckBox_ = new QCheckBox("Show RCS Heat Map", group);
    showHeatMapCheckBox_->setChecked(false);
    layout->addWidget(showHeatMapCheckBox_);

    debugRayCheckBox_ = new QCheckBox("Debug Ray", group);
    debugRayCheckBox_->setChecked(false);
    debugRayCheckBox_->setToolTip("Show single debug ray with reflection visualization");
    layout->addWidget(debugRayCheckBox_);

    // Ray count slider (always available, controls normal RCS compute density)
    QHBoxLayout* rayCountLayout = new QHBoxLayout();
    rayCountLayout->addWidget(new QLabel("Ray Count:", group));
    rayCountSlider_ = new QSlider(Qt::Horizontal, group);
    rayCountSlider_->setRange(100, 10000);
    rayCountSlider_->setValue(10000);
    rayCountSlider_->setSingleStep(100);
    rayCountSlider_->setPageStep(1000);
    rayCountSlider_->setToolTip("Number of rays for RCS computation (100-10000)");
    rayCountLabel_ = new QLabel("10000", group);
    rayCountLabel_->setMinimumWidth(40);
    rayCountLayout->addWidget(rayCountSlider_);
    rayCountLayout->addWidget(rayCountLabel_);
    layout->addLayout(rayCountLayout);

    // Connect signals
    connect(showReflectionLobesCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::reflectionLobesChanged);
    connect(showHeatMapCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::heatMapChanged);
    connect(debugRayCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::debugRayToggled);
    connect(rayCountSlider_, &QSlider::valueChanged, this, [this](int value) {
        rayCountLabel_->setText(QString::number(value));
        emit rayCountChanged(value);
    });

    return group;
}

QGroupBox* ConfigurationWindow::createTargetGroup()
{
    QGroupBox* group = new QGroupBox("Target", this);
    QVBoxLayout* layout = new QVBoxLayout(group);
    layout->setSpacing(4);

    showTargetCheckBox_ = new QCheckBox("Show Target", group);
    showTargetCheckBox_->setChecked(true);
    layout->addWidget(showTargetCheckBox_);

    // Target type selector
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Type:", group));
    targetTypeComboBox_ = new QComboBox(group);
    targetTypeComboBox_->addItem("Cube", static_cast<int>(WireframeType::Cube));
    targetTypeComboBox_->addItem("Cylinder", static_cast<int>(WireframeType::Cylinder));
    targetTypeComboBox_->addItem("Aircraft", static_cast<int>(WireframeType::Aircraft));
    targetTypeComboBox_->addItem("Sphere", static_cast<int>(WireframeType::Sphere));
    typeLayout->addWidget(targetTypeComboBox_);
    layout->addLayout(typeLayout);

    // Connect signals
    connect(showTargetCheckBox_, &QCheckBox::toggled, this, &ConfigurationWindow::targetVisibilityChanged);
    connect(targetTypeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                WireframeType type = static_cast<WireframeType>(targetTypeComboBox_->itemData(index).toInt());
                emit targetTypeChanged(type);
            });

    return group;
}

void ConfigurationWindow::setProfiles(const QStringList& profiles)
{
    profileComboBox_->blockSignals(true);
    profileComboBox_->clear();
    profileComboBox_->addItems(profiles);
    profileComboBox_->blockSignals(false);
}

void ConfigurationWindow::setCurrentProfile(int index)
{
    profileComboBox_->blockSignals(true);
    profileComboBox_->setCurrentIndex(index);
    profileComboBox_->blockSignals(false);
}

int ConfigurationWindow::currentProfileIndex() const
{
    return profileComboBox_->currentIndex();
}

void ConfigurationWindow::syncStateFromScene(bool axesVisible, bool sphereVisible, bool gridVisible,
                                              bool reflectionLobesVisible, bool heatMapVisible,
                                              bool debugRayEnabled, int rayCount,
                                              bool beamVisible, bool shadowVisible, BeamType beamType,
                                              bool targetVisible, WireframeType targetType)
{
    // Block signals during sync to prevent feedback loops
    showAxesCheckBox_->blockSignals(true);
    showSphereCheckBox_->blockSignals(true);
    showGridCheckBox_->blockSignals(true);
    showReflectionLobesCheckBox_->blockSignals(true);
    showHeatMapCheckBox_->blockSignals(true);
    debugRayCheckBox_->blockSignals(true);
    rayCountSlider_->blockSignals(true);
    showBeamCheckBox_->blockSignals(true);
    showShadowCheckBox_->blockSignals(true);
    beamTypeComboBox_->blockSignals(true);
    showTargetCheckBox_->blockSignals(true);
    targetTypeComboBox_->blockSignals(true);

    // Set checkbox states
    showAxesCheckBox_->setChecked(axesVisible);
    showSphereCheckBox_->setChecked(sphereVisible);
    showGridCheckBox_->setChecked(gridVisible);
    showReflectionLobesCheckBox_->setChecked(reflectionLobesVisible);
    showHeatMapCheckBox_->setChecked(heatMapVisible);
    debugRayCheckBox_->setChecked(debugRayEnabled);
    rayCountSlider_->setValue(rayCount);
    rayCountLabel_->setText(QString::number(rayCount));
    showBeamCheckBox_->setChecked(beamVisible);
    showShadowCheckBox_->setChecked(shadowVisible);
    showTargetCheckBox_->setChecked(targetVisible);

    // Set combo box selections
    int beamIndex = beamTypeComboBox_->findData(static_cast<int>(beamType));
    if (beamIndex >= 0) beamTypeComboBox_->setCurrentIndex(beamIndex);

    int targetIndex = targetTypeComboBox_->findData(static_cast<int>(targetType));
    if (targetIndex >= 0) targetTypeComboBox_->setCurrentIndex(targetIndex);

    // Unblock signals
    showAxesCheckBox_->blockSignals(false);
    showSphereCheckBox_->blockSignals(false);
    showGridCheckBox_->blockSignals(false);
    showReflectionLobesCheckBox_->blockSignals(false);
    showHeatMapCheckBox_->blockSignals(false);
    debugRayCheckBox_->blockSignals(false);
    rayCountSlider_->blockSignals(false);
    showBeamCheckBox_->blockSignals(false);
    showShadowCheckBox_->blockSignals(false);
    beamTypeComboBox_->blockSignals(false);
    showTargetCheckBox_->blockSignals(false);
    targetTypeComboBox_->blockSignals(false);
}
