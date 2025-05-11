// ---- BeamController.cpp ----

#include "BeamController.h"

BeamController::BeamController(QObject* parent)
    : QObject(parent),
    radarBeam_(nullptr),
    showBeam_(true)
{
}

BeamController::~BeamController() {
    delete radarBeam_;
}

void BeamController::setBeamType(BeamType type) {
    // Will be implemented later
}

void BeamController::setBeamWidth(float degrees) {
    // Will be implemented later
}

void BeamController::setBeamColor(const QVector3D& color) {
    // Will be implemented later
}

void BeamController::setBeamOpacity(float opacity) {
    // Will be implemented later
}

void BeamController::setBeamVisible(bool visible) {
    showBeam_ = visible;
}

void BeamController::updateBeamPosition(const QVector3D& position) {
    // Will be implemented later
}