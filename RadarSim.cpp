// ---- RadarSim.cpp ----

#include "RadarSim.h"
#include "ControlsWindow.h"
#include "AppSettings.h"
#include "PolarRCSPlot.h"
#include "RCSCompute/RCSSampler.h"  // For CutType enum
#include "Constants.h"

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
#include <QShowEvent>
#include <QMouseEvent>
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
    polarRCSPlot_(nullptr),
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
    appSettings_(new RSConfig::AppSettings(this))
{
    setupUI();
    setupLayout();
    setupMenuBar();
    setupConfigurationWindow();
    connectSignals();

    // Try to restore last session settings
    if (appSettings_->restoreLastSession()) {
        // Apply restored settings after a brief delay to ensure scene is ready
        QTimer::singleShot(100, this, &RadarSim::applySettingsToScene);
    }

    // Show floating windows at startup
    controlsWindow_->show();
    configWindow_->show();
}

// Destructor
RadarSim::~RadarSim() {
    // Any cleanup needed for tab widgets
}

void RadarSim::setupUI() {
    // Set the main window title
    setWindowTitle("Radar Simulation System");

    // Set minimum window size (controls panel now in floating window)
    setMinimumSize(640, 750);

    // Allow nested dock widgets
    setDockNestingEnabled(true);

    // No central widget - use dock widgets only
    // Create a minimal central widget to satisfy QMainWindow
    auto* centralWidget = new QWidget(this);
    centralWidget->setMaximumWidth(0);
    setCentralWidget(centralWidget);
}

void RadarSim::setupLayout() {
    setupSceneDocks();
    setupControlsWindow();
}

void RadarSim::setupSceneDocks() {
    // Create dock widget for 3D Radar Scene
    sceneDock_ = new QDockWidget("3D Radar Scene", this);
    sceneDock_->setFeatures(QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable |
                            QDockWidget::DockWidgetClosable);
    sceneDock_->setAllowedAreas(Qt::AllDockWidgetAreas);

    QWidget* sceneContainer = new QWidget();
    QHBoxLayout* sceneLayout = new QHBoxLayout(sceneContainer);
    sceneLayout->setContentsMargins(4, 0, 0, 0);  // Left margin only
    radarSceneView_ = new RadarSceneWidget();
    radarSceneView_->setMinimumSize(600, 350);
    sceneLayout->addWidget(radarSceneView_);
    sceneDock_->setWidget(sceneContainer);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock_);

    // Create dock widget for 2D Polar RCS Plot
    polarDock_ = new QDockWidget("2D Polar RCS Plot", this);
    polarDock_->setFeatures(QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable |
                            QDockWidget::DockWidgetClosable);
    polarDock_->setAllowedAreas(Qt::AllDockWidgetAreas);

    QWidget* polarContainer = new QWidget();
    QHBoxLayout* polarLayout = new QHBoxLayout(polarContainer);
    polarLayout->setContentsMargins(4, 0, 0, 0);  // Left margin only
    polarRCSPlot_ = new PolarRCSPlot();
    polarRCSPlot_->setMinimumSize(600, 350);
    polarLayout->addWidget(polarRCSPlot_);
    polarDock_->setWidget(polarContainer);
    addDockWidget(Qt::LeftDockWidgetArea, polarDock_);

    // Stack polar dock below scene dock
    splitDockWidget(sceneDock_, polarDock_, Qt::Vertical);
}

void RadarSim::setupControlsPanel() {
    // Create controls panel widget (will be parented to ControlsWindow)
    controlsPanel_ = new QWidget();

    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsPanel_);
    controlsLayout->setContentsMargins(4, 4, 4, 4);
    controlsLayout->setSpacing(8);

    // Setup control groups (title is handled by window title bar)
    setupRadarControls(controlsPanel_, controlsLayout);
    setupTargetControls(controlsPanel_, controlsLayout);
    setupRCSPlaneControls(controlsPanel_, controlsLayout);
    controlsLayout->addStretch();
}

void RadarSim::setupControlsWindow() {
    // First create the controls panel widget
    setupControlsPanel();

    // Create the floating window
    controlsWindow_ = new ControlsWindow(this);

    // Transfer the controls panel to the window
    controlsWindow_->setContent(controlsPanel_);
}

void RadarSim::setupMenuBar() {
    // Create menu bar
    QMenuBar* menuBar = this->menuBar();

    // View menu
    viewMenu_ = menuBar->addMenu("&View");

    // Controls window action
    showControlsWindowAction_ = new QAction("C&ontrols Window", this);
    showControlsWindowAction_->setStatusTip("Show the controls window");
    connect(showControlsWindowAction_, &QAction::triggered, this, &RadarSim::onShowControlsWindow);
    viewMenu_->addAction(showControlsWindowAction_);

    // Configuration window action
    showConfigWindowAction_ = new QAction("&Configuration Window", this);
    showConfigWindowAction_->setStatusTip("Show the configuration window");
    connect(showConfigWindowAction_, &QAction::triggered, this, &RadarSim::onShowConfigurationWindow);
    viewMenu_->addAction(showConfigWindowAction_);
}

void RadarSim::setupConfigurationWindow() {
    configWindow_ = new ConfigurationWindow(this);

    // Note: Position is set in showEvent() after main window geometry is valid

    // Connect profile signals
    connect(appSettings_, &RSConfig::AppSettings::profilesChanged,
            this, &RadarSim::onProfilesChanged);

    // Populate profile list
    refreshProfileList();
}

