// RadarSceneWidget.cpp - Updated to use components
#include "RadarSceneWidget.h"
#include <QDebug>

RadarSceneWidget::RadarSceneWidget(QWidget* parent)
    : QWidget(parent),
    sphereWidget_(nullptr),
    radarGLWidget_(nullptr),
    layout_(new QVBoxLayout(this)),
    sphereRenderer_(nullptr),
    beamController_(nullptr),
    cameraController_(nullptr),
    modelManager_(nullptr),
    useComponents_(false)
{
    qDebug() << "Creating RadarSceneWidget";

    // Set up layout properties
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    // Create the SphereWidget and add it to the layout
    sphereWidget_ = new SphereWidget(this);
    layout_->addWidget(sphereWidget_);

    // Create the RadarGLWidget but don't add it to layout yet
    radarGLWidget_ = new RadarGLWidget(this);

    // Create components (but don't initialize OpenGL yet)
    createComponents();

    // Connect signals from RadarGLWidget
    connect(radarGLWidget_, &RadarGLWidget::radiusChanged,
        this, &RadarSceneWidget::onRadiusChanged);
    connect(radarGLWidget_, &RadarGLWidget::anglesChanged,
        this, &RadarSceneWidget::onAnglesChanged);

    // Set the layout
    setLayout(layout_);

    qDebug() << "RadarSceneWidget constructor complete";
}

RadarSceneWidget::~RadarSceneWidget() {
    qDebug() << "RadarSceneWidget destructor called";

    // Clean up components
    delete sphereRenderer_;
    delete beamController_;
    delete cameraController_;
    delete modelManager_;
}

void RadarSceneWidget::createComponents() {
    qDebug() << "Creating RadarSceneWidget components";

    // Just create the component objects
    sphereRenderer_ = new SphereRenderer(this);
    beamController_ = new BeamController(this);
    cameraController_ = new CameraController(this);
    modelManager_ = new ModelManager(this);

    // Associate with the GL widget
    radarGLWidget_->initialize(sphereRenderer_, beamController_,
        cameraController_, modelManager_);

    qDebug() << "RadarSceneWidget components created";
}

void RadarSceneWidget::initializeComponents() {
    qDebug() << "Initializing RadarSceneWidget components";

    // Create components
    sphereRenderer_ = new SphereRenderer(this);
    beamController_ = new BeamController(this);
    cameraController_ = new CameraController(this);
    modelManager_ = new ModelManager(this);

    // Initialize the RadarGLWidget with components
    radarGLWidget_->initialize(sphereRenderer_, beamController_,
        cameraController_, modelManager_);

    qDebug() << "RadarSceneWidget components created";
}

void RadarSceneWidget::enableComponentRendering(bool enable) {
    if (useComponents_ == enable) {
        return; // No change
    }

    try {
        // Make sure components exist and GL widget is ready
        if (enable && (!sphereRenderer_ || !beamController_ ||
            !cameraController_ || !modelManager_ || !radarGLWidget_)) {
            qWarning() << "Can't enable component rendering - not all components ready";
            return;
        }

        useComponents_ = enable;

        // Remove the current widget from layout
        if (useComponents_) {
            layout_->removeWidget(sphereWidget_);
            sphereWidget_->hide();

            // Add GL widget to layout
            layout_->addWidget(radarGLWidget_);
            radarGLWidget_->show();

            qDebug() << "Switched to component-based rendering";
        }
        else {
            layout_->removeWidget(radarGLWidget_);
            radarGLWidget_->hide();

            layout_->addWidget(sphereWidget_);
            sphereWidget_->show();

            qDebug() << "Switched to SphereWidget rendering";
        }

        // Ensure the widget is updated
        update();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in enableComponentRendering:" << e.what();
        useComponents_ = false; // Revert to safe state
    }
    catch (...) {
        qCritical() << "Unknown exception in enableComponentRendering";
        useComponents_ = false; // Revert to safe state
    }
}

// Slot handlers for RadarGLWidget signals
void RadarSceneWidget::onRadiusChanged(float radius) {
    emit radarPositionChanged(radius, getTheta(), getPhi());
}

void RadarSceneWidget::onAnglesChanged(float theta, float phi) {
    emit radarPositionChanged(sphereWidget_->getRadius(), theta, phi);
}

// --- Forwarding methods with component support --- //

void RadarSceneWidget::setRadius(float radius) {
    if (useComponents_) {
        radarGLWidget_->setRadius(radius);
    }
    else {
        sphereWidget_->setRadius(radius);
    }
    emit radarPositionChanged(radius, getTheta(), getPhi());
}

