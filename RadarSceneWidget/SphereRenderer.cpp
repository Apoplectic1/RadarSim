// ---- SphereRenderer.cpp ----

#include "SphereRenderer.h"

SphereRenderer::SphereRenderer(QObject* parent)
    : QObject(parent),
    radius_(100.0f),
    showSphere_(true),
    showGridLines_(true),
    showAxes_(true)
{
}

SphereRenderer::~SphereRenderer() {
}

void SphereRenderer::initialize() {
    // Will be implemented later
}

void SphereRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    // Will be implemented later
}

void SphereRenderer::setRadius(float radius) {
    radius_ = radius;
    // Will be expanded later
}

void SphereRenderer::setSphereVisible(bool visible) {
    showSphere_ = visible;
}

void SphereRenderer::setGridLinesVisible(bool visible) {
    showGridLines_ = visible;
}

void SphereRenderer::setAxesVisible(bool visible) {
    showAxes_ = visible;
}