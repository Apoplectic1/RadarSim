// ConfigurationWindow.h - Floating configuration window for display and beam settings
#pragma once

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "RadarBeam/BeamController.h"
#include "WireframeTarget/WireframeShapes.h"

class ConfigurationWindow : public QWidget {
    Q_OBJECT

public:
    explicit ConfigurationWindow(QWidget* parent = nullptr);
    ~ConfigurationWindow() override = default;

    // Profile management
    void setProfiles(const QStringList& profiles);
    void setCurrentProfile(int index);
    int currentProfileIndex() const;

    // State synchronization - call after scene is initialized
    void syncStateFromScene(bool axesVisible, bool sphereVisible, bool gridVisible,
                           bool inertiaEnabled, bool reflectionLobesVisible, bool heatMapVisible,
                           bool beamVisible, bool shadowVisible, BeamType beamType,
                           bool targetVisible, WireframeType targetType);

signals:
    // Profile signals
    void profileSelected(int index);
    void saveRequested();
    void saveAsRequested();
    void deleteRequested();
    void resetRequested();

    // Visibility signals
    void axesVisibilityChanged(bool visible);
    void sphereVisibilityChanged(bool visible);
    void gridVisibilityChanged(bool visible);
    void inertiaChanged(bool enabled);
    void reflectionLobesChanged(bool visible);
    void heatMapChanged(bool visible);

    // Beam signals
    void beamVisibilityChanged(bool visible);
    void beamShadowChanged(bool visible);
    void beamTypeChanged(BeamType type);

    // Target signals
    void targetVisibilityChanged(bool visible);
    void targetTypeChanged(WireframeType type);

private:
    void setupUI();
    QGroupBox* createProfileGroup();
    QGroupBox* createDisplayGroup();
    QGroupBox* createBeamGroup();
    QGroupBox* createVisualizationGroup();
    QGroupBox* createTargetGroup();

    // Profile controls
    QComboBox* profileComboBox_ = nullptr;
    QPushButton* saveButton_ = nullptr;
    QPushButton* saveAsButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;

    // Display options (checkboxes)
    QCheckBox* showAxesCheckBox_ = nullptr;
    QCheckBox* showSphereCheckBox_ = nullptr;
    QCheckBox* showGridCheckBox_ = nullptr;
    QCheckBox* enableInertiaCheckBox_ = nullptr;

    // Beam controls
    QCheckBox* showBeamCheckBox_ = nullptr;
    QCheckBox* showShadowCheckBox_ = nullptr;
    QComboBox* beamTypeComboBox_ = nullptr;

    // Visualization controls
    QCheckBox* showReflectionLobesCheckBox_ = nullptr;
    QCheckBox* showHeatMapCheckBox_ = nullptr;

    // Target controls
    QCheckBox* showTargetCheckBox_ = nullptr;
    QComboBox* targetTypeComboBox_ = nullptr;
};
