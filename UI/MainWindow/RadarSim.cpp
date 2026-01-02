// ---- RadarSim.cpp ----

#include "RadarSim.h"
#include "ControlsWindow.h"
#include "AppSettings.h"
#include "PolarRCSPlot.h"
#include "RCSSampler.h"  // For CutType enum
#include "Constants.h"

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
    polarRCSPlot_(nullptr),
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

    // Setup control widgets (title is handled by window title bar)
    radarControls_ = new RadarControlsWidget(controlsPanel_);
    controlsLayout->addWidget(radarControls_);
    targetControls_ = new TargetControlsWidget(controlsPanel_);
    controlsLayout->addWidget(targetControls_);
    rcsPlaneControls_ = new RCSPlaneControlsWidget(controlsPanel_);
    controlsLayout->addWidget(rcsPlaneControls_);
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

void RadarSim::connectSignals() {
    // Connect radar controls widget
    connect(radarControls_, &RadarControlsWidget::radiusChanged,
            this, &RadarSim::onRadarRadiusChanged);
    connect(radarControls_, &RadarControlsWidget::anglesChanged,
            this, &RadarSim::onRadarAnglesChanged);

    // Connect target controls widget
    connect(targetControls_, &TargetControlsWidget::positionChanged,
            this, &RadarSim::onTargetPositionChanged);
    connect(targetControls_, &TargetControlsWidget::rotationChanged,
            this, &RadarSim::onTargetRotationChanged);
    connect(targetControls_, &TargetControlsWidget::scaleChanged,
            this, &RadarSim::onTargetScaleChanged);

    // Connect RCS plane controls widget
    connect(rcsPlaneControls_, &RCSPlaneControlsWidget::cutTypeChanged,
            this, &RadarSim::onRCSCutTypeChanged);
    connect(rcsPlaneControls_, &RCSPlaneControlsWidget::planeOffsetChanged,
            this, &RadarSim::onRCSPlaneOffsetChanged);
    connect(rcsPlaneControls_, &RCSPlaneControlsWidget::sliceThicknessChanged,
            this, &RadarSim::onRCSSliceThicknessChanged);
    connect(rcsPlaneControls_, &RCSPlaneControlsWidget::showFillChanged,
            this, &RadarSim::onRCSPlaneShowFillChanged);

    // Connect polar plot data from radar scene
    connect(radarSceneView_, &RadarSceneWidget::polarPlotDataReady,
            polarRCSPlot_, &PolarRCSPlot::setData);

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

// Radar control slots (from RadarControlsWidget)
void RadarSim::onRadarRadiusChanged(int radius) {
    radarSceneView_->setRadius(radius);
}

void RadarSim::onRadarAnglesChanged(float theta, float phi) {
    radarSceneView_->setAngles(theta, phi);
}

// Target control slots (from TargetControlsWidget)
void RadarSim::onTargetPositionChanged(float x, float y, float z) {
    if (auto* controller = radarSceneView_->getWireframeController()) {
        controller->setPosition(x, y, z);
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetRotationChanged(float pitch, float yaw, float roll) {
    if (auto* controller = radarSceneView_->getWireframeController()) {
        controller->setRotation(pitch, yaw, roll);
        radarSceneView_->updateScene();
    }
}

void RadarSim::onTargetScaleChanged(float scale) {
    if (auto* controller = radarSceneView_->getWireframeController()) {
        controller->setScale(scale);
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

// RCS plane control slot implementations (from RCSPlaneControlsWidget)
void RadarSim::onRCSCutTypeChanged(CutType type) {
    radarSceneView_->setRCSCutType(type);
}

void RadarSim::onRCSPlaneOffsetChanged(float degrees) {
    radarSceneView_->setRCSPlaneOffset(degrees);
}

void RadarSim::onRCSSliceThicknessChanged(float degrees) {
    radarSceneView_->setRCSSliceThickness(degrees);
}

void RadarSim::onRCSPlaneShowFillChanged(bool show) {
    radarSceneView_->setRCSPlaneShowFill(show);
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

    // Close floating windows before main window
    if (controlsWindow_) controlsWindow_->close();
    if (configWindow_) configWindow_->close();

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
    // All slider double-click resets are now handled by the control widgets
    Q_UNUSED(obj);
    Q_UNUSED(event);
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

    // Read radar control settings from widget
    if (radarControls_) {
        radarControls_->readSettings(appSettings_->scene);
    }

    // Read camera settings
    if (auto* camera = radarSceneView_->getCameraController()) {
        appSettings_->camera.distance = camera->getDistance();
        appSettings_->camera.azimuth = camera->getAzimuth();
        appSettings_->camera.elevation = camera->getElevation();
        appSettings_->camera.focusPoint = camera->getFocusPoint();
        appSettings_->camera.inertiaEnabled = camera->isInertiaEnabled();
    }

    // Read target settings from widget
    if (targetControls_) {
        targetControls_->readSettings(appSettings_->target);
    }
    // Target type is managed by ConfigurationWindow, read from controller
    if (auto* target = radarSceneView_->getWireframeController()) {
        appSettings_->target.targetType = static_cast<int>(target->getTargetType());
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

    // Read RCS plane settings from widget
    if (rcsPlaneControls_) {
        rcsPlaneControls_->readSettings(appSettings_->scene);
    }
}

void RadarSim::applySettingsToScene() {
    if (!radarSceneView_) {
        return;
    }

    // Apply radar control settings to widget and scene
    if (radarControls_) {
        radarControls_->applySettings(appSettings_->scene);
        // Also update scene directly
        radarSceneView_->setRadius(static_cast<int>(appSettings_->scene.sphereRadius));
        radarSceneView_->setAngles(appSettings_->scene.radarTheta, appSettings_->scene.radarPhi);
    }

    // Apply camera settings
    if (auto* camera = radarSceneView_->getCameraController()) {
        camera->setDistance(appSettings_->camera.distance);
        camera->setAzimuth(appSettings_->camera.azimuth);
        camera->setElevation(appSettings_->camera.elevation);
        camera->setFocusPoint(appSettings_->camera.focusPoint);
        camera->setInertiaEnabled(appSettings_->camera.inertiaEnabled);
    }

    // Apply target settings to widget and scene
    if (targetControls_) {
        targetControls_->applySettings(appSettings_->target);
    }
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

    // Apply RCS plane settings to widget and scene
    if (rcsPlaneControls_) {
        rcsPlaneControls_->applySettings(appSettings_->scene);
    }
    radarSceneView_->setRCSCutType(static_cast<CutType>(appSettings_->scene.rcsCutType));
    radarSceneView_->setRCSPlaneOffset(appSettings_->scene.rcsPlaneOffset);
    radarSceneView_->setRCSSliceThickness(appSettings_->scene.rcsSliceThickness);
    radarSceneView_->setRCSPlaneShowFill(appSettings_->scene.rcsPlaneShowFill);

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