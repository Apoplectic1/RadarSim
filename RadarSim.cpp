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
    physicsTabWidget_(nullptr)
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

    // Set minimum window size
    setMinimumSize(1024, 800);
}

void RadarSim::setupTabs() {
/*
    1. We create frames without parents first, then add them to the splitter later
    2. We use setStretchFactor() instead of setSizes() for safer sizing
    3. We set appropriate size policies on the frames to help with proper sizing
    4. We add frames to the splitter only after all their contents are fully set up
    5. We maintain a clean hierarchy of ownership by being careful about parent-child relationships
*/

    qDebug() << "Starting setupTabs() - Full Implementation";

    // Create tab widgets with proper parent
    configTabWidget_ = new QWidget(tabWidget_);
    radarSceneTabWidget_ = new QWidget(tabWidget_);
    physicsTabWidget_ = new QWidget(tabWidget_);

    // Add tabs
    tabWidget_->addTab(configTabWidget_, "Configuration");
    tabWidget_->addTab(radarSceneTabWidget_, "Radar Scene");
    tabWidget_->addTab(physicsTabWidget_, "Physics Analysis");

    // ************************************************************************************************
    // Setup Configuration Tab
    // ************************************************************************************************

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

    // Add group to layout
    configLayout->addWidget(simSettingsGroup);

    // Create a group box for beam settings
    QGroupBox* beamSettingsGroup = new QGroupBox("Beam Settings", configTabWidget_);
    QFormLayout* beamSettingsLayout = new QFormLayout(beamSettingsGroup);

    // Add some placeholder settings
    QComboBox* beamTypeComboBox = new QComboBox(beamSettingsGroup);
    beamTypeComboBox->addItem("Conical");
    beamTypeComboBox->addItem("Phased Array");
    beamSettingsLayout->addRow("Default Beam Type:", beamTypeComboBox);

    QDoubleSpinBox* beamWidthSpinBox = new QDoubleSpinBox(beamSettingsGroup);
    beamWidthSpinBox->setRange(1.0, 45.0);
    beamWidthSpinBox->setValue(15.0);
    beamSettingsLayout->addRow("Default Beam Width (°):", beamWidthSpinBox);

    // Add group to layout
    configLayout->addWidget(beamSettingsGroup);


    // Add a group box for component architecture
    QGroupBox* architectureGroup = new QGroupBox("Architecture", configTabWidget_);
    QVBoxLayout* architectureLayout = new QVBoxLayout(architectureGroup);

    // Add switch for component-based rendering
    QCheckBox* useComponentsCheckbox = new QCheckBox("Use Component Architecture", architectureGroup);
    useComponentsCheckbox->setChecked(true);
    useComponentsCheckbox->setToolTip("Switch between old and new rendering architecture. Checked is new.");
    architectureLayout->addWidget(useComponentsCheckbox);

    // Connect checkbox to toggle component rendering
    connect(useComponentsCheckbox, &QCheckBox::toggled, [this](bool checked) {
        if (radarSceneView_) {
            radarSceneView_->enableComponentRendering(checked);
        }
        });

    // Add to config layout
    configLayout->addWidget(architectureGroup);

    // Add spacer at the bottom
    configLayout->addStretch();

    // Add buttons for loading/saving configuration
    QHBoxLayout* configButtonsLayout = new QHBoxLayout();
    QPushButton* loadConfigButton = new QPushButton("Load Configuration", configTabWidget_);
    QPushButton* saveConfigButton = new QPushButton("Save Configuration", configTabWidget_);
    configButtonsLayout->addWidget(loadConfigButton);
    configButtonsLayout->addWidget(saveConfigButton);
    configLayout->addLayout(configButtonsLayout);

    // ************************************************************************************************
    // Setup Radar Scene Tab
    // ************************************************************************************************

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

    // Create RadarSceneWidget
    radarSceneView_ = new RadarSceneWidget(sphereFrame);
    radarSceneView_->setMinimumSize(800, 500);
    sphereFrameLayout->addWidget(radarSceneView_);

    // Create controls group
    QGroupBox* controlsGroup = new QGroupBox("Radar Controls", controlsFrame);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
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

    phiLayout->addWidget(phiSlider_);
    phiLayout->addWidget(phiSpinBox_);
    controlsLayout->addLayout(phiLayout);

    // Add frames to splitter AFTER all setup is done
    radarSplitter_->addWidget(sphereFrame);
    radarSplitter_->addWidget(controlsFrame);

    // Use stretch factors instead of explicit sizes
    radarSplitter_->setStretchFactor(0, 5);  // RadarScene gets 5 parts
    radarSplitter_->setStretchFactor(1, 1);  // Controls get 1 part

    // ************************************************************************************************
    // Setup Physics Analysis Tab
    // ************************************************************************************************

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

    qDebug() << "Exiting setupTabs()";
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