// ---- RadarSim.cpp ----

#include "RadarSim.h"
#include "SphereWidget.h"
#include "AppSettings.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTabWidget>
#include <QFrame>
#include <QSplitter>
#include <QCloseEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>

// Constructor
RadarSim::RadarSim(QWidget* parent)
    : QMainWindow(parent),
    radarSceneView_(nullptr),
    radiusSlider_(nullptr),
    thetaSlider_(nullptr),
    phiSlider_(nullptr),
    radiusSpinBox_(nullptr),
    thetaSpinBox_(nullptr),
    phiSpinBox_(nullptr),
    tabWidget_(nullptr),
    configTabWidget_(nullptr),
    radarSceneTabWidget_(nullptr),
    physicsTabWidget_(nullptr),
    targetPosXSlider_(nullptr),
    targetPosYSlider_(nullptr),
    targetPosZSlider_(nullptr),
    targetPitchSlider_(nullptr),
    targetYawSlider_(nullptr),
    targetRollSlider_(nullptr),
    targetScaleSlider_(nullptr),
    targetPosXSpinBox_(nullptr),
    targetPosYSpinBox_(nullptr),
    targetPosZSpinBox_(nullptr),
    targetPitchSpinBox_(nullptr),
    targetYawSpinBox_(nullptr),
    targetRollSpinBox_(nullptr),
    targetScaleSpinBox_(nullptr),
    appSettings_(new RSConfig::AppSettings(this)),
    profileComboBox_(nullptr)
{
    setupUI();
    setupTabs();
    connectSignals();

    // Try to restore last session settings
    if (appSettings_->restoreLastSession()) {
        // Apply restored settings after a brief delay to ensure scene is ready
        QTimer::singleShot(100, this, &RadarSim::applySettingsToScene);
    }

    // Start with the radar scene tab selected
    tabWidget_->setCurrentIndex(1);
}

// Destructor
RadarSim::~RadarSim() {
    // Any cleanup needed for tab widgets
}

void RadarSim::setupUI() {
    // Create central widget for the main window
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create main layout
    auto* mainLayout = new QVBoxLayout(centralWidget);

    // Create tab widget
    tabWidget_ = new QTabWidget(centralWidget);
    mainLayout->addWidget(tabWidget_);

    // Set the main window title
    setWindowTitle("Radar Simulation System");

    // Set minimum window size - increased height to accommodate more controls
    setMinimumSize(1024, 1100);
}

void RadarSim::setupTabs() {
    qDebug() << "Starting setupTabs()";

    // Create tab widgets with proper parent
    configTabWidget_ = new QWidget(tabWidget_);
    radarSceneTabWidget_ = new QWidget(tabWidget_);
    physicsTabWidget_ = new QWidget(tabWidget_);

    // Add tabs
    tabWidget_->addTab(configTabWidget_, "Configuration");
    tabWidget_->addTab(radarSceneTabWidget_, "Radar Scene");
    tabWidget_->addTab(physicsTabWidget_, "Physics Analysis");

    // Setup each tab independently
    setupConfigurationTab();
    setupRadarSceneTab();
    setupPhysicsAnalysisTab();

    qDebug() << "Exiting setupTabs()";
}

