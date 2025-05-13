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

void BeamController::initialize() {
    qDebug() << "Initializing BeamController";

    // Create initial beam
    createBeam();

    qDebug() << "BeamController initialization complete";
}

void BeamController::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    if (radarBeam_ && showBeam_) {
        radarBeam_->render(nullptr, projection, view, model);
    }
}

void BeamController::setBeamType(BeamType type) {
    if (currentBeamType_ != type) {
        currentBeamType_ = type;

        // Re-create beam with new type
        createBeam();

        emit beamTypeChanged(type);
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

void BeamController::updateBeamPosition(const QVector3D& position) {
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
    }
}