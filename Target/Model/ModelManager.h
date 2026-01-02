// ModelManager.h
#pragma once

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>
#include <string>
#include <string_view>
#include <memory>

// Forward declaration for model class
class Model;

class ModelManager : public QObject, protected QOpenGLFunctions_4_5_Core {
    Q_OBJECT

public:
    explicit ModelManager(QObject* parent = nullptr);
    ~ModelManager();

    // Initialization (returns false on failure)
    bool initialize();

    // Model management
    bool loadModel(const std::string& filename, const QVector3D& position = QVector3D(0, 0, 0));
    void removeModel(int index);
    void clearAllModels();

    // Model operations
    void setModelPosition(int index, const QVector3D& position);
    void setModelRotation(int index, const QVector3D& eulerAngles);
    void setModelScale(int index, float scale);

    // Rendering
    void render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model);

    // Intersection testing
    bool checkBeamIntersection(const QVector3D& beamOrigin, const QVector3D& beamDirection,
        QVector3D& intersectionPoint, QVector3D& surfaceNormal);

signals:
    void modelAdded(int index);
    void modelRemoved(int index);
    void modelCountChanged(int count);

private:
    // Shader for models
    std::unique_ptr<QOpenGLShaderProgram> modelShaderProgram_;

    // Collection of models
    std::vector<std::shared_ptr<Model>> models_;

    // Shader sources (string_view for type-safe literals)
    std::string_view vertexShaderSource_;
    std::string_view fragmentShaderSource_;

    // Helper methods
    bool setupShaders();
};