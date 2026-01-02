// ---- PolarPlot/PolarRCSPlot.h ----

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
#include <vector>
#include <memory>

// Data point for polar RCS plot - keeps angle and dBsm paired
struct RCSDataPoint {
    float angleDegrees;  // 0-360 degrees (azimuth) or -90 to +90 (elevation)
    float dBsm;          // dBsm value, kDBsmFloor if no hits
    bool valid;          // True if bin had hits, false if empty

    RCSDataPoint() : angleDegrees(0), dBsm(-60.0f), valid(false) {}
    RCSDataPoint(float angle, float db, bool v = true)
        : angleDegrees(angle), dBsm(db), valid(v) {}
};

class PolarRCSPlot : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit PolarRCSPlot(QWidget* parent = nullptr);
    ~PolarRCSPlot() override;

    // Set the RCS data to display (360 data points, one per degree)
    void setData(const std::vector<RCSDataPoint>& data);

    // Set the dBsm scale range
    void setScale(float minDBsm, float maxDBsm);

    // Get current scale
    float getMinDBsm() const { return minDBsm_; }
    float getMaxDBsm() const { return maxDBsm_; }

signals:
    void popoutRequested();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    // Rendering methods
    void drawPolarGrid();
    void drawAxisLabels();
    void drawDataCurve();

    // Shader setup
    void setupShaders();
    void setupBuffers();

    // Coordinate conversion
    QPointF polarToScreen(float angleDeg, float radius) const;
    float dBsmToRadius(float dBsm) const;

    // Data
    std::vector<RCSDataPoint> data_;
    float minDBsm_ = -40.0f;   // Minimum dBsm for display scale
    float maxDBsm_ = 20.0f;    // Maximum dBsm for display scale

    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> lineShader_;
    GLuint gridVao_ = 0;
    GLuint gridVbo_ = 0;
    GLuint dataVao_ = 0;
    GLuint dataVbo_ = 0;

    // Geometry cache
    std::vector<float> gridVertices_;
    std::vector<float> dataVertices_;
    bool gridDirty_ = true;
    bool dataDirty_ = true;

    // View parameters
    float plotCenterX_ = 0.0f;
    float plotCenterY_ = 0.0f;
    float plotRadius_ = 0.0f;  // Set in resizeGL
    int viewWidth_ = 0;
    int viewHeight_ = 0;
    bool glInitialized_ = false;  // Set after initializeGL completes

    // Constants
    static constexpr int kAngularGridLines = 12;    // Every 30 degrees
    static constexpr int kRadialGridRings = 6;      // Every 10 dB from -40 to +20
    static constexpr int kCircleSegments = 360;     // Segments for circular elements
};
