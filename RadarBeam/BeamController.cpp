// BeamController.cpp
#include "BeamController.h"
#include <QDebug>

BeamController::BeamController(QObject* parent)
    : QObject(parent),
    radarBeam_(nullptr),
    currentBeamType_(BeamType::Conical),
    sphereRadius_(100.0f),
    showBeam_(true)
{
}

BeamController::~BeamController() {
    delete radarBeam_;
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
    qDebug() << "Initializing BeamController";

    // Create initial beam
    createBeam();

    qDebug() << "BeamController initialization complete";
}

void BeamController::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    if (radarBeam_) {
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
    return currentBeamType_;
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
    return radarBeam_ ? radarBeam_->getBeamWidth() : 15.0f;
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
    return radarBeam_ ? radarBeam_->getColor() : QVector3D(1.0f, 0.5f, 0.0f);
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
    return radarBeam_ ? radarBeam_->getOpacity() : 0.3f;
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
    qDebug() << "=== BeamController::setBeamVisible called ===";
    qDebug() << "  Requested visible:" << visible;
    qDebug() << "  Current showBeam_:" << showBeam_;

    if (showBeam_ != visible) {
        showBeam_ = visible;
        qDebug() << "  Changed showBeam_ to:" << showBeam_;

        if (radarBeam_) {
            radarBeam_->setVisible(visible);
            qDebug() << "  Called radarBeam_->setVisible(" << visible << ")";
            qDebug() << "  radarBeam_->isVisible() now returns:" << radarBeam_->isVisible();
        }
        else {
            qDebug() << "  ERROR: radarBeam_ is nullptr!";
        }

        emit beamVisibilityChanged(visible);
    }
    else {
        qDebug() << "  No change - already at" << visible;
    }
    qDebug() << "===========================================";
}

bool BeamController::isBeamVisible() const {
    return showBeam_;
}

void BeamController::updateBeamPosition(const QVector3D& position) {
    currentPosition_ = position;  // Store position for beam recreation
    if (radarBeam_) {
        radarBeam_->update(position);
    }
}

void BeamController::createBeam() {
    // Store current properties if beam exists
    float width = 15.0f;
    QVector3D color(1.0f, 0.5f, 0.0f);
    float opacity = 0.3f;
    bool visible = showBeam_;

    if (radarBeam_) {
        // Save existing beam properties
        width = radarBeam_->getBeamWidth();
        color = radarBeam_->getColor();
        opacity = radarBeam_->getOpacity();
        visible = radarBeam_->isVisible();

        // Delete old beam
        delete radarBeam_;
        radarBeam_ = nullptr;
    }

    // Create new beam
    radarBeam_ = RadarBeam::createBeam(currentBeamType_, sphereRadius_, width);

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