void RadarSim::setupConfigurationTab() {
    QVBoxLayout* configLayout = new QVBoxLayout(configTabWidget_);

    // Create a group box for profile management (at top)
    QGroupBox* profileGroup = new QGroupBox("Configuration Profiles", configTabWidget_);
    QVBoxLayout* profileLayout = new QVBoxLayout(profileGroup);

    // Profile selector row
    QHBoxLayout* profileSelectorLayout = new QHBoxLayout();
    QLabel* profileLabel = new QLabel("Profile:", profileGroup);
    profileComboBox_ = new QComboBox(profileGroup);
    profileComboBox_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    profileSelectorLayout->addWidget(profileLabel);
    profileSelectorLayout->addWidget(profileComboBox_);
    profileLayout->addLayout(profileSelectorLayout);

    // Profile buttons row
    QHBoxLayout* profileButtonsLayout = new QHBoxLayout();
    QPushButton* saveButton = new QPushButton("Save", profileGroup);
    QPushButton* saveAsButton = new QPushButton("Save As...", profileGroup);
    QPushButton* deleteButton = new QPushButton("Delete", profileGroup);
    QPushButton* resetButton = new QPushButton("Reset to Defaults", profileGroup);

    saveButton->setToolTip("Save current settings to the selected profile");
    saveAsButton->setToolTip("Save current settings to a new profile");
    deleteButton->setToolTip("Delete the selected profile");
    resetButton->setToolTip("Reset all settings to their default values");

    profileButtonsLayout->addWidget(saveButton);
    profileButtonsLayout->addWidget(saveAsButton);
    profileButtonsLayout->addWidget(deleteButton);
    profileButtonsLayout->addWidget(resetButton);
    profileLayout->addLayout(profileButtonsLayout);

    configLayout->addWidget(profileGroup);

    // Create a group box for simulation settings
    QGroupBox* simSettingsGroup = new QGroupBox("Simulation Settings", configTabWidget_);
    QFormLayout* simSettingsLayout = new QFormLayout(simSettingsGroup);

    // Add some placeholder settings
    QSpinBox* maxTargetsSpinBox = new QSpinBox(simSettingsGroup);
    maxTargetsSpinBox->setRange(1, 50);
    maxTargetsSpinBox->setValue(5);
    simSettingsLayout->addRow("Maximum Targets:", maxTargetsSpinBox);

    QDoubleSpinBox* physicsStepSpinBox = new QDoubleSpinBox(simSettingsGroup);
    physicsStepSpinBox->setRange(0.001, 0.1);
    physicsStepSpinBox->setValue(0.01);
    physicsStepSpinBox->setSingleStep(0.001);
    simSettingsLayout->addRow("Physics Time Step (s):", physicsStepSpinBox);

    QCheckBox* enableAdvancedPhysicsCheckBox = new QCheckBox(simSettingsGroup);
    enableAdvancedPhysicsCheckBox->setChecked(false);
    simSettingsLayout->addRow("Enable Advanced Physics:", enableAdvancedPhysicsCheckBox);

    configLayout->addWidget(simSettingsGroup);

    // Create a group box for beam settings
    QGroupBox* beamSettingsGroup = new QGroupBox("Beam Settings", configTabWidget_);
    QFormLayout* beamSettingsLayout = new QFormLayout(beamSettingsGroup);

    QComboBox* beamTypeComboBox = new QComboBox(beamSettingsGroup);
    beamTypeComboBox->addItem("Conical");
    beamTypeComboBox->addItem("Phased Array");
    beamSettingsLayout->addRow("Default Beam Type:", beamTypeComboBox);

    QDoubleSpinBox* beamWidthSpinBox = new QDoubleSpinBox(beamSettingsGroup);
    beamWidthSpinBox->setRange(1.0, 45.0);
    beamWidthSpinBox->setValue(15.0);
    beamSettingsLayout->addRow("Default Beam Width (°):", beamWidthSpinBox);

    configLayout->addWidget(beamSettingsGroup);

    // Add a group box for component architecture
    QGroupBox* architectureGroup = new QGroupBox("Architecture", configTabWidget_);
    QVBoxLayout* architectureLayout = new QVBoxLayout(architectureGroup);

    QCheckBox* useComponentsCheckbox = new QCheckBox("Use Component Architecture", architectureGroup);
    useComponentsCheckbox->setChecked(true);
    useComponentsCheckbox->setToolTip("Switch between old and new rendering architecture. Checked is new.");
    architectureLayout->addWidget(useComponentsCheckbox);

    connect(useComponentsCheckbox, &QCheckBox::toggled, [this](bool checked) {
        if (radarSceneView_) {
            radarSceneView_->enableComponentRendering(checked);
        }
    });

    configLayout->addWidget(architectureGroup);
    configLayout->addStretch();

    // Connect profile management signals
    connect(profileComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RadarSim::onProfileSelected);
    connect(saveButton, &QPushButton::clicked, this, &RadarSim::onSaveProfile);
    connect(saveAsButton, &QPushButton::clicked, this, &RadarSim::onSaveProfileAs);
    connect(deleteButton, &QPushButton::clicked, this, &RadarSim::onDeleteProfile);
    connect(resetButton, &QPushButton::clicked, this, &RadarSim::onResetToDefaults);

    // Connect settings signals
    connect(appSettings_, &RSConfig::AppSettings::profilesChanged,
            this, &RadarSim::onProfilesChanged);

    // Populate profile list
    refreshProfileList();
}

