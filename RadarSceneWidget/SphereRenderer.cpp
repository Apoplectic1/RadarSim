#include "SphereRenderer.h"
#include <QtMath>

SphereRenderer::SphereRenderer(QObject* parent)
    : QObject(parent),
    radius_(100.0f),
    showSphere_(true),
    showGridLines_(true),
    showAxes_(true)
{
    // Initialize shader sources
    vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 color;

        out vec3 FragColor;
        out vec3 Normal;
        out vec3 FragPos;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            FragColor = color;
        }
    )";

    fragmentShaderSource = R"(
        #version 330 core
        in vec3 FragColor;
        in vec3 Normal;
        in vec3 FragPos;

        uniform vec3 lightPos;

        out vec4 outColor;

        void main() {
            // Simple lighting calculation
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
            vec3 ambient = vec3(0.3, 0.3, 0.3);
            vec3 result = (ambient + diffuse) * FragColor;
            outColor = vec4(result, 1.0);
        }
    )";
}

SphereRenderer::~SphereRenderer() {
    // Clean up OpenGL resources
    if (sphereVAO.isCreated()) {
        sphereVAO.destroy();
    }
    if (sphereVBO.isCreated()) {
        sphereVBO.destroy();
    }
    if (sphereEBO.isCreated()) {
        sphereEBO.destroy();
    }

    if (linesVAO.isCreated()) {
        linesVAO.destroy();
    }
    if (linesVBO.isCreated()) {
        linesVBO.destroy();
    }

    if (axesVAO.isCreated()) {
        axesVAO.destroy();
    }
    if (axesVBO.isCreated()) {
        axesVBO.destroy();
    }

    if (shaderProgram) {
        delete shaderProgram;
        shaderProgram = nullptr;
    }
}

void SphereRenderer::initialize() {
    qDebug() << "Initializing SphereRenderer";
    initializeOpenGLFunctions();

    // Set up OpenGL
    glEnable(GL_DEPTH_TEST);

    // Create and compile main shader program
    shaderProgram = new QOpenGLShaderProgram();
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    shaderProgram->link();

    // Create geometry
    createSphere();
    createLatitudeLongitudeLines();
    createCoordinateAxes();

    qDebug() << "SphereRenderer initialization complete";
}


void SphereRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    // Render sphere if visible
    if (showSphere_) {
        shaderProgram->bind();
        shaderProgram->setUniformValue("projection", projection);
        shaderProgram->setUniformValue("view", view);
        shaderProgram->setUniformValue("model", model);
        shaderProgram->setUniformValue("color", QVector3D(0.95f, 0.95f, 0.95f));
        shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

        sphereVAO.bind();
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        sphereVAO.release();
        shaderProgram->release();
    }

    // Render grid lines if visible
    if (showGridLines_) {
        shaderProgram->bind();
        shaderProgram->setUniformValue("projection", projection);
        shaderProgram->setUniformValue("view", view);
        shaderProgram->setUniformValue("model", model);
        shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

        linesVAO.bind();
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glDepthFunc(GL_LEQUAL);

        // Regular latitude lines
        glLineWidth(1.5f);
        shaderProgram->setUniformValue("color", QVector3D(0.25f, 0.25f, 0.25f));

        // Draw latitude lines
        for (int lat = 0; lat < latitudeLineCount; lat++) {
            int startIdx = lat * (72 + 1);
            if (startIdx == equatorStartIndex) {
                continue; // Skip equator, drawn separately
            }
            glDrawArrays(GL_LINE_STRIP, startIdx, 72 + 1);
        }

        // Draw longitude lines
        int longitudeStartOffset = latitudeLineCount * (72 + 1);
        for (int lon = 0; lon < longitudeLineCount; lon++) {
            int startIdx = longitudeStartOffset + lon * (50 + 1);
            if (startIdx == primeMeridianStartIndex) {
                continue; // Skip prime meridian, drawn separately
            }
            glDrawArrays(GL_LINE_STRIP, startIdx, 50 + 1);
        }

        // Special lines (equator and prime meridian)
        glLineWidth(3.5f);

        // Equator (green)
        shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.9f, 0.0f));
        glDrawArrays(GL_LINE_STRIP, equatorStartIndex, 72 + 1);

        // Prime Meridian (blue)
        shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 0.9f));
        glDrawArrays(GL_LINE_STRIP, primeMeridianStartIndex, 50 + 1);

        glDepthFunc(GL_LESS);
        glDisable(GL_LINE_SMOOTH);
        linesVAO.release();
        shaderProgram->release();
    }

    // Render coordinate axes if visible
    if (showAxes_) {
        shaderProgram->bind();
        shaderProgram->setUniformValue("projection", projection);
        shaderProgram->setUniformValue("view", view);
        shaderProgram->setUniformValue("model", model);
        shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

        axesVAO.bind();
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        // Draw main axis lines
        glLineWidth(3.0f);

        // X axis (red)
        shaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
        glDrawArrays(GL_LINES, 0, 2);

        // Y axis (green)
        shaderProgram->setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
        glDrawArrays(GL_LINES, 2, 2);

        // Z axis (blue)
        shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
        glDrawArrays(GL_LINES, 4, 2);

        // Draw arrowheads (assuming 12 triangles per arrowhead)
        int segments = 12;

        // X axis arrowhead
        shaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 6, segments * 3);

        // Y axis arrowhead
        shaderProgram->setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 6 + segments * 3, segments * 3);

        // Z axis arrowhead
        shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
        glDrawArrays(GL_TRIANGLES, 6 + 2 * segments * 3, segments * 3);

        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_LINE_SMOOTH);
        axesVAO.release();
        shaderProgram->release();
    }
}


void SphereRenderer::setRadius(float radius) {
    if (radius_ != radius) {
        radius_ = radius;

        // Regenerate geometry
        createSphere();
        createLatitudeLongitudeLines();
        createCoordinateAxes();

        emit radiusChanged(radius);
    }
}

void SphereRenderer::setSphereVisible(bool visible) {
    if (showSphere_ != visible) {
        showSphere_ = visible;
        emit visibilityChanged("sphere", visible);
    }
}

void SphereRenderer::setGridLinesVisible(bool visible) {
    if (showGridLines_ != visible) {
        showGridLines_ = visible;
        emit visibilityChanged("gridLines", visible);
    }
}

void SphereRenderer::setAxesVisible(bool visible) {
    if (showAxes_ != visible) {
        showAxes_ = visible;
        emit visibilityChanged("axes", visible);
    }
}
