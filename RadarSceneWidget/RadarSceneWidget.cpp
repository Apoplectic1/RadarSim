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
    wireframeController_(nullptr),
    useComponents_(true)  // Default to component-based rendering
{
    qDebug() << "Creating RadarSceneWidget";

    // Set up layout properties
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    // Create both widgets
    sphereWidget_ = new SphereWidget(this);
    radarGLWidget_ = new RadarGLWidget(this);

    // Create components (but don't initialize OpenGL yet)
    createComponents();

    // Connect signals from RadarGLWidget
    connect(radarGLWidget_, &RadarGLWidget::radiusChanged,
        this, &RadarSceneWidget::onRadiusChanged);
    connect(radarGLWidget_, &RadarGLWidget::anglesChanged,
        this, &RadarSceneWidget::onAnglesChanged);

    // Add the appropriate widget to the layout based on useComponents_
    if (useComponents_) {
        qDebug() << "Using component architecture by default";
        layout_->addWidget(radarGLWidget_);
        // Hide the other widget
        sphereWidget_->hide();
    }
    else {
        qDebug() << "Using traditional rendering by default";
        layout_->addWidget(sphereWidget_);
        // Hide the other widget
        radarGLWidget_->hide();
    }

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
    delete wireframeController_;
}

void RadarSceneWidget::createComponents() {
    qDebug() << "Creating RadarSceneWidget components";

    // Just create the component objects
    sphereRenderer_ = new SphereRenderer(this);
    beamController_ = new BeamController(this);
    cameraController_ = new CameraController(this);
    modelManager_ = new ModelManager(this);
    wireframeController_ = new WireframeTargetController(this);

    // Associate with the GL widget
    radarGLWidget_->initialize(sphereRenderer_, beamController_,
        cameraController_, modelManager_, wireframeController_);

    qDebug() << "RadarSceneWidget components created";
}

void RadarSceneWidget::initializeComponents() {
    qDebug() << "Initializing RadarSceneWidget components";

    // Create components
    sphereRenderer_ = new SphereRenderer(this);
    beamController_ = new BeamController(this);
    cameraController_ = new CameraController(this);
    modelManager_ = new ModelManager(this);
    wireframeController_ = new WireframeTargetController(this);

    // Initialize the RadarGLWidget with components
    radarGLWidget_->initialize(sphereRenderer_, beamController_,
        cameraController_, modelManager_, wireframeController_);

    qDebug() << "RadarSceneWidget components created";
}

void RadarSceneWidget::enableComponentRendering(bool enable) {
    if (useComponents_ == enable) {
        return; // No change
    }

    try {
        useComponents_ = enable;

        if (useComponents_) {
            qDebug() << "Switching to component-based rendering";

            // Ensure components are ready
            if (!sphereRenderer_ || !beamController_ || !cameraController_ || !modelManager_) {
                qDebug() << "Creating components for first use";
                createComponents();
            }

            // Transfer current state from SphereWidget to components
            transferStateToComponents();

            // Update layout
            layout_->removeWidget(sphereWidget_);
            sphereWidget_->hide();

            layout_->addWidget(radarGLWidget_);
            radarGLWidget_->show();

            // Force an immediate update to render with new components
            radarGLWidget_->update();
        }
        else {
            qDebug() << "Switching to SphereWidget rendering";

            // Update layout
            layout_->removeWidget(radarGLWidget_);
            radarGLWidget_->hide();

            layout_->addWidget(sphereWidget_);
            sphereWidget_->show();

            // Force an immediate update
            sphereWidget_->update();
        }

        // Update the parent widget
        update();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in enableComponentRendering:" << e.what();
        useComponents_ = false;
    }
    catch (...) {
        qCritical() << "Unknown exception in enableComponentRendering";
        useComponents_ = false;
    }
}
void RadarSceneWidget::transferStateToComponents() {
    if (!sphereWidget_ || !sphereRenderer_ || !beamController_ || !cameraController_)
        return;

    // Transfer state from SphereWidget to components

    // 1. Transfer sphere properties
    float radius = sphereWidget_->getRadius();
    sphereRenderer_->setRadius(radius);
    sphereRenderer_->setSphereVisible(sphereWidget_->isSphereVisible());
    sphereRenderer_->setGridLinesVisible(sphereWidget_->areGridLinesVisible());
    sphereRenderer_->setAxesVisible(sphereWidget_->areAxesVisible());

    // 2. Transfer camera/view properties
    // We can't directly copy rotation state, but we can reset to default
    // The user can adjust the view after switching
    cameraController_->setInertiaEnabled(sphereWidget_->isInertiaEnabled());

    // 3. Transfer beam properties
    RadarBeam* oldBeam = sphereWidget_->getBeam();
    if (oldBeam) {
        BeamType beamType = oldBeam->getBeamType();

        beamController_->setBeamType(beamType);
        beamController_->setBeamWidth(oldBeam->getBeamWidth());
        beamController_->setBeamColor(oldBeam->getColor());
        beamController_->setBeamOpacity(oldBeam->getOpacity());
        beamController_->setBeamVisible(oldBeam->isVisible());
    }

    // 4. Update radar position
    radarGLWidget_->setRadius(radius);
    radarGLWidget_->setAngles(sphereWidget_->getTheta(), sphereWidget_->getPhi());
}

// Helper function for spherical to cartesian conversion (Z-up convention)
QVector3D RadarSceneWidget::sphericalToCartesian(float r, float thetaDeg, float phiDeg) {
    const float toRad = float(M_PI / 180.0);
    float theta = thetaDeg * toRad;
    float phi = phiDeg * toRad;
    return QVector3D(
        r * cos(phi) * cos(theta),
        r * cos(phi) * sin(theta),  // Y is now horizontal
        r * sin(phi)                // Z is now vertical (elevation)
    );
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

float RadarSceneWidget::getRadius() const {
    if (useComponents_) {
        return radarGLWidget_->getRadius();
    }
    else {
        return sphereWidget_->getRadius();
    }
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