void RadarSim::setupRadarControls(QWidget* parent, QVBoxLayout* layout) {
    QGroupBox* controlsGroup = new QGroupBox("Radar Controls", parent);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(6, 0, 6, 0);
    layout->addWidget(controlsGroup);

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
    radiusSpinBox_->setMinimumWidth(60);  // Ensure consistent spinbox width

    radiusLayout->addWidget(radiusSlider_);
    radiusLayout->addWidget(radiusSpinBox_);
    controlsLayout->addLayout(radiusLayout);

    // Azimuth controls (0.5 degree increments: slider 0-718 maps to 0-359 degrees)
    QLabel* thetaLabel = new QLabel("Radar Azimuth (θ)", controlsGroup);
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
    QLabel* phiLabel = new QLabel("Radar Elevation (φ)", controlsGroup);
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

void RadarSim::setupTargetControls(QWidget* parent, QVBoxLayout* layout) {
    QGroupBox* targetControlsGroup = new QGroupBox("Target Controls", parent);
    QVBoxLayout* targetControlsLayout = new QVBoxLayout(targetControlsGroup);
    targetControlsLayout->setSpacing(0);  // Match Radar Controls spacing exactly
    targetControlsLayout->setContentsMargins(6, 0, 6, 0);
    layout->addWidget(targetControlsGroup);

    // Target Position X (0.5 increments: slider -200 to 200 maps to -100 to 100)
    QLabel* targetPosXLabel = new QLabel("Position X", targetControlsGroup);
    targetControlsLayout->addWidget(targetPosXLabel);

    QHBoxLayout* targetPosXLayout = new QHBoxLayout();
    targetPosXLayout->setContentsMargins(0, 0, 0, 0);
    targetPosXSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPosXSlider_->setRange(-200, 200);
    targetPosXSlider_->setValue(0);

    targetPosXSpinBox_ = new QDoubleSpinBox(targetControlsGroup);
    targetPosXSpinBox_->setRange(-100.0, 100.0);
    targetPosXSpinBox_->setSingleStep(0.5);
    targetPosXSpinBox_->setDecimals(1);
    targetPosXSpinBox_->setValue(0.0);
    targetPosXSpinBox_->setMinimumWidth(60);

    targetPosXLayout->addWidget(targetPosXSlider_);
    targetPosXLayout->addWidget(targetPosXSpinBox_);
    targetControlsLayout->addLayout(targetPosXLayout);

    // Target Position Y (0.5 increments)
    QLabel* targetPosYLabel = new QLabel("Position Y", targetControlsGroup);
    targetControlsLayout->addWidget(targetPosYLabel);

    QHBoxLayout* targetPosYLayout = new QHBoxLayout();
    targetPosYLayout->setContentsMargins(0, 0, 0, 0);
    targetPosYSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPosYSlider_->setRange(-200, 200);
    targetPosYSlider_->setValue(0);

    targetPosYSpinBox_ = new QDoubleSpinBox(targetControlsGroup);
    targetPosYSpinBox_->setRange(-100.0, 100.0);
    targetPosYSpinBox_->setSingleStep(0.5);
    targetPosYSpinBox_->setDecimals(1);
    targetPosYSpinBox_->setValue(0.0);
    targetPosYSpinBox_->setMinimumWidth(60);

    targetPosYLayout->addWidget(targetPosYSlider_);
    targetPosYLayout->addWidget(targetPosYSpinBox_);
    targetControlsLayout->addLayout(targetPosYLayout);

    // Target Position Z (0.5 increments)
    QLabel* targetPosZLabel = new QLabel("Position Z", targetControlsGroup);
    targetControlsLayout->addWidget(targetPosZLabel);

    QHBoxLayout* targetPosZLayout = new QHBoxLayout();
    targetPosZLayout->setContentsMargins(0, 0, 0, 0);
    targetPosZSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPosZSlider_->setRange(-200, 200);
    targetPosZSlider_->setValue(0);

    targetPosZSpinBox_ = new QDoubleSpinBox(targetControlsGroup);
    targetPosZSpinBox_->setRange(-100.0, 100.0);
    targetPosZSpinBox_->setSingleStep(0.5);
    targetPosZSpinBox_->setDecimals(1);
    targetPosZSpinBox_->setValue(0.0);
    targetPosZSpinBox_->setMinimumWidth(60);

    targetPosZLayout->addWidget(targetPosZSlider_);
    targetPosZLayout->addWidget(targetPosZSpinBox_);
    targetControlsLayout->addLayout(targetPosZLayout);

    // Target Pitch (0.5 degree increments: slider -360 to 360 maps to -180 to 180)
    QLabel* targetPitchLabel = new QLabel("Pitch (X rotation)", targetControlsGroup);
    targetControlsLayout->addWidget(targetPitchLabel);

    QHBoxLayout* targetPitchLayout = new QHBoxLayout();
    targetPitchLayout->setContentsMargins(0, 0, 0, 0);
    targetPitchSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetPitchSlider_->setRange(-360, 360);
    targetPitchSlider_->setValue(0);

    targetPitchSpinBox_ = new QDoubleSpinBox(targetControlsGroup);
    targetPitchSpinBox_->setRange(-180.0, 180.0);
    targetPitchSpinBox_->setSingleStep(0.5);
    targetPitchSpinBox_->setDecimals(1);
    targetPitchSpinBox_->setValue(0.0);
    targetPitchSpinBox_->setMinimumWidth(60);

    targetPitchLayout->addWidget(targetPitchSlider_);
    targetPitchLayout->addWidget(targetPitchSpinBox_);
    targetControlsLayout->addLayout(targetPitchLayout);

    // Target Yaw (0.5 degree increments)
    QLabel* targetYawLabel = new QLabel("Yaw (Y rotation)", targetControlsGroup);
    targetControlsLayout->addWidget(targetYawLabel);

    QHBoxLayout* targetYawLayout = new QHBoxLayout();
    targetYawLayout->setContentsMargins(0, 0, 0, 0);
    targetYawSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetYawSlider_->setRange(-360, 360);
    targetYawSlider_->setValue(0);

    targetYawSpinBox_ = new QDoubleSpinBox(targetControlsGroup);
    targetYawSpinBox_->setRange(-180.0, 180.0);
    targetYawSpinBox_->setSingleStep(0.5);
    targetYawSpinBox_->setDecimals(1);
    targetYawSpinBox_->setValue(0.0);
    targetYawSpinBox_->setMinimumWidth(60);

    targetYawLayout->addWidget(targetYawSlider_);
    targetYawLayout->addWidget(targetYawSpinBox_);
    targetControlsLayout->addLayout(targetYawLayout);

    // Target Roll (0.5 degree increments)
    QLabel* targetRollLabel = new QLabel("Roll (Z rotation)", targetControlsGroup);
    targetControlsLayout->addWidget(targetRollLabel);

    QHBoxLayout* targetRollLayout = new QHBoxLayout();
    targetRollLayout->setContentsMargins(0, 0, 0, 0);
    targetRollSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetRollSlider_->setRange(-360, 360);
    targetRollSlider_->setValue(0);

    targetRollSpinBox_ = new QDoubleSpinBox(targetControlsGroup);
    targetRollSpinBox_->setRange(-180.0, 180.0);
    targetRollSpinBox_->setSingleStep(0.5);
    targetRollSpinBox_->setDecimals(1);
    targetRollSpinBox_->setValue(0.0);
    targetRollSpinBox_->setMinimumWidth(60);

    targetRollLayout->addWidget(targetRollSlider_);
    targetRollLayout->addWidget(targetRollSpinBox_);
    targetControlsLayout->addLayout(targetRollLayout);

    // Target Scale (0.5 increments: slider 2 to 200 maps to 1 to 100)
    QLabel* targetScaleLabel = new QLabel("Scale", targetControlsGroup);
    targetControlsLayout->addWidget(targetScaleLabel);

    QHBoxLayout* targetScaleLayout = new QHBoxLayout();
    targetScaleLayout->setContentsMargins(0, 0, 0, 0);
    targetScaleSlider_ = new QSlider(Qt::Horizontal, targetControlsGroup);
    targetScaleSlider_->setRange(2, 200);
    targetScaleSlider_->setValue(40);  // 20 * 2

    targetScaleSpinBox_ = new QDoubleSpinBox(targetControlsGroup);
    targetScaleSpinBox_->setRange(1.0, 100.0);
    targetScaleSpinBox_->setSingleStep(0.5);
    targetScaleSpinBox_->setDecimals(1);
    targetScaleSpinBox_->setValue(20.0);
    targetScaleSpinBox_->setMinimumWidth(60);

    targetScaleLayout->addWidget(targetScaleSlider_);
    targetScaleLayout->addWidget(targetScaleSpinBox_);
    targetControlsLayout->addLayout(targetScaleLayout);

    // Install event filters for double-click reset
    targetPosXSlider_->installEventFilter(this);
    targetPosYSlider_->installEventFilter(this);
    targetPosZSlider_->installEventFilter(this);
    targetPitchSlider_->installEventFilter(this);
    targetYawSlider_->installEventFilter(this);
    targetRollSlider_->installEventFilter(this);
    targetScaleSlider_->installEventFilter(this);
}

void RadarSim::setupRCSPlaneControls(QWidget* parent, QVBoxLayout* layout) {
    QGroupBox* rcsPlaneGroup = new QGroupBox("2D RCS Plane", parent);
    QVBoxLayout* rcsPlaneLayout = new QVBoxLayout(rcsPlaneGroup);
    rcsPlaneLayout->setSpacing(0);
    rcsPlaneLayout->setContentsMargins(6, 0, 6, 0);
    layout->addWidget(rcsPlaneGroup);

    // Cut Type selector
    QLabel* cutTypeLabel = new QLabel("Cut Type", rcsPlaneGroup);
    rcsPlaneLayout->addWidget(cutTypeLabel);

    rcsCutTypeComboBox_ = new QComboBox(rcsPlaneGroup);
    rcsCutTypeComboBox_->addItem("Azimuth Cut");      // Index 0
    rcsCutTypeComboBox_->addItem("Elevation Cut");    // Index 1
    rcsCutTypeComboBox_->setCurrentIndex(0);
    rcsPlaneLayout->addWidget(rcsCutTypeComboBox_);

    // Plane Offset control
    QLabel* offsetLabel = new QLabel("Plane Offset (°)", rcsPlaneGroup);
    rcsPlaneLayout->addWidget(offsetLabel);

    QHBoxLayout* offsetLayout = new QHBoxLayout();
    offsetLayout->setContentsMargins(0, 0, 0, 0);
    rcsPlaneOffsetSlider_ = new QSlider(Qt::Horizontal, rcsPlaneGroup);
    rcsPlaneOffsetSlider_->setRange(-90, 90);
    rcsPlaneOffsetSlider_->setValue(0);

    rcsPlaneOffsetSpinBox_ = new QSpinBox(rcsPlaneGroup);
    rcsPlaneOffsetSpinBox_->setRange(-90, 90);
    rcsPlaneOffsetSpinBox_->setValue(0);
    rcsPlaneOffsetSpinBox_->setMinimumWidth(60);

    offsetLayout->addWidget(rcsPlaneOffsetSlider_);
    offsetLayout->addWidget(rcsPlaneOffsetSpinBox_);
    rcsPlaneLayout->addLayout(offsetLayout);

    // Slice Thickness control (non-linear scale: 0.1° steps 0.5-3°, 1° steps 3-40°)
    QLabel* thicknessLabel = new QLabel("Slice Thickness (±°)", rcsPlaneGroup);
    rcsPlaneLayout->addWidget(thicknessLabel);

    QHBoxLayout* thicknessLayout = new QHBoxLayout();
    thicknessLayout->setContentsMargins(0, 0, 0, 0);
    rcsSliceThicknessSlider_ = new QSlider(Qt::Horizontal, rcsPlaneGroup);
    rcsSliceThicknessSlider_->setRange(0, RS::Constants::kThicknessScaleSize - 1);
    rcsSliceThicknessSlider_->setValue(RS::Constants::kDefaultThicknessIndex);

    rcsSliceThicknessSpinBox_ = new QDoubleSpinBox(rcsPlaneGroup);
    rcsSliceThicknessSpinBox_->setRange(0.5, 40.0);
    rcsSliceThicknessSpinBox_->setDecimals(1);
    rcsSliceThicknessSpinBox_->setSingleStep(0.1);
    rcsSliceThicknessSpinBox_->setValue(RS::Constants::kThicknessScale[RS::Constants::kDefaultThicknessIndex]);
    rcsSliceThicknessSpinBox_->setMinimumWidth(60);

    thicknessLayout->addWidget(rcsSliceThicknessSlider_);
    thicknessLayout->addWidget(rcsSliceThicknessSpinBox_);
    rcsPlaneLayout->addLayout(thicknessLayout);

    // Show Fill checkbox
    rcsPlaneShowFillCheckBox_ = new QCheckBox("Show Plane Fill", rcsPlaneGroup);
    rcsPlaneShowFillCheckBox_->setChecked(true);
    rcsPlaneLayout->addWidget(rcsPlaneShowFillCheckBox_);

    // Install event filters for double-click reset
    rcsPlaneOffsetSlider_->installEventFilter(this);
    rcsSliceThicknessSlider_->installEventFilter(this);
}

void RadarSim::connectSignals() {
    // Connect radius slider and spin box
    connect(radiusSlider_, &QSlider::valueChanged, this, &RadarSim::onRadiusSliderValueChanged);
    connect(radiusSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onRadiusSpinBoxValueChanged);

    // Connect theta (azimuth) slider and spin box
    connect(thetaSlider_, &QSlider::valueChanged, this, &RadarSim::onThetaSliderValueChanged);
    connect(thetaSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onThetaSpinBoxValueChanged);

    // Connect phi (elevation) slider and spin box
    connect(phiSlider_, &QSlider::valueChanged, this, &RadarSim::onPhiSliderValueChanged);
    connect(phiSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onPhiSpinBoxValueChanged);

    // Connect target position X controls
    connect(targetPosXSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPosXSliderChanged);
    connect(targetPosXSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onTargetPosXSpinBoxChanged);

    // Connect target position Y controls
    connect(targetPosYSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPosYSliderChanged);
    connect(targetPosYSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onTargetPosYSpinBoxChanged);

    // Connect target position Z controls
    connect(targetPosZSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPosZSliderChanged);
    connect(targetPosZSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onTargetPosZSpinBoxChanged);

    // Connect target pitch controls
    connect(targetPitchSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetPitchSliderChanged);
    connect(targetPitchSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onTargetPitchSpinBoxChanged);

    // Connect target yaw controls
    connect(targetYawSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetYawSliderChanged);
    connect(targetYawSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onTargetYawSpinBoxChanged);

    // Connect target roll controls
    connect(targetRollSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetRollSliderChanged);
    connect(targetRollSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onTargetRollSpinBoxChanged);

    // Connect target scale controls
    connect(targetScaleSlider_, &QSlider::valueChanged, this, &RadarSim::onTargetScaleSliderChanged);
    connect(targetScaleSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RadarSim::onTargetScaleSpinBoxChanged);

    // Connect polar plot data from radar scene
    connect(radarSceneView_, &RadarSceneWidget::polarPlotDataReady,
            polarRCSPlot_, &PolarRCSPlot::setData);

    // Connect RCS plane controls
    connect(rcsCutTypeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RadarSim::onRCSCutTypeChanged);
    connect(rcsPlaneOffsetSlider_, &QSlider::valueChanged, this, &RadarSim::onRCSPlaneOffsetChanged);
    connect(rcsPlaneOffsetSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &RadarSim::onRCSPlaneOffsetChanged);
    // Thickness slider uses lookup table, spinbox just displays value
    connect(rcsSliceThicknessSlider_, &QSlider::valueChanged, this, &RadarSim::onRCSSliceThicknessChanged);
    connect(rcsPlaneShowFillCheckBox_, &QCheckBox::toggled, this, &RadarSim::onRCSPlaneShowFillChanged);

    // Connect pop-out signals from widgets
    connect(radarSceneView_, &RadarSceneWidget::popoutRequested,
            this, &RadarSim::onScenePopoutRequested);
    connect(polarRCSPlot_, &PolarRCSPlot::popoutRequested,
            this, &RadarSim::onPolarPopoutRequested);

    // Connect ConfigurationWindow signals
    connect(configWindow_, &ConfigurationWindow::profileSelected,
            this, &RadarSim::onProfileSelected);
    connect(configWindow_, &ConfigurationWindow::saveRequested,
            this, &RadarSim::onSaveProfile);
    connect(configWindow_, &ConfigurationWindow::saveAsRequested,
            this, &RadarSim::onSaveProfileAs);
    connect(configWindow_, &ConfigurationWindow::deleteRequested,
            this, &RadarSim::onDeleteProfile);
    connect(configWindow_, &ConfigurationWindow::resetRequested,
            this, &RadarSim::onResetToDefaults);

    // Visibility signals
    connect(configWindow_, &ConfigurationWindow::axesVisibilityChanged,
            this, &RadarSim::onAxesVisibilityChanged);
    connect(configWindow_, &ConfigurationWindow::sphereVisibilityChanged,
            this, &RadarSim::onSphereVisibilityChanged);
    connect(configWindow_, &ConfigurationWindow::gridVisibilityChanged,
            this, &RadarSim::onGridVisibilityChanged);
    connect(configWindow_, &ConfigurationWindow::inertiaChanged,
            this, &RadarSim::onInertiaChanged);
    connect(configWindow_, &ConfigurationWindow::reflectionLobesChanged,
            this, &RadarSim::onReflectionLobesChanged);
    connect(configWindow_, &ConfigurationWindow::heatMapChanged,
            this, &RadarSim::onHeatMapChanged);

    // Beam signals
    connect(configWindow_, &ConfigurationWindow::beamVisibilityChanged,
            this, &RadarSim::onBeamVisibilityChanged);
    connect(configWindow_, &ConfigurationWindow::beamShadowChanged,
            this, &RadarSim::onBeamShadowChanged);
    connect(configWindow_, &ConfigurationWindow::beamTypeChanged,
            this, &RadarSim::onBeamTypeChanged);

    // Target signals
    connect(configWindow_, &ConfigurationWindow::targetVisibilityChanged,
            this, &RadarSim::onTargetVisibilityChanged);
    connect(configWindow_, &ConfigurationWindow::targetTypeChanged,
            this, &RadarSim::onTargetTypeChanged);
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

    // Slider value is in half-degrees (0-718), convert to degrees (0-359)
    // Also reverse the sense for Azimuth slider
    double degrees = (718 - value) / 2.0;

    // Update the spin box with the reversed value
    thetaSpinBox_->setValue(degrees);
    thetaSpinBox_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setAngles(degrees, radarSceneView_->getPhi());
}

void RadarSim::onPhiSliderValueChanged(int value) {
    // Block signals to prevent circular updates
    phiSpinBox_->blockSignals(true);

    // Slider value is in half-degrees (-180 to 180), convert to degrees (-90 to 90)
    double degrees = value / 2.0;
    phiSpinBox_->setValue(degrees);
    phiSpinBox_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setAngles(radarSceneView_->getTheta(), degrees);
}

void RadarSim::onRadiusSpinBoxValueChanged(int value) {
    // Block signals to prevent circular updates
    radiusSlider_->blockSignals(true);
    radiusSlider_->setValue(value);
    radiusSlider_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setRadius(value);
}

void RadarSim::onThetaSpinBoxValueChanged(double value) {
    // Block signals to prevent circular updates
    thetaSlider_->blockSignals(true);

    // Convert degrees to half-degrees and reverse for slider
    int sliderValue = static_cast<int>((359.0 - value) * 2);
    thetaSlider_->setValue(sliderValue);
    thetaSlider_->blockSignals(false);

    // Update the radar scene widget with the direct spin box value
    radarSceneView_->setAngles(value, radarSceneView_->getPhi());
}

void RadarSim::onPhiSpinBoxValueChanged(double value) {
    // Block signals to prevent circular updates
    phiSlider_->blockSignals(true);

    // Convert degrees to half-degrees for slider
    int sliderValue = static_cast<int>(value * 2);
    phiSlider_->setValue(sliderValue);
    phiSlider_->blockSignals(false);

    // Update the radar scene widget
    radarSceneView_->setAngles(radarSceneView_->getTheta(), value);
}

// Helper to sync slider and spinbox values without triggering signals
void RadarSim::syncSliderSpinBox(QSlider* slider, QSpinBox* spinBox, int value) {
    slider->blockSignals(true);
    spinBox->blockSignals(true);
    slider->setValue(value);
    spinBox->setValue(value);
    slider->blockSignals(false);
    spinBox->blockSignals(false);
}

// Target control slot implementations - slider handlers
void RadarSim::onTargetPosXSliderChanged(int value) {
    double actualValue = value / 2.0;
    targetPosXSpinBox_->blockSignals(true);
    targetPosXSpinBox_->setValue(actualValue);
    targetPosXSpinBox_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(static_cast<float>(actualValue), pos.y(), pos.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPosYSliderChanged(int value) {
    double actualValue = value / 2.0;
    targetPosYSpinBox_->blockSignals(true);
    targetPosYSpinBox_->setValue(actualValue);
    targetPosYSpinBox_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(pos.x(), static_cast<float>(actualValue), pos.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPosZSliderChanged(int value) {
    double actualValue = value / 2.0;
    targetPosZSpinBox_->blockSignals(true);
    targetPosZSpinBox_->setValue(actualValue);
    targetPosZSpinBox_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(pos.x(), pos.y(), static_cast<float>(actualValue));
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPitchSliderChanged(int value) {
    double actualValue = value / 2.0;
    targetPitchSpinBox_->blockSignals(true);
    targetPitchSpinBox_->setValue(actualValue);
    targetPitchSpinBox_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(static_cast<float>(actualValue), rot.y(), rot.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetYawSliderChanged(int value) {
    double actualValue = value / 2.0;
    targetYawSpinBox_->blockSignals(true);
    targetYawSpinBox_->setValue(actualValue);
    targetYawSpinBox_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(rot.x(), static_cast<float>(actualValue), rot.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetRollSliderChanged(int value) {
    double actualValue = value / 2.0;
    targetRollSpinBox_->blockSignals(true);
    targetRollSpinBox_->setValue(actualValue);
    targetRollSpinBox_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(rot.x(), rot.y(), static_cast<float>(actualValue));
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetScaleSliderChanged(int value) {
    double actualValue = value / 2.0;
    targetScaleSpinBox_->blockSignals(true);
    targetScaleSpinBox_->setValue(actualValue);
    targetScaleSpinBox_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        controller->setScale(static_cast<float>(actualValue));
        radarSceneView_->updateScene();
    }
}

// Target control slot implementations - spinbox handlers
void RadarSim::onTargetPosXSpinBoxChanged(double value) {
    targetPosXSlider_->blockSignals(true);
    targetPosXSlider_->setValue(static_cast<int>(value * 2));
    targetPosXSlider_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(static_cast<float>(value), pos.y(), pos.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPosYSpinBoxChanged(double value) {
    targetPosYSlider_->blockSignals(true);
    targetPosYSlider_->setValue(static_cast<int>(value * 2));
    targetPosYSlider_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(pos.x(), static_cast<float>(value), pos.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPosZSpinBoxChanged(double value) {
    targetPosZSlider_->blockSignals(true);
    targetPosZSlider_->setValue(static_cast<int>(value * 2));
    targetPosZSlider_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D pos = controller->getPosition();
        controller->setPosition(pos.x(), pos.y(), static_cast<float>(value));
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetPitchSpinBoxChanged(double value) {
    targetPitchSlider_->blockSignals(true);
    targetPitchSlider_->setValue(static_cast<int>(value * 2));
    targetPitchSlider_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(static_cast<float>(value), rot.y(), rot.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetYawSpinBoxChanged(double value) {
    targetYawSlider_->blockSignals(true);
    targetYawSlider_->setValue(static_cast<int>(value * 2));
    targetYawSlider_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(rot.x(), static_cast<float>(value), rot.z());
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetRollSpinBoxChanged(double value) {
    targetRollSlider_->blockSignals(true);
    targetRollSlider_->setValue(static_cast<int>(value * 2));
    targetRollSlider_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        QVector3D rot = controller->getRotation();
        controller->setRotation(rot.x(), rot.y(), static_cast<float>(value));
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetScaleSpinBoxChanged(double value) {
    targetScaleSlider_->blockSignals(true);
    targetScaleSlider_->setValue(static_cast<int>(value * 2));
    targetScaleSlider_->blockSignals(false);
    if (auto* controller = radarSceneView_->getWireframeController()) {
        controller->setScale(static_cast<float>(value));
        radarSceneView_->updateScene();
    }
}

// Configuration window slot implementations
void RadarSim::onShowConfigurationWindow() {
    if (configWindow_) {
        configWindow_->show();
        configWindow_->raise();
        configWindow_->activateWindow();
    }
}

void RadarSim::onShowControlsWindow() {
    if (controlsWindow_) {
        controlsWindow_->show();
        controlsWindow_->raise();
        controlsWindow_->activateWindow();
    }
}

void RadarSim::onAxesVisibilityChanged(bool visible) {
    radarSceneView_->setAxesVisible(visible);
}

void RadarSim::onSphereVisibilityChanged(bool visible) {
    radarSceneView_->setSphereVisible(visible);
}

void RadarSim::onGridVisibilityChanged(bool visible) {
    radarSceneView_->setGridLinesVisible(visible);
}

void RadarSim::onInertiaChanged(bool enabled) {
    radarSceneView_->setInertiaEnabled(enabled);
}

void RadarSim::onReflectionLobesChanged(bool visible) {
    if (auto* glWidget = radarSceneView_->getGLWidget()) {
        glWidget->setReflectionLobesVisible(visible);
    }
}

void RadarSim::onHeatMapChanged(bool visible) {
    if (auto* glWidget = radarSceneView_->getGLWidget()) {
        glWidget->setHeatMapVisible(visible);
    }
}

void RadarSim::onBeamVisibilityChanged(bool visible) {
    if (auto* beam = radarSceneView_->getBeamController()) {
        beam->setFootprintOnly(!visible);  // visible = full beam, !visible = footprint only
        radarSceneView_->updateScene();
    }
}

void RadarSim::onBeamShadowChanged(bool visible) {
    radarSceneView_->setShowShadow(visible);
}

void RadarSim::onBeamTypeChanged(BeamType type) {
    if (auto* beam = radarSceneView_->getBeamController()) {
        beam->setBeamType(type);
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetVisibilityChanged(bool visible) {
    if (auto* target = radarSceneView_->getWireframeController()) {
        target->setVisible(visible);
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetTypeChanged(WireframeType type) {
    if (auto* target = radarSceneView_->getWireframeController()) {
        target->setTargetType(type);
        radarSceneView_->updateScene();
    }
}

// RCS plane control slot implementations
void RadarSim::onRCSCutTypeChanged(int index) {
    CutType type = static_cast<CutType>(index);
    radarSceneView_->setRCSCutType(type);
}

void RadarSim::onRCSPlaneOffsetChanged(int value) {
    syncSliderSpinBox(rcsPlaneOffsetSlider_, rcsPlaneOffsetSpinBox_, value);
    radarSceneView_->setRCSPlaneOffset(static_cast<float>(value));
}

void RadarSim::onRCSSliceThicknessChanged(int sliderIndex) {
    // Get actual thickness from lookup table
    float thickness = RS::Constants::kThicknessScale[sliderIndex];

    // Update spinbox display (read-only, just shows the value)
    rcsSliceThicknessSpinBox_->blockSignals(true);
    rcsSliceThicknessSpinBox_->setValue(static_cast<double>(thickness));
    rcsSliceThicknessSpinBox_->blockSignals(false);

    // Apply to scene
    radarSceneView_->setRCSSliceThickness(thickness);
}

void RadarSim::onRCSPlaneShowFillChanged(bool checked) {
    radarSceneView_->setRCSPlaneShowFill(checked);
}

// Profile management slot implementations
void RadarSim::onProfileSelected(int index) {
    if (index < 0 || !configWindow_) {
        return;
    }

    // Get profile name from AppSettings since ConfigurationWindow uses string list
    QStringList profiles = appSettings_->availableProfiles();
    if (index >= profiles.size()) {
        return;
    }
    QString profileName = profiles.at(index);
    if (profileName.isEmpty()) {
        return;
    }

    // Load the selected profile
    if (appSettings_->loadProfile(profileName)) {
        applySettingsToScene();
    }
}

void RadarSim::onSaveProfile() {
    if (!configWindow_ || configWindow_->currentProfileIndex() < 0) {
        // No profile selected, use Save As instead
        onSaveProfileAs();
        return;
    }

    QStringList profiles = appSettings_->availableProfiles();
    int index = configWindow_->currentProfileIndex();
    if (index >= profiles.size()) {
        onSaveProfileAs();
        return;
    }
    QString profileName = profiles.at(index);
    if (profileName.isEmpty()) {
        onSaveProfileAs();
        return;
    }

    // Read current settings from scene and save
    readSettingsFromScene();
    appSettings_->saveProfile(profileName);
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
            QStringList profiles = appSettings_->availableProfiles();
            int index = profiles.indexOf(name);
            if (index >= 0 && configWindow_) {
                configWindow_->setCurrentProfile(index);
            }
        }
    }
}

void RadarSim::onDeleteProfile() {
    if (!configWindow_ || configWindow_->currentProfileIndex() < 0) {
        return;
    }

    QStringList profiles = appSettings_->availableProfiles();
    int index = configWindow_->currentProfileIndex();
    if (index >= profiles.size()) {
        return;
    }
    QString profileName = profiles.at(index);
    if (profileName.isEmpty()) {
        return;
    }

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Delete Profile",
        QString("Are you sure you want to delete the profile '%1'?").arg(profileName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        appSettings_->deleteProfile(profileName);
    }
}

void RadarSim::onResetToDefaults() {
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Reset to Defaults",
        "Are you sure?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Preserve beam type during reset, but show full beam
        int savedBeamType = appSettings_->beam.beamType;
        appSettings_->resetToDefaults();
        appSettings_->beam.beamType = savedBeamType;
        appSettings_->beam.footprintOnly = false;  // Reset shows full beam
        applySettingsToScene();
    }
}

void RadarSim::onProfilesChanged() {
    refreshProfileList();
}

void RadarSim::closeEvent(QCloseEvent* event) {
    readSettingsFromScene();
    appSettings_->saveLastSession();
    event->accept();
}

void RadarSim::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);

    // Position floating windows now that main window geometry is valid
    positionControlsWindow();
    positionConfigWindow();

    // Force polar plot to resize after window is fully visible
    // This triggers proper OpenGL initialization that doesn't happen on initial layout
    QTimer::singleShot(50, this, [this]() {
        if (polarRCSPlot_) {
            // Trigger a resize event by resizing to current size
            QSize size = polarRCSPlot_->size();
            polarRCSPlot_->resize(size.width() + 1, size.height());
            polarRCSPlot_->resize(size);
        }
    });
}

void RadarSim::positionControlsWindow() {
    if (!controlsWindow_) return;

    // Position to the LEFT of the main window with 10px gap
    int newX = x() - controlsWindow_->width() - 10;

    // Ensure window doesn't go off-screen to the left
    if (newX < 0) {
        newX = 0;
    }

    controlsWindow_->move(newX, y());
}

void RadarSim::positionConfigWindow() {
    if (!configWindow_) return;

    // Position to the right of the main window with 10px gap
    configWindow_->move(x() + width() + 10, y());
}

bool RadarSim::eventFilter(QObject* obj, QEvent* event) {
    using namespace RS::Constants::Defaults;

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // Radar controls
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
            // Target position (sliders use 2x for 0.5 increments)
            if (obj == targetPosXSlider_) {
                targetPosXSlider_->setValue(static_cast<int>(kTargetPositionX * 2));
                return true;
            }
            if (obj == targetPosYSlider_) {
                targetPosYSlider_->setValue(static_cast<int>(kTargetPositionY * 2));
                return true;
            }
            if (obj == targetPosZSlider_) {
                targetPosZSlider_->setValue(static_cast<int>(kTargetPositionZ * 2));
                return true;
            }
            // Target rotation (sliders use 2x for 0.5 degree increments)
            if (obj == targetPitchSlider_) {
                targetPitchSlider_->setValue(static_cast<int>(kTargetPitch * 2));
                return true;
            }
            if (obj == targetYawSlider_) {
                targetYawSlider_->setValue(static_cast<int>(kTargetYaw * 2));
                return true;
            }
            if (obj == targetRollSlider_) {
                targetRollSlider_->setValue(static_cast<int>(kTargetRoll * 2));
                return true;
            }
            // Target scale (slider uses 2x for 0.5 increments)
            if (obj == targetScaleSlider_) {
                targetScaleSlider_->setValue(static_cast<int>(kTargetScale * 2));
                return true;
            }
            // RCS plane controls
            if (obj == rcsPlaneOffsetSlider_) {
                rcsPlaneOffsetSlider_->setValue(static_cast<int>(kRCSPlaneOffset));
                return true;
            }
            if (obj == rcsSliceThicknessSlider_) {
                rcsSliceThicknessSlider_->setValue(RS::Constants::kDefaultThicknessIndex);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void RadarSim::refreshProfileList() {
    if (!configWindow_) {
        return;
    }

    QString currentProfile = appSettings_->currentProfileName();
    QStringList profiles = appSettings_->availableProfiles();

    configWindow_->setProfiles(profiles);

    // Restore selection
    if (!currentProfile.isEmpty()) {
        int index = profiles.indexOf(currentProfile);
        if (index >= 0) {
            configWindow_->setCurrentProfile(index);
        }
    }
}

void RadarSim::syncConfigWindowState() {
    if (!configWindow_ || !radarSceneView_) {
        return;
    }

    // Get current state from scene
    bool axesVisible = radarSceneView_->areAxesVisible();
    bool sphereVisible = radarSceneView_->isSphereVisible();
    bool gridVisible = radarSceneView_->areGridLinesVisible();
    bool inertiaEnabled = radarSceneView_->isInertiaEnabled();

    // Get beam state
    bool beamVisible = true;
    bool shadowVisible = radarSceneView_->isShowShadow();
    BeamType beamType = BeamType::Sinc;
    if (auto* beam = radarSceneView_->getBeamController()) {
        beamVisible = !beam->isFootprintOnly();  // visible = full beam
        beamType = beam->getBeamType();
    }

    // Get target state
    bool targetVisible = true;
    WireframeType targetType = WireframeType::Cube;
    if (auto* target = radarSceneView_->getWireframeController()) {
        targetVisible = target->isVisible();
        targetType = target->getTargetType();
    }

    // Get visualization state
    bool reflectionLobesVisible = false;
    bool heatMapVisible = false;
    if (auto* glWidget = radarSceneView_->getGLWidget()) {
        reflectionLobesVisible = glWidget->areReflectionLobesVisible();
        heatMapVisible = glWidget->isHeatMapVisible();
    }

    // Sync to config window
    configWindow_->syncStateFromScene(axesVisible, sphereVisible, gridVisible,
                                       inertiaEnabled, reflectionLobesVisible, heatMapVisible,
                                       beamVisible, shadowVisible, beamType,
                                       targetVisible, targetType);
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
        appSettings_->target.targetType = static_cast<int>(target->getTargetType());
        appSettings_->target.position = target->getPosition();
        appSettings_->target.rotation = target->getRotation();
        appSettings_->target.scale = target->getScale();
    }

    // Read beam settings from BeamController
    if (auto* beam = radarSceneView_->getBeamController()) {
        appSettings_->beam.beamType = static_cast<int>(beam->getBeamType());
        appSettings_->beam.beamWidth = beam->getBeamWidth();
        appSettings_->beam.opacity = beam->getBeamOpacity();
        appSettings_->beam.footprintOnly = beam->isFootprintOnly();
    }

    // Read visibility settings
    appSettings_->scene.showAxes = radarSceneView_->areAxesVisible();
    appSettings_->scene.showSphere = radarSceneView_->isSphereVisible();
    appSettings_->scene.showGrid = radarSceneView_->areGridLinesVisible();
    appSettings_->scene.showShadow = radarSceneView_->isShowShadow();

    // Read RCS plane settings
    appSettings_->scene.rcsCutType = static_cast<int>(radarSceneView_->getRCSCutType());
    appSettings_->scene.rcsPlaneOffset = radarSceneView_->getRCSPlaneOffset();
    appSettings_->scene.rcsSliceThickness = radarSceneView_->getRCSSliceThickness();
    appSettings_->scene.rcsPlaneShowFill = radarSceneView_->isRCSPlaneShowFill();
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
    // Convert degrees to half-degrees and reverse for slider
    thetaSlider_->setValue(static_cast<int>((359.0 - appSettings_->scene.radarTheta) * 2));
    thetaSpinBox_->setValue(static_cast<double>(appSettings_->scene.radarTheta));
    thetaSlider_->blockSignals(false);
    thetaSpinBox_->blockSignals(false);

    phiSlider_->blockSignals(true);
    phiSpinBox_->blockSignals(true);
    // Convert degrees to half-degrees for slider
    phiSlider_->setValue(static_cast<int>(appSettings_->scene.radarPhi * 2));
    phiSpinBox_->setValue(static_cast<double>(appSettings_->scene.radarPhi));
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

        // Update target UI controls (slider values are 2x actual values for 0.5 increments)
        targetPosXSlider_->blockSignals(true);
        targetPosXSpinBox_->blockSignals(true);
        targetPosXSlider_->setValue(static_cast<int>(appSettings_->target.position.x() * 2));
        targetPosXSpinBox_->setValue(static_cast<double>(appSettings_->target.position.x()));
        targetPosXSlider_->blockSignals(false);
        targetPosXSpinBox_->blockSignals(false);

        targetPosYSlider_->blockSignals(true);
        targetPosYSpinBox_->blockSignals(true);
        targetPosYSlider_->setValue(static_cast<int>(appSettings_->target.position.y() * 2));
        targetPosYSpinBox_->setValue(static_cast<double>(appSettings_->target.position.y()));
        targetPosYSlider_->blockSignals(false);
        targetPosYSpinBox_->blockSignals(false);

        targetPosZSlider_->blockSignals(true);
        targetPosZSpinBox_->blockSignals(true);
        targetPosZSlider_->setValue(static_cast<int>(appSettings_->target.position.z() * 2));
        targetPosZSpinBox_->setValue(static_cast<double>(appSettings_->target.position.z()));
        targetPosZSlider_->blockSignals(false);
        targetPosZSpinBox_->blockSignals(false);

        targetPitchSlider_->blockSignals(true);
        targetPitchSpinBox_->blockSignals(true);
        targetPitchSlider_->setValue(static_cast<int>(appSettings_->target.rotation.x() * 2));
        targetPitchSpinBox_->setValue(static_cast<double>(appSettings_->target.rotation.x()));
        targetPitchSlider_->blockSignals(false);
        targetPitchSpinBox_->blockSignals(false);

        targetYawSlider_->blockSignals(true);
        targetYawSpinBox_->blockSignals(true);
        targetYawSlider_->setValue(static_cast<int>(appSettings_->target.rotation.y() * 2));
        targetYawSpinBox_->setValue(static_cast<double>(appSettings_->target.rotation.y()));
        targetYawSlider_->blockSignals(false);
        targetYawSpinBox_->blockSignals(false);

        targetRollSlider_->blockSignals(true);
        targetRollSpinBox_->blockSignals(true);
        targetRollSlider_->setValue(static_cast<int>(appSettings_->target.rotation.z() * 2));
        targetRollSpinBox_->setValue(static_cast<double>(appSettings_->target.rotation.z()));
        targetRollSlider_->blockSignals(false);
        targetRollSpinBox_->blockSignals(false);

        targetScaleSlider_->blockSignals(true);
        targetScaleSpinBox_->blockSignals(true);
        targetScaleSlider_->setValue(static_cast<int>(appSettings_->target.scale * 2));
        targetScaleSpinBox_->setValue(static_cast<double>(appSettings_->target.scale));
        targetScaleSlider_->blockSignals(false);
        targetScaleSpinBox_->blockSignals(false);

        // Apply target type
        target->setTargetType(static_cast<WireframeType>(appSettings_->target.targetType));
    }

    // Apply beam settings
    if (auto* beam = radarSceneView_->getBeamController()) {
        beam->setBeamType(static_cast<BeamType>(appSettings_->beam.beamType));
        beam->setBeamWidth(appSettings_->beam.beamWidth);
        beam->setBeamOpacity(appSettings_->beam.opacity);
        beam->setFootprintOnly(appSettings_->beam.footprintOnly);
    }

    // Apply visibility settings
    radarSceneView_->setAxesVisible(appSettings_->scene.showAxes);
    radarSceneView_->setSphereVisible(appSettings_->scene.showSphere);
    radarSceneView_->setGridLinesVisible(appSettings_->scene.showGrid);
    radarSceneView_->setShowShadow(appSettings_->scene.showShadow);

    // Apply RCS plane settings
    radarSceneView_->setRCSCutType(static_cast<CutType>(appSettings_->scene.rcsCutType));
    radarSceneView_->setRCSPlaneOffset(appSettings_->scene.rcsPlaneOffset);
    radarSceneView_->setRCSSliceThickness(appSettings_->scene.rcsSliceThickness);

    // Update RCS plane UI controls
    if (rcsCutTypeComboBox_) {
        rcsCutTypeComboBox_->blockSignals(true);
        rcsCutTypeComboBox_->setCurrentIndex(appSettings_->scene.rcsCutType);
        rcsCutTypeComboBox_->blockSignals(false);
    }
    if (rcsPlaneOffsetSlider_ && rcsPlaneOffsetSpinBox_) {
        rcsPlaneOffsetSlider_->blockSignals(true);
        rcsPlaneOffsetSpinBox_->blockSignals(true);
        rcsPlaneOffsetSlider_->setValue(static_cast<int>(appSettings_->scene.rcsPlaneOffset));
        rcsPlaneOffsetSpinBox_->setValue(static_cast<int>(appSettings_->scene.rcsPlaneOffset));
        rcsPlaneOffsetSlider_->blockSignals(false);
        rcsPlaneOffsetSpinBox_->blockSignals(false);
    }
    if (rcsSliceThicknessSlider_ && rcsSliceThicknessSpinBox_) {
        // Find the closest index in the thickness scale for the saved thickness
        int thicknessIndex = findClosestThicknessIndex(appSettings_->scene.rcsSliceThickness);
        float actualThickness = RS::Constants::kThicknessScale[thicknessIndex];

        rcsSliceThicknessSlider_->blockSignals(true);
        rcsSliceThicknessSpinBox_->blockSignals(true);
        rcsSliceThicknessSlider_->setValue(thicknessIndex);
        rcsSliceThicknessSpinBox_->setValue(static_cast<double>(actualThickness));
        rcsSliceThicknessSlider_->blockSignals(false);
        rcsSliceThicknessSpinBox_->blockSignals(false);
    }

    // Apply RCS plane show fill setting
    radarSceneView_->setRCSPlaneShowFill(appSettings_->scene.rcsPlaneShowFill);
    if (rcsPlaneShowFillCheckBox_) {
        rcsPlaneShowFillCheckBox_->blockSignals(true);
        rcsPlaneShowFillCheckBox_->setChecked(appSettings_->scene.rcsPlaneShowFill);
        rcsPlaneShowFillCheckBox_->blockSignals(false);
    }

    // Sync ConfigurationWindow checkboxes with current scene state
    syncConfigWindowState();

    radarSceneView_->updateScene();
}

// Pop-out window slot implementations
void RadarSim::onScenePopoutRequested() {
    if (scenePopOut_) {
        // Already popped out - close it (dock back)
        scenePopOut_->close();
        return;
    }

    // Enable FBO rendering on the radar scene widget
    if (auto* glWidget = radarSceneView_->getGLWidget()) {
        glWidget->setRenderToFBO(true);

        // Create pop-out window
        scenePopOut_ = new PopOutWindow(PopOutWindow::Type::RadarScene);
        scenePopOut_->setWindowTitle("3D Radar Scene");
        scenePopOut_->setSourceRadarWidget(glWidget);
        scenePopOut_->resize(1024, 768);

        connect(scenePopOut_, &PopOutWindow::windowClosed,
                this, &RadarSim::onScenePopoutClosed);

        scenePopOut_->show();
    }
}

void RadarSim::onPolarPopoutRequested() {
    if (polarPopOut_) {
        // Already popped out - close it (dock back)
        polarPopOut_->close();
        return;
    }

    // Create pop-out window with new PolarRCSPlot instance
    polarPopOut_ = new PopOutWindow(PopOutWindow::Type::PolarPlot);
    polarPopOut_->setWindowTitle("2D Polar RCS Plot");
    polarPopOut_->setSourcePolarPlot(polarRCSPlot_, radarSceneView_);
    polarPopOut_->resize(800, 600);

    connect(polarPopOut_, &PopOutWindow::windowClosed,
            this, &RadarSim::onPolarPopoutClosed);

    polarPopOut_->show();
}

void RadarSim::onScenePopoutClosed() {
    // Disable FBO rendering
    if (auto* glWidget = radarSceneView_->getGLWidget()) {
        glWidget->setRenderToFBO(false);
    }

    scenePopOut_->deleteLater();
    scenePopOut_ = nullptr;
}

void RadarSim::onPolarPopoutClosed() {
    polarPopOut_->deleteLater();
    polarPopOut_ = nullptr;
}