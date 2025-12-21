// ---- RadarSim.cpp ----

#include "RadarSim.h"
#include "SphereWidget.h"

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
    targetScaleSpinBox_(nullptr)
{
    setupUI();
    setupTabs();
    connectSignals();

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

    // Add buttons for loading/saving configuration
    QHBoxLayout* configButtonsLayout = new QHBoxLayout();
    QPushButton* loadConfigButton = new QPushButton("Load Configuration", configTabWidget_);
    QPushButton* saveConfigButton = new QPushButton("Save Configuration", configTabWidget_);
    configButtonsLayout->addWidget(loadConfigButton);
    configButtonsLayout->addWidget(saveConfigButton);
    configLayout->addLayout(configButtonsLayout);
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
        radarSceneView_->update();
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
        radarSceneView_->update();
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
        radarSceneView_->update();
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
        radarSceneView_->update();
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
        radarSceneView_->update();
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
        radarSceneView_->update();
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
        radarSceneView_->update();
    }
}