void RadarSim::setupRadarSceneTab() {
    QVBoxLayout* radarSceneLayout = new QVBoxLayout(radarSceneTabWidget_);
    radarSceneLayout->setContentsMargins(4, 4, 4, 4);
    radarSceneLayout->setSpacing(0);

    // Create splitter
    radarSplitter_ = new QSplitter(Qt::Vertical, radarSceneTabWidget_);
    radarSceneLayout->addWidget(radarSplitter_);

    // Create frames first without parent
    QFrame* sphereFrame = new QFrame();
    sphereFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    sphereFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QFrame* controlsFrame = new QFrame();
    controlsFrame->setFrameStyle(QFrame::StyledPanel);
    controlsFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Setup layouts for frames
    QVBoxLayout* sphereFrameLayout = new QVBoxLayout(sphereFrame);
    sphereFrameLayout->setContentsMargins(1, 1, 1, 1);

    QVBoxLayout* controlsFrameLayout = new QVBoxLayout(controlsFrame);
    controlsFrameLayout->setContentsMargins(4, 4, 4, 4);
    controlsFrameLayout->setSpacing(0);

    // Create RadarSceneWidget
    radarSceneView_ = new RadarSceneWidget(sphereFrame);
    radarSceneView_->setMinimumSize(800, 400);
    sphereFrameLayout->addWidget(radarSceneView_);

    // Setup control groups
    setupRadarControls(controlsFrame, controlsFrameLayout);
    setupTargetControls(controlsFrame, controlsFrameLayout);

    // Add frames to splitter AFTER all setup is done
    radarSplitter_->addWidget(sphereFrame);
    radarSplitter_->addWidget(controlsFrame);

    // Adjust stretch factors to give equal visual weight
    radarSplitter_->setStretchFactor(0, 1);  // RadarScene gets 1 part
    radarSplitter_->setStretchFactor(0, 1);  // Controls get 1 part (equal space)
}

void RadarSim::setupRadarControls(QFrame* controlsFrame, QVBoxLayout* controlsFrameLayout) {
    QGroupBox* controlsGroup = new QGroupBox("Radar Controls", controlsFrame);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(6, 0, 6, 0);
    controlsFrameLayout->addWidget(controlsGroup);

    // Radius controls
    QLabel* radiusLabel = new QLabel("Sphere Radius", controlsGroup);
    controlsLayout->addWidget(radiusLabel);

    QHBoxLayout* radiusLayout = new QHBoxLayout();
    radiusSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    radiusSlider_->setRange(50, 300);
    radiusSlider_->setValue(100);

    radiusSpinBox_ = new QSpinBox(controlsGroup);
    radiusSpinBox_->setRange(50, 300);
    radiusSpinBox_->setValue(100);
    radiusSpinBox_->setMinimumWidth(60);  // Ensure consistent spinbox width

    radiusLayout->addWidget(radiusSlider_);
    radiusLayout->addWidget(radiusSpinBox_);
    controlsLayout->addLayout(radiusLayout);

    // Azimuth controls
    QLabel* thetaLabel = new QLabel("Radar Azimuth (θ)", controlsGroup);
    controlsLayout->addWidget(thetaLabel);

    QHBoxLayout* thetaLayout = new QHBoxLayout();
    thetaSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    thetaSlider_->setRange(0, 359);
    int initialAzimuth = 45;
    thetaSlider_->setValue(359 - initialAzimuth);

    thetaSpinBox_ = new QSpinBox(controlsGroup);
    thetaSpinBox_->setRange(0, 359);
    thetaSpinBox_->setValue(initialAzimuth);
    thetaSpinBox_->setMinimumWidth(60);  // Ensure consistent spinbox width

    thetaLayout->addWidget(thetaSlider_);
    thetaLayout->addWidget(thetaSpinBox_);
    controlsLayout->addLayout(thetaLayout);

    // Elevation controls
    QLabel* phiLabel = new QLabel("Radar Elevation (φ)", controlsGroup);
    controlsLayout->addWidget(phiLabel);

    QHBoxLayout* phiLayout = new QHBoxLayout();
    phiSlider_ = new QSlider(Qt::Horizontal, controlsGroup);
    phiSlider_->setRange(-90, 90);
    phiSlider_->setValue(45);

    phiSpinBox_ = new QSpinBox(controlsGroup);
    phiSpinBox_->setRange(-90, 90);
    phiSpinBox_->setValue(45);
    phiSpinBox_->setMinimumWidth(60);  // Ensure consistent spinbox width

    phiLayout->addWidget(phiSlider_);
    phiLayout->addWidget(phiSpinBox_);
    controlsLayout->addLayout(phiLayout);
}

