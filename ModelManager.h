// ---- ModelManager.h ----

#pragma once

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <string>

// Forward declaration for future model classes
class Model;

class ModelManager : public QObject {
    Q_OBJECT

public:
    explicit ModelManager(QObject* parent = nullptr);
    ~ModelManager();

    // Model management
    bool loadModel(const std::string& filename);
    void removeModel(int index);
    void clearAllModels();

    // Model rendering
    void renderModels(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);

private:
    // Placeholder for future implementation
    // Will contain model management code
    std::vector<Model*> models_;
};