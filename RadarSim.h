// ---- RadarSim.h ----

#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QSplitter>
#include <QComboBox>
#include "RadarSceneWidget.h"
#include "AppSettings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RadarSim; }
QT_END_NAMESPACE

class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QVBoxLayout;
class QFrame;
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

    // Beam type slot
    void onBeamTypeChanged(int index);

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

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void setupTabs();
    void connectSignals();

    // Tab setup helper functions
    void setupConfigurationTab();
    void setupRadarSceneTab();
    void setupRadarControls(QWidget* parent, QVBoxLayout* layout);
    void setupTargetControls(QWidget* parent, QVBoxLayout* layout);
    void setupRCSPlaneControls(QWidget* parent, QVBoxLayout* layout);

    // UI helper functions
    void syncSliderSpinBox(QSlider* slider, QSpinBox* spinBox, int value);

    // New tab management
    QTabWidget* tabWidget_;

    // Container widgets for each tab
    QWidget* configTabWidget_;
    QWidget* radarSceneTabWidget_;

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
    QComboBox* profileComboBox_;
    QComboBox* beamTypeComboBox_ = nullptr;

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
};