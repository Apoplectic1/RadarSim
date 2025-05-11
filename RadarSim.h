// ---- RadarSim.h ----

#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QSplitter> 
#include "SphereWidget.h"
#include "RadarSceneWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RadarSim; }
QT_END_NAMESPACE

class QSlider;
class QSpinBox;
class RadarSceneWidget;

class RadarSim : public QMainWindow {
    Q_OBJECT
public:
    explicit RadarSim(QWidget* parent = nullptr);
    ~RadarSim();

private slots:
    // Your existing slots remain
    void onRadiusSliderValueChanged(int value);
    void onThetaSliderValueChanged(int value);
    void onPhiSliderValueChanged(int value);
    void onRadiusSpinBoxValueChanged(int value);
    void onThetaSpinBoxValueChanged(int value);
    void onPhiSpinBoxValueChanged(int value);

private:
    void setupUI();
    void setupTabs();
    void connectSignals();

    // New tab management
    QTabWidget* tabWidget_;

    // Container widgets for each tab
    QWidget* configTabWidget_;
    QWidget* radarSceneTabWidget_;
    QWidget* physicsTabWidget_;
        
    RadarSceneWidget* radarSceneView_;
    QSplitter* radarSplitter_ = nullptr;

    QSlider* radiusSlider_;
    QSlider* thetaSlider_;
    QSlider* phiSlider_;
    QSpinBox* radiusSpinBox_;
    QSpinBox* thetaSpinBox_;
    QSpinBox* phiSpinBox_;
};