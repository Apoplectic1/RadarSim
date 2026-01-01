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
    // Radar control slots
    void onRadiusSliderValueChanged(int value);
    void onThetaSliderValueChanged(int value);
    void onPhiSliderValueChanged(int value);
    void onRadiusSpinBoxValueChanged(int value);
    void onThetaSpinBoxValueChanged(double value);
    void onPhiSpinBoxValueChanged(double value);

    // Target control slots (slider)
    void onTargetPosXSliderChanged(int value);
    void onTargetPosYSliderChanged(int value);
    void onTargetPosZSliderChanged(int value);
    void onTargetPitchSliderChanged(int value);
    void onTargetYawSliderChanged(int value);
    void onTargetRollSliderChanged(int value);
    void onTargetScaleSliderChanged(int value);
    // Target control slots (spinbox)
    void onTargetPosXSpinBoxChanged(double value);
    void onTargetPosYSpinBoxChanged(double value);
    void onTargetPosZSpinBoxChanged(double value);
    void onTargetPitchSpinBoxChanged(double value);
    void onTargetYawSpinBoxChanged(double value);
    void onTargetRollSpinBoxChanged(double value);
    void onTargetScaleSpinBoxChanged(double value);

    // RCS plane control slots
    void onRCSCutTypeChanged(int index);
    void onRCSPlaneOffsetChanged(int value);
    void onRCSSliceThicknessChanged(int value);
    void onRCSPlaneShowFillChanged(bool checked);

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
    void setupLayout();
    void setupMenuBar();
    void setupConfigurationWindow();
    void connectSignals();

    // Layout setup helper functions
    void setupSceneDocks();
    void setupControlsPanel();
    void setupRadarControls(QWidget* parent, QVBoxLayout* layout);
    void setupTargetControls(QWidget* parent, QVBoxLayout* layout);
    void setupRCSPlaneControls(QWidget* parent, QVBoxLayout* layout);

    // UI helper functions
    void syncSliderSpinBox(QSlider* slider, QSpinBox* spinBox, int value);
    void syncConfigWindowState();

    RadarSceneWidget* radarSceneView_;
    PolarRCSPlot* polarRCSPlot_;  // 2D polar RCS plot widget
    QSplitter* radarSplitter_ = nullptr;

    // Radar controls
    QSlider* radiusSlider_;
    QSlider* thetaSlider_;
    QSlider* phiSlider_;
    QSpinBox* radiusSpinBox_;
    QDoubleSpinBox* thetaSpinBox_;
    QDoubleSpinBox* phiSpinBox_;

    // Target controls
    QSlider* targetPosXSlider_;
    QSlider* targetPosYSlider_;
    QSlider* targetPosZSlider_;
    QSlider* targetPitchSlider_;
    QSlider* targetYawSlider_;
    QSlider* targetRollSlider_;
    QSlider* targetScaleSlider_;
    QDoubleSpinBox* targetPosXSpinBox_;
    QDoubleSpinBox* targetPosYSpinBox_;
    QDoubleSpinBox* targetPosZSpinBox_;
    QDoubleSpinBox* targetPitchSpinBox_;
    QDoubleSpinBox* targetYawSpinBox_;
    QDoubleSpinBox* targetRollSpinBox_;
    QDoubleSpinBox* targetScaleSpinBox_;

    // Settings and profile management
    RSConfig::AppSettings* appSettings_;

    // RCS plane controls
    QComboBox* rcsCutTypeComboBox_ = nullptr;
    QSlider* rcsPlaneOffsetSlider_ = nullptr;
    QSpinBox* rcsPlaneOffsetSpinBox_ = nullptr;
    QSlider* rcsSliceThicknessSlider_ = nullptr;
    QDoubleSpinBox* rcsSliceThicknessSpinBox_ = nullptr;
    QCheckBox* rcsPlaneShowFillCheckBox_ = nullptr;

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

    // Configuration window (floating)
    ConfigurationWindow* configWindow_ = nullptr;

    // Pop-out windows
    PopOutWindow* scenePopOut_ = nullptr;
    PopOutWindow* polarPopOut_ = nullptr;
};