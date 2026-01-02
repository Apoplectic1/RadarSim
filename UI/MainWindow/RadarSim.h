// ---- RadarSim.h ----

#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QComboBox>
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include "RadarSceneWidget.h"
#include "AppSettings.h"
#include "PopOutWindow.h"
#include "ConfigurationWindow.h"
#include "ControlsWindow.h"
#include "RadarControlsWidget.h"
#include "TargetControlsWidget.h"
#include "RCSPlaneControlsWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RadarSim; }
QT_END_NAMESPACE

class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QVBoxLayout;
class QFrame;
class QLabel;
class RadarSceneWidget;

class PolarRCSPlot;

class RadarSim : public QMainWindow {
    Q_OBJECT
public:
    explicit RadarSim(QWidget* parent = nullptr);
    ~RadarSim();

private slots:
    // Radar control slot (from widget)
    void onRadarRadiusChanged(int radius);
    void onRadarAnglesChanged(float theta, float phi);

    // Target control slots (from widget)
    void onTargetPositionChanged(float x, float y, float z);
    void onTargetRotationChanged(float pitch, float yaw, float roll);
    void onTargetScaleChanged(float scale);

    // RCS plane control slots (from widget)
    void onRCSCutTypeChanged(CutType type);
    void onRCSPlaneOffsetChanged(float degrees);
    void onRCSSliceThicknessChanged(float degrees);
    void onRCSPlaneShowFillChanged(bool show);

    // Profile management slots
    void onProfileSelected(int index);
    void onSaveProfile();
    void onSaveProfileAs();
    void onDeleteProfile();
    void onResetToDefaults();
    void onProfilesChanged();

    // Pop-out window slots
    void onScenePopoutRequested();
    void onPolarPopoutRequested();
    void onScenePopoutClosed();
    void onPolarPopoutClosed();

    // Configuration window slots
    void onShowConfigurationWindow();
    void onShowControlsWindow();
    void onAxesVisibilityChanged(bool visible);
    void onSphereVisibilityChanged(bool visible);
    void onGridVisibilityChanged(bool visible);
    void onInertiaChanged(bool enabled);
    void onReflectionLobesChanged(bool visible);
    void onHeatMapChanged(bool visible);
    void onBeamVisibilityChanged(bool visible);
    void onBeamShadowChanged(bool visible);
    void onBeamTypeChanged(BeamType type);
    void onTargetVisibilityChanged(bool visible);
    void onTargetTypeChanged(WireframeType type);

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void positionConfigWindow();
    void positionControlsWindow();
    void setupControlsWindow();
    void setupLayout();
    void setupMenuBar();
    void setupConfigurationWindow();
    void connectSignals();

    // Layout setup helper functions
    void setupSceneDocks();
    void setupControlsPanel();

    // UI helper functions
    void syncConfigWindowState();

    RadarSceneWidget* radarSceneView_;
    PolarRCSPlot* polarRCSPlot_;  // 2D polar RCS plot widget
    QSplitter* radarSplitter_ = nullptr;

    // Control widgets
    RadarControlsWidget* radarControls_ = nullptr;
    TargetControlsWidget* targetControls_ = nullptr;
    RCSPlaneControlsWidget* rcsPlaneControls_ = nullptr;

    // Settings and profile management
    RSConfig::AppSettings* appSettings_;

    // Helper methods
    void readSettingsFromScene();
    void applySettingsToScene();
    void refreshProfileList();

    // Dock widgets for scene views (left side)
    QDockWidget* sceneDock_ = nullptr;
    QDockWidget* polarDock_ = nullptr;

    // Fixed controls panel (right side, not dockable)
    QWidget* controlsPanel_ = nullptr;

    // Menu bar
    QMenu* viewMenu_ = nullptr;
    QAction* showConfigWindowAction_ = nullptr;
    QAction* showControlsWindowAction_ = nullptr;

    // Floating windows
    ConfigurationWindow* configWindow_ = nullptr;
    ControlsWindow* controlsWindow_ = nullptr;

    // Pop-out windows
    PopOutWindow* scenePopOut_ = nullptr;
    PopOutWindow* polarPopOut_ = nullptr;
};