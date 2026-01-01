// RadarGLWidget.h
#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QMenu>
#include <memory>

#include "RadarSceneWidget/SphereRenderer.h"
#include "RadarBeam/BeamController.h"
#include "RadarSceneWidget/CameraController.h"
#include "ModelManager/ModelManager.h"
#include "WireframeTargetController.h"
#include "RCSCompute/RCSCompute.h"
#include "RCSCompute/RCSSampler.h"
#include "RCSCompute/AzimuthCutSampler.h"
#include "RCSCompute/ElevationCutSampler.h"
#include "RCSCompute/SlicingPlaneRenderer.h"
#include "ReflectionRenderer/ReflectionRenderer.h"
#include "ReflectionRenderer/HeatMapRenderer.h"

class FBORenderer;

class RadarGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit RadarGLWidget(QWidget* parent = nullptr);
    ~RadarGLWidget() override;

    // Initialization
    void initialize(SphereRenderer* sphereRenderer,
        BeamController* beamController,
        CameraController* cameraController,
        ModelManager* modelManager,
        WireframeTargetController* wireframeController);

    // Radar position
    void setRadius(float radius);
    void setAngles(float theta, float phi);
    float getRadius() const { return radius_; }
    float getTheta() const { return theta_; }
    float getPhi() const { return phi_; }

    // Sync context menu checkmarks with controller state
    void syncBeamMenu();

    // Shadow visibility (beam projection on sphere)
    void setShowShadow(bool show);
    bool isShowShadow() const;

    // RCS slicing plane control
    void setRCSCutType(CutType type);
    CutType getRCSCutType() const { return currentCutType_; }
    void setRCSPlaneOffset(float degrees);
    float getRCSPlaneOffset() const;
    void setRCSSliceThickness(float degrees);
    float getRCSSliceThickness() const;
    void setRCSPlaneShowFill(bool show);
    bool isRCSPlaneShowFill() const;

    // FBO rendering support (for pop-out windows)
    void setRenderToFBO(bool enable);
    bool isRenderingToFBO() const { return renderToFBO_; }
    FBORenderer* getFBORenderer() const { return fboRenderer_.get(); }

    // Camera controller access (for mouse event forwarding)
    CameraController* getCameraController() const { return cameraController_.get(); }

    // Request FBO to be resized (called by pop-out window for sharp rendering)
    void requestFBOResize(int width, int height);

    // Event handlers - override from QOpenGLWidget
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void radiusChanged(float radius);
    void anglesChanged(float theta, float phi);
    void polarPlotDataReady(const std::vector<RCSDataPoint>& data);
    void popoutRequested();

protected:
    // Override OpenGL functions
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private slots:
    // Clean up OpenGL resources before context is destroyed
    void cleanupGL();

private:
    // Radar position
    float radius_ = 100.0f;
    float theta_ = 45.0f;
    float phi_ = 45.0f;
    bool beamDirty_ = true;
    bool glCleanedUp_ = false;  // Prevent double cleanup
    bool glInitialized_ = false;  // Track if initializeGL completed successfully

    // Component ownership - deleted while GL context is still valid
    std::unique_ptr<SphereRenderer> sphereRenderer_;
    std::unique_ptr<BeamController> beamController_;
    std::unique_ptr<CameraController> cameraController_;
    std::unique_ptr<ModelManager> modelManager_;
    std::unique_ptr<WireframeTargetController> wireframeController_;

    // RCS computation (owned by this widget)
    std::unique_ptr<RCS::RCSCompute> rcsCompute_;

    // Reflection lobe visualization
    std::unique_ptr<ReflectionRenderer> reflectionRenderer_;

    // Heat map visualization
    std::unique_ptr<HeatMapRenderer> heatMapRenderer_;

    // FBO rendering for pop-out windows
    std::unique_ptr<FBORenderer> fboRenderer_;
    bool renderToFBO_ = false;

    // Slicing plane visualization
    std::unique_ptr<SlicingPlaneRenderer> slicingPlaneRenderer_;

    // RCS sampling for polar plot
    std::unique_ptr<AzimuthCutSampler> azimuthSampler_;
    std::unique_ptr<ElevationCutSampler> elevationSampler_;
    RCSSampler* currentSampler_ = nullptr;  // Points to active sampler
    CutType currentCutType_ = CutType::Azimuth;
    std::vector<RCSDataPoint> polarPlotData_;

    // Context menu
    QMenu* contextMenu_ = nullptr;

    // Beam menu actions (for syncing checked state)
    QAction* showBeamAction_ = nullptr;
    QAction* conicalBeamAction_ = nullptr;
    QAction* phasedBeamAction_ = nullptr;
    QAction* sincBeamAction_ = nullptr;

    // Visibility toggle actions (for syncing checked state)
    QAction* toggleAxesAction_ = nullptr;
    QAction* toggleSphereAction_ = nullptr;
    QAction* toggleGridAction_ = nullptr;
    QAction* showShadowAction_ = nullptr;
    QAction* toggleHeatMapAction_ = nullptr;

    // Target type menu actions (for syncing checked state)
    QAction* cubeTargetAction_ = nullptr;
    QAction* cylinderTargetAction_ = nullptr;
    QAction* aircraftTargetAction_ = nullptr;
    QAction* sphereTargetAction_ = nullptr;

    // Helper methods
    void syncBeamTypeMenuToController();
    void syncTargetTypeMenu();
    QVector3D sphericalToCartesian(float r, float thetaDeg, float phiDeg);
    void setupContextMenu();
    void updateBeamPosition();
    QPointF projectToScreen(const QVector3D& worldPos, const QMatrix4x4& projection,
                            const QMatrix4x4& view, const QMatrix4x4& model);
};