void RadarSim::setupTargetControls(QFrame* controlsFrame, QVBoxLayout* controlsFrameLayout) {
    QGroupBox* targetControlsGroup = new QGroupBox("Target Controls", controlsFrame);
    QVBoxLayout* targetControlsLayout = new QVBoxLayout(targetControlsGroup);
    targetControlsLayout->setSpacing(0);  // Match Radar Controls spacing exactly
    targetControlsLayout->setContentsMargins(6, 0, 6, 0);
    controlsFrameLayout->addWidget(targetControlsGroup);

    // Target Position X
    QLabel* targetPosXLabel = new QLabel("Position X", targetControlsGroup);
    targetControlsLayout->addWidget(targetPosXLabel);

    QHBoxLayout* targetPosXLayout = new QHBoxLayout();
    targetPosXSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPosXSlider_->setRange(-100, 100);
    targetPosXSlider_->setValue(0);

    targetPosXSpinBox_ = new QSpinBox(targetControlsGroup);
    targetPosXSpinBox_->setRange(-100, 100);
    targetPosXSpinBox_->setValue(0);
    targetPosXSpinBox_->setMinimumWidth(60);  // Match Radar Controls spinbox width

    targetPosXLayout->addWidget(targetPosXSlider_);
    targetPosXLayout->addWidget(targetPosXSpinBox_);
    targetControlsLayout->addLayout(targetPosXLayout);

    // Target Position Y
    QLabel* targetPosYLabel = new QLabel("Position Y", targetControlsGroup);
    targetControlsLayout->addWidget(targetPosYLabel);

    QHBoxLayout* targetPosYLayout = new QHBoxLayout();
    targetPosYSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPosYSlider_->setRange(-100, 100);
    targetPosYSlider_->setValue(0);

    targetPosYSpinBox_ = new QSpinBox(targetControlsGroup);
    targetPosYSpinBox_->setRange(-100, 100);
    targetPosYSpinBox_->setValue(0);
    targetPosYSpinBox_->setMinimumWidth(60);  // Match Radar Controls spinbox width

    targetPosYLayout->addWidget(targetPosYSlider_);
    targetPosYLayout->addWidget(targetPosYSpinBox_);
    targetControlsLayout->addLayout(targetPosYLayout);

    // Target Position Z
    QLabel* targetPosZLabel = new QLabel("Position Z", targetControlsGroup);
    targetControlsLayout->addWidget(targetPosZLabel);

    QHBoxLayout* targetPosZLayout = new QHBoxLayout();
    targetPosZSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPosZSlider_->setRange(-100, 100);
    targetPosZSlider_->setValue(0);

    targetPosZSpinBox_ = new QSpinBox(targetControlsGroup);
    targetPosZSpinBox_->setRange(-100, 100);
    targetPosZSpinBox_->setValue(0);
    targetPosZSpinBox_->setMinimumWidth(60);  // Match Radar Controls spinbox width

    targetPosZLayout->addWidget(targetPosZSlider_);
    targetPosZLayout->addWidget(targetPosZSpinBox_);
    targetControlsLayout->addLayout(targetPosZLayout);

    // Target Pitch (rotation around X)
    QLabel* targetPitchLabel = new QLabel("Pitch (X rotation)", targetControlsGroup);
    targetControlsLayout->addWidget(targetPitchLabel);

    QHBoxLayout* targetPitchLayout = new QHBoxLayout();
    targetPitchSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPitchSlider_->setRange(-180, 180);
    targetPitchSlider_->setValue(0);

    targetPitchSpinBox_ = new QSpinBox(targetControlsGroup);
    targetPitchSpinBox_->setRange(-180, 180);
    targetPitchSpinBox_->setValue(0);
    targetPitchSpinBox_->setMinimumWidth(60);  // Match Radar Controls spinbox width

    targetPitchLayout->addWidget(targetPitchSlider_);
    targetPitchLayout->addWidget(targetPitchSpinBox_);
    targetControlsLayout->addLayout(targetPitchLayout);

    // Target Yaw (rotation around Y)
    QLabel* targetYawLabel = new QLabel("Yaw (Y rotation)", targetControlsGroup);
    targetControlsLayout->addWidget(targetYawLabel);

    QHBoxLayout* targetYawLayout = new QHBoxLayout();
    targetYawSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetYawSlider_->setRange(-180, 180);
    targetYawSlider_->setValue(0);

    targetYawSpinBox_ = new QSpinBox(targetControlsGroup);
    targetYawSpinBox_->setRange(-180, 180);
    targetYawSpinBox_->setValue(0);
    targetYawSpinBox_->setMinimumWidth(60);  // Match Radar Controls spinbox width

    targetYawLayout->addWidget(targetYawSlider_);
    targetYawLayout->addWidget(targetYawSpinBox_);
    targetControlsLayout->addLayout(targetYawLayout);

    // Target Roll (rotation around Z)
    QLabel* targetRollLabel = new QLabel("Roll (Z rotation)", targetControlsGroup);
    targetControlsLayout->addWidget(targetRollLabel);

    QHBoxLayout* targetRollLayout = new QHBoxLayout();
    targetRollSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetRollSlider_->setRange(-180, 180);
    targetRollSlider_->setValue(0);

    targetRollSpinBox_ = new QSpinBox(targetControlsGroup);
    targetRollSpinBox_->setRange(-180, 180);
    targetRollSpinBox_->setValue(0);
    targetRollSpinBox_->setMinimumWidth(60);  // Match Radar Controls spinbox width

    targetRollLayout->addWidget(targetRollSlider_);
    targetRollLayout->addWidget(targetRollSpinBox_);
    targetControlsLayout->addLayout(targetRollLayout);

    // Target Scale
    QLabel* targetScaleLabel = new QLabel("Scale", targetControlsGroup);
    targetControlsLayout->addWidget(targetScaleLabel);

    QHBoxLayout* targetScaleLayout = new QHBoxLayout();
    targetScaleSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetScaleSlider_->setRange(1, 100);
    targetScaleSlider_->setValue(20);

    targetScaleSpinBox_ = new QSpinBox(targetControlsGroup);
    targetScaleSpinBox_->setRange(1, 100);
    targetScaleSpinBox_->setValue(20);
    targetScaleSpinBox_->setMinimumWidth(60);  // Match Radar Controls spinbox width

    targetScaleLayout->addWidget(targetScaleSlider_);
    targetScaleLayout->addWidget(targetScaleSpinBox_);
    targetControlsLayout->addLayout(targetScaleLayout);
}

