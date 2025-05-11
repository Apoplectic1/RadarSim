// ---- ModelManager.cpp ----

#include "ModelManager.h"

ModelManager::ModelManager(QObject* parent)
    : QObject(parent)
{
}

ModelManager::~ModelManager() {
    clearAllModels();
}

bool ModelManager::loadModel(const std::string& filename) {
    // Will be implemented later
    return false;
}

void ModelManager::removeModel(int index) {
    // Will be implemented later
}

void ModelManager::clearAllModels() {
    // Will be implemented later
}

void ModelManager::renderModels(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    // Will be implemented later
}