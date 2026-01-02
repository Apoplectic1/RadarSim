// TargetControlsWidget.h - Self-contained target control panel widget
#pragma once

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QVector3D>

namespace RSConfig {
    struct TargetConfig;
}

class TargetControlsWidget : public QWidget {
    Q_OBJECT

public:
    explicit TargetControlsWidget(QWidget* parent = nullptr);
    ~TargetControlsWidget() override = default;

    // Settings persistence
    void readSettings(RSConfig::TargetConfig& config) const;
    void applySettings(const RSConfig::TargetConfig& config);

    // Current values
    QVector3D getPosition() const;
    QVector3D getRotation() const;
    float getScale() const;

signals:
    void positionChanged(float x, float y, float z);
    void rotationChanged(float pitch, float yaw, float roll);
    void scaleChanged(float scale);

public slots:
    void setPosition(float x, float y, float z);
    void setRotation(float pitch, float yaw, float roll);
    void setScale(float scale);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    // Position slots
    void onPosXSliderChanged(int value);
    void onPosXSpinBoxChanged(double value);
    void onPosYSliderChanged(int value);
    void onPosYSpinBoxChanged(double value);
    void onPosZSliderChanged(int value);
    void onPosZSpinBoxChanged(double value);
    // Rotation slots
    void onPitchSliderChanged(int value);
    void onPitchSpinBoxChanged(double value);
    void onYawSliderChanged(int value);
    void onYawSpinBoxChanged(double value);
    void onRollSliderChanged(int value);
    void onRollSpinBoxChanged(double value);
    // Scale slot
    void onScaleSliderChanged(int value);
    void onScaleSpinBoxChanged(double value);

private:
    void setupUI();
    void connectSignals();

    // Position controls
    QSlider* posXSlider_ = nullptr;
    QDoubleSpinBox* posXSpinBox_ = nullptr;
    QSlider* posYSlider_ = nullptr;
    QDoubleSpinBox* posYSpinBox_ = nullptr;
    QSlider* posZSlider_ = nullptr;
    QDoubleSpinBox* posZSpinBox_ = nullptr;

    // Rotation controls
    QSlider* pitchSlider_ = nullptr;
    QDoubleSpinBox* pitchSpinBox_ = nullptr;
    QSlider* yawSlider_ = nullptr;
    QDoubleSpinBox* yawSpinBox_ = nullptr;
    QSlider* rollSlider_ = nullptr;
    QDoubleSpinBox* rollSpinBox_ = nullptr;

    // Scale control
    QSlider* scaleSlider_ = nullptr;
    QDoubleSpinBox* scaleSpinBox_ = nullptr;
};