void RadarSim::setupPhysicsAnalysisTab() {
    QVBoxLayout* physicsLayout = new QVBoxLayout(physicsTabWidget_);

    // Add a placeholder for the physics visualization
    QLabel* physicsVisPlaceholder = new QLabel("Physics Visualization", physicsTabWidget_);
    physicsVisPlaceholder->setMinimumSize(700, 300);
    physicsVisPlaceholder->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    physicsVisPlaceholder->setAlignment(Qt::AlignCenter);
    physicsVisPlaceholder->setStyleSheet("background-color: #f0f0f0;");
    physicsLayout->addWidget(physicsVisPlaceholder);

    // Add a tab widget for different analysis tools
    QTabWidget* analysisTabWidget = new QTabWidget(physicsTabWidget_);
    physicsLayout->addWidget(analysisTabWidget);

    // Create tabs for different analysis modes
    QWidget* emFieldTab = new QWidget(analysisTabWidget);
    QWidget* materialAnalysisTab = new QWidget(analysisTabWidget);
    QWidget* signatureTab = new QWidget(analysisTabWidget);

    analysisTabWidget->addTab(emFieldTab, "EM Field Analysis");
    analysisTabWidget->addTab(materialAnalysisTab, "Material Analysis");
    analysisTabWidget->addTab(signatureTab, "Radar Signature");

    // For each tab, add a simple placeholder layout
    QVBoxLayout* emFieldLayout = new QVBoxLayout(emFieldTab);
    emFieldLayout->addWidget(new QLabel("Electromagnetic field analysis tools will go here"));

    QVBoxLayout* materialLayout = new QVBoxLayout(materialAnalysisTab);
    materialLayout->addWidget(new QLabel("Material analysis tools will go here"));

    QVBoxLayout* signatureLayout = new QVBoxLayout(signatureTab);
    signatureLayout->addWidget(new QLabel("Radar signature analysis tools will go here"));
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

    // Connect target position X controls
    connect(targetPosXSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPosXChanged);
    connect(targetPosXSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onTargetPosXChanged);

    // Connect target position Y controls
    connect(targetPosYSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPosYChanged);
    connect(targetPosYSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onTargetPosYChanged);

    // Connect target position Z controls
    connect(targetPosZSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPosZChanged);
    connect(targetPosZSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onTargetPosZChanged);

    // Connect target pitch controls
    connect(targetPitchSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPitchChanged);
    connect(targetPitchSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onTargetPitchChanged);

    // Connect target yaw controls
    connect(targetYawSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetYawChanged);
    connect(targetYawSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onTargetYawChanged);

    // Connect target roll controls
    connect(targetRollSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetRollChanged);
    connect(targetRollSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onTargetRollChanged);

    // Connect target scale controls
    connect(targetScaleSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetScaleChanged);
    connect(targetScaleSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onTargetScaleChanged);
}

// Slot implementations
void RadarSim::onRadiusSliderValueChanged(int value) {
    // Block signals to prevent circular updates
    radiusSpinBox_->blockSignals(true);
    radiusSpinBox_->setValue(value);
    radiusSpinBox_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setRadius(value);
}

void RadarSim::onThetaSliderValueChanged(int value) {
    // Block signals to prevent circular updates
    thetaSpinBox_->blockSignals(true);

    // Reverse the sense for Azimuth slider
    int reversedValue = 359 - value;

    // Update the spin box with the reversed value
    thetaSpinBox_->setValue(reversedValue);
    thetaSpinBox_->blockSignals(false);

    // Update the radar scene widget with the reversed value
    radarSceneView_->setAngles(reversedValue, radarSceneView_->getPhi());
}

void RadarSim::onPhiSliderValueChanged(int value) {
    // Block signals to prevent circular updates
    phiSpinBox_->blockSignals(true);
    phiSpinBox_->setValue(value);
    phiSpinBox_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setAngles(radarSceneView_->getTheta(), value);
}

