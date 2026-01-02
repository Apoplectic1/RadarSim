// RadarControlsWidget.h - Self-contained radar control panel widget
#pragma once

#include <QWidget>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>

namespace RSConfig {
    struct SceneConfig;
}

class RadarControlsWidget : public QWidget {
    Q_OBJECT

public:
    explicit RadarControlsWidget(QWidget* parent = nullptr);
    ~RadarControlsWidget() override = default;

    // Settings persistence
    void readSettings(RSConfig::SceneConfig& config) const;
    void applySettings(const RSConfig::SceneConfig& config);

    // Current values
    int getRadius() const;
    float getTheta() const;
    float getPhi() const;

signals:
    void radiusChanged(int radius);
    void anglesChanged(float theta, float phi);

public slots:
    void setRadius(int radius);
    void setAngles(float theta, float phi);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onRadiusSliderChanged(int value);
    void onRadiusSpinBoxChanged(int value);
    void onThetaSliderChanged(int value);
    void onThetaSpinBoxChanged(double value);
    void onPhiSliderChanged(int value);
    void onPhiSpinBoxChanged(double value);

private:
    void setupUI();
    void connectSignals();

    // UI elements
    QSlider* radiusSlider_ = nullptr;
    QSpinBox* radiusSpinBox_ = nullptr;
    QSlider* thetaSlider_ = nullptr;
    QDoubleSpinBox* thetaSpinBox_ = nullptr;
    QSlider* phiSlider_ = nullptr;
    QDoubleSpinBox* phiSpinBox_ = nullptr;
};