void RadarSceneWidget::setAngles(float theta, float phi) {
    if (useComponents_) {
        radarGLWidget_->setAngles(theta, phi);
    }
    else {
        sphereWidget_->setAngles(theta, phi);
    }
    emit radarPositionChanged(getTheta(), theta, phi);
}

float RadarSceneWidget::getTheta() const {
    return useComponents_ ? radarGLWidget_->getTheta() : sphereWidget_->getTheta();
}

float RadarSceneWidget::getPhi() const {
    return useComponents_ ? radarGLWidget_->getPhi() : sphereWidget_->getPhi();
}

void RadarSceneWidget::setBeamWidth(float degrees) {
    if (useComponents_) {
        if (beamController_) {
            beamController_->setBeamWidth(degrees);
            radarGLWidget_->update();
        }
    }
    else {
        sphereWidget_->setBeamWidth(degrees);
    }
    emit beamWidthChanged(degrees);
}

void RadarSceneWidget::setBeamType(BeamType type) {
    if (useComponents_) {
        if (beamController_) {
            beamController_->setBeamType(type);
            radarGLWidget_->update();
        }
    }
    else {
        sphereWidget_->setBeamType(type);
    }
    emit beamTypeChanged(type);
}

void RadarSceneWidget::setBeamColor(const QVector3D& color) {
    if (useComponents_) {
        if (beamController_) {
            beamController_->setBeamColor(color);
            radarGLWidget_->update();
        }
    }
    else {
        sphereWidget_->setBeamColor(color);
    }
}

void RadarSceneWidget::setBeamOpacity(float opacity) {
    if (useComponents_) {
        if (beamController_) {
            beamController_->setBeamOpacity(opacity);
            radarGLWidget_->update();
        }
    }
    else {
        sphereWidget_->setBeamOpacity(opacity);
    }
}

RadarBeam* RadarSceneWidget::getBeam() const {
    if (useComponents_) {
        return beamController_ ? beamController_->getBeam() : nullptr;
    }
    else {
        return sphereWidget_ ? sphereWidget_->getBeam() : nullptr;
    }
}

void RadarSceneWidget::setSphereVisible(bool visible) {
    if (useComponents_) {
        if (sphereRenderer_) {
            sphereRenderer_->setSphereVisible(visible);
            radarGLWidget_->update();
        }
    }
    else {
        sphereWidget_->setSphereVisible(visible);
    }
    emit visibilityOptionChanged("sphere", visible);
}

void RadarSceneWidget::setAxesVisible(bool visible) {
    if (useComponents_) {
        if (sphereRenderer_) {
            sphereRenderer_->setAxesVisible(visible);
            radarGLWidget_->update();
        }
    }
    else {
        sphereWidget_->setAxesVisible(visible);
    }
    emit visibilityOptionChanged("axes", visible);
}

void RadarSceneWidget::setGridLinesVisible(bool visible) {
    if (useComponents_) {
        if (sphereRenderer_) {
            sphereRenderer_->setGridLinesVisible(visible);
            radarGLWidget_->update();
        }
    }
    else {
        sphereWidget_->setGridLinesVisible(visible);
    }
    emit visibilityOptionChanged("gridLines", visible);
}

void RadarSceneWidget::setInertiaEnabled(bool enabled) {
    if (useComponents_) {
        if (cameraController_) {
            cameraController_->setInertiaEnabled(enabled);
        }
    }
    else {
        sphereWidget_->setInertiaEnabled(enabled);
    }
    emit visibilityOptionChanged("inertia", enabled);
}

bool RadarSceneWidget::isSphereVisible() const {
    if (useComponents_) {
        return sphereRenderer_ ? sphereRenderer_->isSphereVisible() : false;
    }
    else {
        return sphereWidget_ ? sphereWidget_->isSphereVisible() : false;
    }
}

bool RadarSceneWidget::areAxesVisible() const {
    if (useComponents_) {
        return sphereRenderer_ ? sphereRenderer_->areAxesVisible() : false;
    }
    else {
        return sphereWidget_ ? sphereWidget_->areAxesVisible() : false;
    }
}

bool RadarSceneWidget::areGridLinesVisible() const {
    if (useComponents_) {
        return sphereRenderer_ ? sphereRenderer_->areGridLinesVisible() : false;
    }
    else {
        return sphereWidget_ ? sphereWidget_->areGridLinesVisible() : false;
    }
}

bool RadarSceneWidget::isInertiaEnabled() const {
    if (useComponents_) {
        return cameraController_ ? cameraController_->isInertiaEnabled() : false;
    }
    else {
        return sphereWidget_ ? sphereWidget_->isInertiaEnabled() : false;
    }
}