void RadarSim::onRadiusSpinBoxValueChanged(int value) {
    // Block signals to prevent circular updates
    radiusSlider_->blockSignals(true);
    radiusSlider_->setValue(value);
    radiusSlider_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setRadius(value);
}

void RadarSim::onThetaSpinBoxValueChanged(int value) {
    // Block signals to prevent circular updates
    thetaSlider_->blockSignals(true);

    // Set slider to reversed value (for display purposes)
    thetaSlider_->setValue(359 - value);
    thetaSlider_->blockSignals(false);

    // Update the radar scene widget with the direct spin box value
    radarSceneView_->setAngles(value, radarSceneView_->getPhi());
}

void RadarSim::onPhiSpinBoxValueChanged(int value) {
    // Block signals to prevent circular updates
    phiSlider_->blockSignals(true);
    phiSlider_->setValue(value);
    phiSlider_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setAngles(radarSceneView_->getTheta(), value);
}

// Target control slot implementations
void RadarSim::onTargetPosXChanged(int value) {
    // Sync slider and spinbox
    targetPosXSlider_->blockSignals(true);
    targetPosXSpinBox_->blockSignals(true);
    targetPosXSlider_->setValue(value);
    targetPosXSpinBox_->setValue(value);
    targetPosXSlider_->blockSignals(false);
    targetPosXSpinBox_->blockSignals(false);

    // Update the wireframe target position
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(static_cast<float>(value), pos.y(), pos.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPosYChanged(int value) {
    // Sync slider and spinbox
    targetPosYSlider_->blockSignals(true);
    targetPosYSpinBox_->blockSignals(true);
    targetPosYSlider_->setValue(value);
    targetPosYSpinBox_->setValue(value);
    targetPosYSlider_->blockSignals(false);
    targetPosYSpinBox_->blockSignals(false);

    // Update the wireframe target position
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(pos.x(), static_cast<float>(value), pos.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPosZChanged(int value) {
    // Sync slider and spinbox
    targetPosZSlider_->blockSignals(true);
    targetPosZSpinBox_->blockSignals(true);
    targetPosZSlider_->setValue(value);
    targetPosZSpinBox_->setValue(value);
    targetPosZSlider_->blockSignals(false);
    targetPosZSpinBox_->blockSignals(false);

    // Update the wireframe target position
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(pos.x(), pos.y(), static_cast<float>(value));
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPitchChanged(int value) {
    // Sync slider and spinbox
    targetPitchSlider_->blockSignals(true);
    targetPitchSpinBox_->blockSignals(true);
    targetPitchSlider_->setValue(value);
    targetPitchSpinBox_->setValue(value);
    targetPitchSlider_->blockSignals(false);
    targetPitchSpinBox_->blockSignals(false);

    // Update the wireframe target rotation
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(static_cast<float>(value), rot.y(), rot.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetYawChanged(int value) {
    // Sync slider and spinbox
    targetYawSlider_->blockSignals(true);
    targetYawSpinBox_->blockSignals(true);
    targetYawSlider_->setValue(value);
    targetYawSpinBox_->setValue(value);
    targetYawSlider_->blockSignals(false);
    targetYawSpinBox_->blockSignals(false);

    // Update the wireframe target rotation
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(rot.x(), static_cast<float>(value), rot.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetRollChanged(int value) {
    // Sync slider and spinbox
    targetRollSlider_->blockSignals(true);
    targetRollSpinBox_->blockSignals(true);
    targetRollSlider_->setValue(value);
    targetRollSpinBox_->setValue(value);
    targetRollSlider_->blockSignals(false);
    targetRollSpinBox_->blockSignals(false);

    // Update the wireframe target rotation
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(rot.x(), rot.y(), static_cast<float>(value));
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetScaleChanged(int value) {
    // Sync slider and spinbox
    targetScaleSlider_->blockSignals(true);
    targetScaleSpinBox_->blockSignals(true);
    targetScaleSlider_->setValue(value);
    targetScaleSpinBox_->setValue(value);
    targetScaleSlider_->blockSignals(false);
    targetScaleSpinBox_->blockSignals(false);

    // Update the wireframe target scale
    if (auto* controller = radarSceneView_->getWireframeController()) {
        controller->setScale(static_cast<float>(value));
        radarSceneView_->updateScene();
    }
}

// Profile management slot implementations
void RadarSim::onProfileSelected(int index) {
    if (index < 0 || !profileComboBox_) {
        return;
    }

    QString profileName = profileComboBox_->currentText();
    if (profileName.isEmpty()) {
        return;
    }

    // Load the selected profile
    if (appSettings_->loadProfile(profileName)) {
        applySettingsToScene();
        qDebug() << "Loaded profile:" << profileName;
    }
}

void RadarSim::onSaveProfile() {
    if (!profileComboBox_ || profileComboBox_->currentIndex() < 0) {
        // No profile selected, use Save As instead
        onSaveProfileAs();
        return;
    }

    QString profileName = profileComboBox_->currentText();
    if (profileName.isEmpty()) {
        onSaveProfileAs();
        return;
    }

    // Read current settings from scene and save
    readSettingsFromScene();
    if (appSettings_->saveProfile(profileName)) {
        qDebug() << "Saved profile:" << profileName;
    }
}

void RadarSim::onSaveProfileAs() {
    bool ok;
    QString name = QInputDialog::getText(this, "Save Profile As",
        "Profile name:", QLineEdit::Normal, "", &ok);

    if (ok && !name.isEmpty()) {
        // Read current settings from scene and save
        readSettingsFromScene();
        if (appSettings_->saveProfile(name)) {
            refreshProfileList();
            // Select the newly saved profile
            int index = profileComboBox_->findText(name);
            if (index >= 0) {
                profileComboBox_->setCurrentIndex(index);
            }
            qDebug() << "Saved new profile:" << name;
        }
    }
}

void RadarSim::onDeleteProfile() {
    if (!profileComboBox_ || profileComboBox_->currentIndex() < 0) {
        return;
    }

    QString profileName = profileComboBox_->currentText();
    if (profileName.isEmpty()) {
        return;
    }

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Delete Profile",
        QString("Are you sure you want to delete the profile '%1'?").arg(profileName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (appSettings_->deleteProfile(profileName)) {
            qDebug() << "Deleted profile:" << profileName;
        }
    }
}

void RadarSim::onResetToDefaults() {
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Reset to Defaults",
        "Are you sure you want to reset all settings to their default values?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        appSettings_->resetToDefaults();
        applySettingsToScene();
        qDebug() << "Reset to defaults";
    }
}

void RadarSim::onProfilesChanged() {
    refreshProfileList();
}

void RadarSim::closeEvent(QCloseEvent* event) {
    // Save current session before closing
    readSettingsFromScene();
    appSettings_->saveLastSession();
    qDebug() << "Saved last session on exit";

    event->accept();
}

void RadarSim::refreshProfileList() {
    if (!profileComboBox_) {
        return;
    }

    // Block signals while updating
    profileComboBox_->blockSignals(true);

    QString currentProfile = appSettings_->currentProfileName();
    profileComboBox_->clear();

    QStringList profiles = appSettings_->availableProfiles();
    profileComboBox_->addItems(profiles);

    // Restore selection
    if (!currentProfile.isEmpty()) {
        int index = profileComboBox_->findText(currentProfile);
        if (index >= 0) {
            profileComboBox_->setCurrentIndex(index);
        }
    }

    profileComboBox_->blockSignals(false);
}

void RadarSim::readSettingsFromScene() {
    if (!radarSceneView_) {
        return;
    }

    // Read scene settings
    appSettings_->scene.sphereRadius = static_cast<float>(radarSceneView_->getRadius());
    appSettings_->scene.radarTheta = static_cast<float>(radarSceneView_->getTheta());
    appSettings_->scene.radarPhi = static_cast<float>(radarSceneView_->getPhi());

    // Read camera settings
    if (auto* camera = radarSceneView_->getCameraController()) {
        appSettings_->camera.distance = camera->getDistance();
        appSettings_->camera.azimuth = camera->getAzimuth();
        appSettings_->camera.elevation = camera->getElevation();
        appSettings_->camera.focusPoint = camera->getFocusPoint();
        appSettings_->camera.inertiaEnabled = camera->isInertiaEnabled();
    }

    // Read target settings
    if (auto* target = radarSceneView_->getWireframeController()) {
        appSettings_->target.position = target->getPosition();
        appSettings_->target.rotation = target->getRotation();
        appSettings_->target.scale = target->getScale();
    }

    // Read beam settings from BeamController
    if (auto* beam = radarSceneView_->getBeamController()) {
        appSettings_->beam.beamWidth = beam->getBeamWidth();
        appSettings_->beam.opacity = beam->getBeamOpacity();
    }
}

void RadarSim::applySettingsToScene() {
    if (!radarSceneView_) {
        return;
    }

    // Apply scene settings
    radarSceneView_->setRadius(static_cast<int>(appSettings_->scene.sphereRadius));
    radarSceneView_->setAngles(
        static_cast<int>(appSettings_->scene.radarTheta),
        static_cast<int>(appSettings_->scene.radarPhi)
    );

    // Update UI controls to match
    radiusSlider_->blockSignals(true);
    radiusSpinBox_->blockSignals(true);
    radiusSlider_->setValue(static_cast<int>(appSettings_->scene.sphereRadius));
    radiusSpinBox_->setValue(static_cast<int>(appSettings_->scene.sphereRadius));
    radiusSlider_->blockSignals(false);
    radiusSpinBox_->blockSignals(false);

    thetaSlider_->blockSignals(true);
    thetaSpinBox_->blockSignals(true);
    thetaSlider_->setValue(359 - static_cast<int>(appSettings_->scene.radarTheta));
    thetaSpinBox_->setValue(static_cast<int>(appSettings_->scene.radarTheta));
    thetaSlider_->blockSignals(false);
    thetaSpinBox_->blockSignals(false);

    phiSlider_->blockSignals(true);
    phiSpinBox_->blockSignals(true);
    phiSlider_->setValue(static_cast<int>(appSettings_->scene.radarPhi));
    phiSpinBox_->setValue(static_cast<int>(appSettings_->scene.radarPhi));
    phiSlider_->blockSignals(false);
    phiSpinBox_->blockSignals(false);

    // Apply camera settings
    if (auto* camera = radarSceneView_->getCameraController()) {
        camera->setDistance(appSettings_->camera.distance);
        camera->setAzimuth(appSettings_->camera.azimuth);
        camera->setElevation(appSettings_->camera.elevation);
        camera->setFocusPoint(appSettings_->camera.focusPoint);
        camera->setInertiaEnabled(appSettings_->camera.inertiaEnabled);
    }

    // Apply target settings
    if (auto* target = radarSceneView_->getWireframeController()) {
        target->setPosition(
            appSettings_->target.position.x(),
            appSettings_->target.position.y(),
            appSettings_->target.position.z()
        );
        target->setRotation(
            appSettings_->target.rotation.x(),
            appSettings_->target.rotation.y(),
            appSettings_->target.rotation.z()
        );
        target->setScale(appSettings_->target.scale);

        // Update target UI controls
        targetPosXSlider_->blockSignals(true);
        targetPosXSpinBox_->blockSignals(true);
        targetPosXSlider_->setValue(static_cast<int>(appSettings_->target.position.x()));
        targetPosXSpinBox_->setValue(static_cast<int>(appSettings_->target.position.x()));
        targetPosXSlider_->blockSignals(false);
        targetPosXSpinBox_->blockSignals(false);

        targetPosYSlider_->blockSignals(true);
        targetPosYSpinBox_->blockSignals(true);
        targetPosYSlider_->setValue(static_cast<int>(appSettings_->target.position.y()));
        targetPosYSpinBox_->setValue(static_cast<int>(appSettings_->target.position.y()));
        targetPosYSlider_->blockSignals(false);
        targetPosYSpinBox_->blockSignals(false);

        targetPosZSlider_->blockSignals(true);
        targetPosZSpinBox_->blockSignals(true);
        targetPosZSlider_->setValue(static_cast<int>(appSettings_->target.position.z()));
        targetPosZSpinBox_->setValue(static_cast<int>(appSettings_->target.position.z()));
        targetPosZSlider_->blockSignals(false);
        targetPosZSpinBox_->blockSignals(false);

        targetPitchSlider_->blockSignals(true);
        targetPitchSpinBox_->blockSignals(true);
        targetPitchSlider_->setValue(static_cast<int>(appSettings_->target.rotation.x()));
        targetPitchSpinBox_->setValue(static_cast<int>(appSettings_->target.rotation.x()));
        targetPitchSlider_->blockSignals(false);
        targetPitchSpinBox_->blockSignals(false);

        targetYawSlider_->blockSignals(true);
        targetYawSpinBox_->blockSignals(true);
        targetYawSlider_->setValue(static_cast<int>(appSettings_->target.rotation.y()));
        targetYawSpinBox_->setValue(static_cast<int>(appSettings_->target.rotation.y()));
        targetYawSlider_->blockSignals(false);
        targetYawSpinBox_->blockSignals(false);

        targetRollSlider_->blockSignals(true);
        targetRollSpinBox_->blockSignals(true);
        targetRollSlider_->setValue(static_cast<int>(appSettings_->target.rotation.z()));
        targetRollSpinBox_->setValue(static_cast<int>(appSettings_->target.rotation.z()));
        targetRollSlider_->blockSignals(false);
        targetRollSpinBox_->blockSignals(false);

        targetScaleSlider_->blockSignals(true);
        targetScaleSpinBox_->blockSignals(true);
        targetScaleSlider_->setValue(static_cast<int>(appSettings_->target.scale));
        targetScaleSpinBox_->setValue(static_cast<int>(appSettings_->target.scale));
        targetScaleSlider_->blockSignals(false);
        targetScaleSpinBox_->blockSignals(false);
    }

    // Apply beam settings
    if (auto* beam = radarSceneView_->getBeamController()) {
        beam->setBeamWidth(appSettings_->beam.beamWidth);
        beam->setBeamOpacity(appSettings_->beam.opacity);
    }

    radarSceneView_->updateScene();
}