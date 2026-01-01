// ---- PopOutWindow.h ----
// Pop-out window container for displaying RadarGLWidget or PolarRCSPlot in a separate window
// Uses texture blit for RadarGLWidget to avoid NVIDIA driver crashes from reparenting

#pragma once

#include <QWidget>

class TextureBlitWidget;
class PolarRCSPlot;
class RadarGLWidget;
class RadarSceneWidget;
class QVBoxLayout;

class PopOutWindow : public QWidget {
    Q_OBJECT
public:
    enum class Type {
        RadarScene,  // 3D radar scene (uses texture blit)
        PolarPlot    // 2D polar RCS plot (creates new instance)
    };

    explicit PopOutWindow(Type type, QWidget* parent = nullptr);
    ~PopOutWindow();

    // For RadarScene type: set the source widget whose FBO texture we'll display
    void setSourceRadarWidget(RadarGLWidget* source);

    // For PolarPlot type: set up the polar plot with data source
    void setSourcePolarPlot(PolarRCSPlot* source, RadarSceneWidget* dataSource);

    // Get the window type
    Type getType() const { return type_; }

    // Get the contained widgets
    TextureBlitWidget* getBlitWidget() const { return blitWidget_; }
    PolarRCSPlot* getPolarPlot() const { return polarPlot_; }

signals:
    // Emitted when the window is closed (user closed or ESC pressed)
    void windowClosed();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupRadarSceneUI();
    void setupPolarPlotUI();

    Type type_;
    QVBoxLayout* layout_ = nullptr;
    TextureBlitWidget* blitWidget_ = nullptr;
    PolarRCSPlot* polarPlot_ = nullptr;
    RadarSceneWidget* dataSource_ = nullptr;
};
