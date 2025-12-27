// ModelManager.cpp
#include "ModelManager.h"
#include <QDebug>
#include <QQuaternion>

// We'll need to define the Model class later
class Model {
public:
    virtual ~Model() {}
    virtual void render(QOpenGLFunctions_4_3_Core* gl, QOpenGLShaderProgram* program) = 0;

    QVector3D position = QVector3D(0, 0, 0);
    QVector3D rotation = QVector3D(0, 0, 0);
    float scale = 1.0f;
};

ModelManager::ModelManager(QObject* parent)
    : QObject(parent)
{
    // Initialize shader sources
    vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    fragmentShaderSource = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;

        uniform vec3 viewPos;
        uniform vec3 lightPos;
        uniform vec3 objectColor;
        uniform sampler2D texture1;
        uniform bool useTexture;

        out vec4 FragColor;

        void main() {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
            
            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
            
            // Result
            vec3 baseColor = useTexture ? texture(texture1, TexCoord).rgb : objectColor;
            vec3 result = (ambient + diffuse + specular) * baseColor;
            FragColor = vec4(result, 1.0);
        }
    )";
}

ModelManager::~ModelManager() {
    // Clean up OpenGL resources
    if (modelShaderProgram) {
        delete modelShaderProgram;
        modelShaderProgram = nullptr;
    }

    // Clear models
    models_.clear();
}

bool ModelManager::initialize() {
    if (!initializeOpenGLFunctions()) {
        qCritical() << "ModelManager: Failed to initialize OpenGL functions!";
        return false;
    }

    // Set up OpenGL
    glEnable(GL_DEPTH_TEST);

    // Create and compile shader program
    if (!setupShaders()) {
        qCritical() << "ModelManager initialization failed - shader setup failed";
        return false;
    }

    return true;
}

bool ModelManager::loadModel(const std::string& filename, const QVector3D& position) {
    // We'll implement actual model loading in a future step
    // For now, return false to indicate failure
    return false;
}

void ModelManager::removeModel(int index) {
    if (index >= 0 && index < models_.size()) {
        models_.erase(models_.begin() + index);
        emit modelRemoved(index);
        emit modelCountChanged(models_.size());
    }
}

void ModelManager::clearAllModels() {
    models_.clear();
    emit modelCountChanged(0);
}

void ModelManager::setModelPosition(int index, const QVector3D& position) {
    if (index >= 0 && index < models_.size()) {
        models_[index]->position = position;
    }
}

void ModelManager::setModelRotation(int index, const QVector3D& eulerAngles) {
    if (index >= 0 && index < models_.size()) {
        models_[index]->rotation = eulerAngles;
    }
}

void ModelManager::setModelScale(int index, float scale) {
    if (index >= 0 && index < models_.size()) {
        models_[index]->scale = scale;
    }
}

void ModelManager::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    if (models_.empty() || !modelShaderProgram) {
        return;
    }

    // Set up shared shader parameters
    modelShaderProgram->bind();
    modelShaderProgram->setUniformValue("projection", projection);
    modelShaderProgram->setUniformValue("view", view);

    // Extract camera position from view matrix for specular calculations
    QMatrix4x4 inverseView = view.inverted();
    QVector3D cameraPos = inverseView.column(3).toVector3D();
    modelShaderProgram->setUniformValue("viewPos", cameraPos);

    // Set light position
    modelShaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

    // Render each model
    for (const auto& modelPtr : models_) {
        // Create model matrix for this specific model
        QMatrix4x4 modelMatrix = model;
        modelMatrix.translate(modelPtr->position);
        modelMatrix.rotate(QQuaternion::fromEulerAngles(modelPtr->rotation));
        modelMatrix.scale(modelPtr->scale);

        modelShaderProgram->setUniformValue("model", modelMatrix);

        // Let the model handle its specific rendering
        modelPtr->render(this, modelShaderProgram);
    }

    modelShaderProgram->release();
}

bool ModelManager::checkBeamIntersection(const QVector3D& beamOrigin, const QVector3D& beamDirection,
    QVector3D& intersectionPoint, QVector3D& surfaceNormal) {
    // We'll implement proper intersection testing in a future step
    // For now, return false to indicate no intersection
    return false;
}

bool ModelManager::setupShaders() {
    if (!vertexShaderSource || !fragmentShaderSource) {
        qCritical() << "ModelManager: Shader sources not initialized!";
        return false;
    }

    modelShaderProgram = new QOpenGLShaderProgram();

    if (!modelShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "ModelManager: Failed to compile vertex shader:" << modelShaderProgram->log();
        delete modelShaderProgram;
        modelShaderProgram = nullptr;
        return false;
    }

    if (!modelShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "ModelManager: Failed to compile fragment shader:" << modelShaderProgram->log();
        delete modelShaderProgram;
        modelShaderProgram = nullptr;
        return false;
    }

    if (!modelShaderProgram->link()) {
        qCritical() << "ModelManager: Failed to link shader program:" << modelShaderProgram->log();
        delete modelShaderProgram;
        modelShaderProgram = nullptr;
        return false;
    }

    return true;
}