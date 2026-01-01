// BeamController.cpp
#include "BeamController.h"
#include "Constants.h"

using namespace RS::Constants;

BeamController::BeamController(QObject* parent)
    : QObject(parent),
    currentBeamType_(BeamType::Sinc),
    sphereRadius_(Defaults::kSphereRadius),
    showBeam_(true)
{
}

BeamController::~BeamController() = default;

void BeamController::cleanup() {
    if (radarBeam_) {
        radarBeam_->cleanup();
    }
}

void BeamController::rebuildBeamGeometry() {
    // Handle deferred beam type change (requires GL context for proper cleanup)
    if (beamTypeChangePending_) {
        currentBeamType_ = pendingBeamType_;
        beamTypeChangePending_ = false;

        // Re-create beam with new type (now we have GL context)
        createBeam();

        emit beamTypeChanged(currentBeamType_);
    }

    if (radarBeam_) {
        // This must run with a valid GL context (we're in paintGL)
        radarBeam_->uploadGeometryToGPU();
    }
}
void BeamController::initialize() {
    createBeam();
}

void BeamController::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    if (radarBeam_ && showBeam_) {
        // Ensure geometry exists before rendering
        if (!currentPosition_.isNull() && radarBeam_->getVertices().empty()) {
            radarBeam_->update(currentPosition_);
        }
        radarBeam_->render(nullptr, projection, view, model);
    }
}

void BeamController::setBeamType(BeamType type) {
    if (currentBeamType_ != type) {
        // Defer the actual beam creation to when we have a GL context
        pendingBeamType_ = type;
        beamTypeChangePending_ = true;
        // Don't emit signal yet - wait until beam is actually created
    }
}

BeamType BeamController::getBeamType() const {
    // Return pending type if a change is waiting for GL context
    return beamTypeChangePending_ ? pendingBeamType_ : currentBeamType_;
}

void BeamController::setBeamWidth(float degrees) {
    if (radarBeam_) {
        float currentWidth = radarBeam_->getBeamWidth();
        if (currentWidth != degrees) {
            radarBeam_->setBeamWidth(degrees);
            emit beamWidthChanged(degrees);
        }
    }
}

float BeamController::getBeamWidth() const {
    return radarBeam_ ? radarBeam_->getBeamWidth() : Defaults::kBeamWidth;
}

float BeamController::getVisualExtentDegrees() const {
    if (!radarBeam_) return Defaults::kBeamWidth;
    return radarBeam_->getBeamWidth() * radarBeam_->getVisualExtentMultiplier();
}

void BeamController::setBeamColor(const QVector3D& color) {
    if (radarBeam_) {
        QVector3D currentColor = radarBeam_->getColor();
        if (currentColor != color) {
            radarBeam_->setColor(color);
            emit beamColorChanged(color);
        }
    }
}

QVector3D BeamController::getBeamColor() const {
    return radarBeam_ ? radarBeam_->getColor() : QVector3D(Colors::kBeamOrange[0], Colors::kBeamOrange[1], Colors::kBeamOrange[2]);
}

void BeamController::setBeamOpacity(float opacity) {
    if (radarBeam_) {
        float currentOpacity = radarBeam_->getOpacity();
        if (currentOpacity != opacity) {
            radarBeam_->setOpacity(opacity);
            emit beamOpacityChanged(opacity);
        }
    }
}

float BeamController::getBeamOpacity() const {
    return radarBeam_ ? radarBeam_->getOpacity() : Defaults::kBeamOpacity;
}

void BeamController::setSphereRadius(float radius) {
    if (sphereRadius_ != radius) {
        sphereRadius_ = radius;

        // Update the beam's sphere radius (only regenerates geometry, no GPU upload)
        if (radarBeam_) {
            radarBeam_->setSphereRadius(radius);
        }
    }
}

void BeamController::setBeamVisible(bool visible) {
    if (showBeam_ != visible) {
        showBeam_ = visible;
        if (radarBeam_) {
            radarBeam_->setVisible(visible);
        }
        emit beamVisibilityChanged(visible);
    }
}

bool BeamController::isBeamVisible() const {
    return showBeam_;
}

void BeamController::setFootprintOnly(bool footprintOnly) {
    if (radarBeam_) {
        radarBeam_->setFootprintOnly(footprintOnly);
    }
}

bool BeamController::isFootprintOnly() const {
    return radarBeam_ ? radarBeam_->isFootprintOnly() : false;
}

void BeamController::setShowShadow(bool show) {
    if (radarBeam_) {
        radarBeam_->setShowShadow(show);
    }
}

bool BeamController::isShowShadow() const {
    return radarBeam_ ? radarBeam_->isShowShadow() : true;
}

void BeamController::updateBeamPosition(const QVector3D& position) {
    currentPosition_ = position;  // Store position for beam recreation
    if (radarBeam_) {
        radarBeam_->update(position);
    }
}

void BeamController::setGPUShadowMap(GLuint textureId) {
    if (radarBeam_) {
        radarBeam_->setGPUShadowMap(textureId);
    }
}

void BeamController::setGPUShadowEnabled(bool enabled) {
    if (radarBeam_) {
        radarBeam_->setGPUShadowEnabled(enabled);
    }
}

void BeamController::setBeamAxis(const QVector3D& axis) {
    if (radarBeam_) {
        radarBeam_->setBeamAxis(axis);
    }
}

void BeamController::setBeamWidthRadians(float radians) {
    if (radarBeam_) {
        radarBeam_->setBeamWidthRadians(radians);
    }
}

void BeamController::setNumRings(int numRings) {
    if (radarBeam_) {
        radarBeam_->setNumRings(numRings);
    }
}

void BeamController::createBeam() {
    // Store current properties if beam exists
    float width = Defaults::kBeamWidth;
    QVector3D color(Colors::kBeamOrange[0], Colors::kBeamOrange[1], Colors::kBeamOrange[2]);
    float opacity = Defaults::kBeamOpacity;
    bool visible = showBeam_;

    if (radarBeam_) {
        // Save existing beam properties
        width = radarBeam_->getBeamWidth();
        color = radarBeam_->getColor();
        opacity = radarBeam_->getOpacity();
        visible = radarBeam_->isVisible();
    }

    // Create new beam (unique_ptr automatically deletes old beam)
    radarBeam_.reset(RadarBeam::createBeam(currentBeamType_, sphereRadius_, width));

    if (radarBeam_) {
        radarBeam_->initialize();
        radarBeam_->setColor(color);
        radarBeam_->setOpacity(opacity);
        radarBeam_->setVisible(visible);

        // Update beam with stored position so geometry is created
        if (!currentPosition_.isNull()) {
            radarBeam_->update(currentPosition_);
        }
    }
}