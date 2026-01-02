// RCSPlaneControlsWidget.h - Self-contained RCS plane control panel widget
#pragma once

#include <QWidget>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>

// Forward declare CutType to avoid include dependency
enum class CutType;

namespace RSConfig {
    struct SceneConfig;
}

class RCSPlaneControlsWidget : public QWidget {
    Q_OBJECT

public:
    explicit RCSPlaneControlsWidget(QWidget* parent = nullptr);
    ~RCSPlaneControlsWidget() override = default;

    // Settings persistence
    void readSettings(RSConfig::SceneConfig& config) const;
    void applySettings(const RSConfig::SceneConfig& config);

    // Current values
    CutType getCutType() const;
    int getPlaneOffset() const;
    float getSliceThickness() const;
    bool isShowFillEnabled() const;

signals:
    void cutTypeChanged(CutType type);
    void planeOffsetChanged(float degrees);
    void sliceThicknessChanged(float degrees);
    void showFillChanged(bool show);

public slots:
    void setCutType(CutType type);
    void setPlaneOffset(int degrees);
    void setSliceThickness(float degrees);
    void setShowFill(bool show);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onCutTypeChanged(int index);
    void onPlaneOffsetSliderChanged(int value);
    void onPlaneOffsetSpinBoxChanged(int value);
    void onSliceThicknessSliderChanged(int index);
    void onShowFillChanged(bool checked);

private:
    void setupUI();
    void connectSignals();

    // UI elements
    QComboBox* cutTypeComboBox_ = nullptr;
    QSlider* planeOffsetSlider_ = nullptr;
    QSpinBox* planeOffsetSpinBox_ = nullptr;
    QSlider* sliceThicknessSlider_ = nullptr;
    QDoubleSpinBox* sliceThicknessSpinBox_ = nullptr;
    QCheckBox* showFillCheckBox_ = nullptr;
};
