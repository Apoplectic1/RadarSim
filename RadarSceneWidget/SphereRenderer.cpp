#include "SphereRenderer.h"
#include <QtMath>

SphereRenderer::SphereRenderer(QObject* parent)
    : QObject(parent),
    radius_(100.0f),
    showSphere_(true),
    showGridLines_(true),
    showAxes_(true)
{
    // Initialize main shader sources (as before)
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

    // Initialize axes shader sources
    axesVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 ourColor;

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            ourColor = aColor;
        }
    )";

    axesFragmentShaderSource = R"(
        #version 330 core
        in vec3 ourColor;
        out vec4 FragColor;

        void main() {
            FragColor = vec4(ourColor, 1.0);
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

    if (axesShaderProgram) {
        delete axesShaderProgram;
        axesShaderProgram = nullptr;
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
        axesShaderProgram->bind();
        axesShaderProgram->setUniformValue("projection", projection);
        axesShaderProgram->setUniformValue("view", view);
        axesShaderProgram->setUniformValue("model", model);

        axesVAO.bind();
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        // Draw main axis lines
        glLineWidth(3.0f);

        // Draw X, Y, and Z axes lines
        glDrawArrays(GL_LINES, 0, 6); // All 3 axis lines

        // Draw arrowheads
        int segments = 12;
        glDrawArrays(GL_TRIANGLES, 6, segments * 3 * 3); // All 3 arrowheads

        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_LINE_SMOOTH);
        axesVAO.release();
        axesShaderProgram->release();
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

void SphereRenderer::createCoordinateAxes() {
    // Create a local vector to collect all vertices
    std::vector<float> vertices;

    float axisLength = radius_ * 1.2f;   // Make axes slightly longer than radius
    float arrowLength = radius_ * 0.06f; // Length of the conical arrowhead
    float arrowRadius = radius_ * 0.02f; // Radius of the conical arrowhead base
    int segments = 12;                   // Number of segments for the cone

    // First, create the axis lines
    // X-axis (red)
    vertices.push_back(0.0f);       // Origin x
    vertices.push_back(0.0f);       // Origin y
    vertices.push_back(0.0f);       // Origin z
    vertices.push_back(1.0f);       // Red
    vertices.push_back(0.0f);       // Green
    vertices.push_back(0.0f);       // Blue

    vertices.push_back(axisLength - arrowLength); // End x (just before the arrowhead starts)
    vertices.push_back(0.0f);                     // End y
    vertices.push_back(0.0f);                     // End z
    vertices.push_back(1.0f);                     // Red
    vertices.push_back(0.0f);                     // Green
    vertices.push_back(0.0f);                     // Blue

    // Y-axis (green)
    vertices.push_back(0.0f);       // Origin x
    vertices.push_back(0.0f);       // Origin y
    vertices.push_back(0.0f);       // Origin z
    vertices.push_back(0.0f);       // Red
    vertices.push_back(1.0f);       // Green
    vertices.push_back(0.0f);       // Blue

    vertices.push_back(0.0f);                     // End x
    vertices.push_back(axisLength - arrowLength); // End y (just before the arrowhead starts)
    vertices.push_back(0.0f);                     // End z
    vertices.push_back(0.0f);                     // Red
    vertices.push_back(1.0f);                     // Green
    vertices.push_back(0.0f);                     // Blue

    // Z-axis (blue)
    vertices.push_back(0.0f);       // Origin x
    vertices.push_back(0.0f);       // Origin y
    vertices.push_back(0.0f);       // Origin z
    vertices.push_back(0.0f);       // Red
    vertices.push_back(0.0f);       // Green
    vertices.push_back(1.0f);       // Blue

    vertices.push_back(0.0f);                     // End x
    vertices.push_back(0.0f);                     // End y
    vertices.push_back(axisLength - arrowLength); // End z (just before the arrowhead starts)
    vertices.push_back(0.0f);                     // Red
    vertices.push_back(0.0f);                     // Green
    vertices.push_back(1.0f);                     // Blue

    // Now create the conical arrowheads
    // Each cone is created by making triangles from the tip to points around the circular base

    // X-axis arrowhead (red)
    QVector3D xTip(axisLength, 0.0f, 0.0f);                     // Tip of the arrow
    QVector3D xCenter(axisLength - arrowLength, 0.0f, 0.0f);    // Center of the base
    QVector3D xNormal(-1.0f, 0.0f, 0.0f);                       // Direction from tip to base center

    // Y-axis arrowhead (green)
    QVector3D yTip(0.0f, axisLength, 0.0f);
    QVector3D yCenter(0.0f, axisLength - arrowLength, 0.0f);
    QVector3D yNormal(0.0f, -1.0f, 0.0f);

    // Z-axis arrowhead (blue)
    QVector3D zTip(0.0f, 0.0f, axisLength);
    QVector3D zCenter(0.0f, 0.0f, axisLength - arrowLength);
    QVector3D zNormal(0.0f, 0.0f, -1.0f);

    // Helper function to create a circle of points perpendicular to an axis
    auto createCirclePoints = [arrowRadius](const QVector3D& center, const QVector3D& normal, int segments) {
        std::vector<QVector3D> circlePoints;

        // Find perpendicular vectors to the normal
        QVector3D perp1, perp2;
        if (normal.z() != 0) {
            perp1 = QVector3D(1, 0, -normal.x() / normal.z()).normalized();
        }
        else if (normal.y() != 0) {
            perp1 = QVector3D(1, -normal.x() / normal.y(), 0).normalized();
        }
        else {
            perp1 = QVector3D(0, 1, 0);
        }
        perp2 = QVector3D::crossProduct(normal, perp1).normalized();

        // Create circle points
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            QVector3D point = center +
                (perp1 * cos(angle) + perp2 * sin(angle)) * arrowRadius;
            circlePoints.push_back(point);
        }

        return circlePoints;
        };

    // Create conical arrowheads using the helper function
    auto createCone = [&vertices](const QVector3D& tip, const QVector3D& color,
        const std::vector<QVector3D>& basePoints) {
            // For each segment of the cone, create a triangle 
            // from the tip to adjacent points on the base circle
            for (size_t i = 0; i < basePoints.size(); i++) {
                // First vertex: tip of the cone
                vertices.push_back(tip.x());
                vertices.push_back(tip.y());
                vertices.push_back(tip.z());
                vertices.push_back(color.x());
                vertices.push_back(color.y());
                vertices.push_back(color.z());

                // Second vertex: first base point
                vertices.push_back(basePoints[i].x());
                vertices.push_back(basePoints[i].y());
                vertices.push_back(basePoints[i].z());
                vertices.push_back(color.x());
                vertices.push_back(color.y());
                vertices.push_back(color.z());

                // Third vertex: next base point (wrapping around)
                size_t nextIdx = (i + 1) % basePoints.size();
                vertices.push_back(basePoints[nextIdx].x());
                vertices.push_back(basePoints[nextIdx].y());
                vertices.push_back(basePoints[nextIdx].z());
                vertices.push_back(color.x());
                vertices.push_back(color.y());
                vertices.push_back(color.z());
            }
        };

    // Generate base circle points for each axis
    std::vector<QVector3D> xBasePoints = createCirclePoints(xCenter, xNormal, segments);
    std::vector<QVector3D> yBasePoints = createCirclePoints(yCenter, yNormal, segments);
    std::vector<QVector3D> zBasePoints = createCirclePoints(zCenter, zNormal, segments);

    // Create the conical arrowheads
    createCone(xTip, QVector3D(1.0f, 0.0f, 0.0f), xBasePoints);
    createCone(yTip, QVector3D(0.0f, 1.0f, 0.0f), yBasePoints);
    createCone(zTip, QVector3D(0.0f, 0.0f, 1.0f), zBasePoints);

    // Now copy the local vertices to the member variable
    axesVertices = vertices;

    // Set up VAO and VBO for axes
    axesVAO.create();
    axesVAO.bind();

    axesVBO.create();
    axesVBO.bind();
    axesVBO.allocate(axesVertices.data(), axesVertices.size() * sizeof(float));

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    axesVAO.release();
}

void SphereRenderer::setAxesVisible(bool visible) {
    if (showAxes_ != visible) {
        showAxes_ = visible;
        emit visibilityChanged("axes", visible);
    }
}

void SphereRenderer::createSphere(int latDivisions, int longDivisions) {
    // Clear previous data
    sphereVertices.clear();
    sphereIndices.clear();

    // Generate vertices
    for (int lat = 0; lat <= latDivisions; lat++) {
        float phi = M_PI * float(lat) / float(latDivisions);
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        for (int lon = 0; lon <= longDivisions; lon++) {
            float theta = 2.0f * M_PI * float(lon) / float(longDivisions);
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            // Position
            float x = radius_ * sinPhi * cosTheta;
            float y = radius_ * cosPhi;
            float z = radius_ * sinPhi * sinTheta;

            // Normal (normalized position for a sphere)
            float nx = sinPhi * cosTheta;
            float ny = cosPhi;
            float nz = sinPhi * sinTheta;

            // Add vertex data
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
            sphereVertices.push_back(nx);
            sphereVertices.push_back(ny);
            sphereVertices.push_back(nz);
        }
    }

    // Generate indices
    for (int lat = 0; lat < latDivisions; lat++) {
        for (int lon = 0; lon < longDivisions; lon++) {
            int first = lat * (longDivisions + 1) + lon;
            int second = first + longDivisions + 1;

            // First triangle
            sphereIndices.push_back(first);
            sphereIndices.push_back(second);
            sphereIndices.push_back(first + 1);

            // Second triangle
            sphereIndices.push_back(second);
            sphereIndices.push_back(second + 1);
            sphereIndices.push_back(first + 1);
        }
    }

    // Set up VAO and VBO for sphere
    sphereVAO.create();
    sphereVAO.bind();

    sphereVBO.create();
    sphereVBO.bind();
    sphereVBO.allocate(sphereVertices.data(), sphereVertices.size() * sizeof(float));

    sphereEBO.create();
    sphereEBO.bind();
    sphereEBO.allocate(sphereIndices.data(), sphereIndices.size() * sizeof(unsigned int));

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    sphereVAO.release();
}

void SphereRenderer::createLatitudeLongitudeLines() {
    // Similar to before, but with a small offset to the grid lines
    latLongLines.clear();

    // Constants for grid generation
    const float degToRad = float(M_PI / 180.0f);
    const int latitudeSegments = 72;  // Higher number for smoother circles
    const int longitudeSegments = 50; // Higher number for smoother arcs

    // Add a small radius offset to ensure grid lines render above the sphere surface
    // Typically 0.1% to 1% larger is sufficient
    const float gridRadiusOffset = radius_ * 1.005f; // 0.5% larger than sphere radius

    // Track our progress in the vertex array
    int vertexOffset = 0;

    // -------------------- LATITUDE LINES --------------------
    // Reset counter for debugging
    latitudeLineCount = 0;

    // Generate latitude lines one by one
    for (int phiDeg = -75; phiDeg <= 75; phiDeg += 15) {
        // Increment counter
        latitudeLineCount++;

        // Mark equator for special rendering
        if (phiDeg == 0) {
            equatorStartIndex = vertexOffset / 3;
        }

        float phi = phiDeg * degToRad;
        float y = gridRadiusOffset * sin(phi);  // Use offset radius
        float rLat = gridRadiusOffset * cos(phi);  // Use offset radius

        // Create a complete circle for this latitude
        for (int i = 0; i <= latitudeSegments; i++) {
            float theta = 2.0f * M_PI * float(i) / float(latitudeSegments);
            float x = rLat * cos(theta);
            float z = rLat * sin(theta);

            // Add vertex
            latLongLines.push_back(x);
            latLongLines.push_back(y);
            latLongLines.push_back(z);

            vertexOffset += 3;
        }
    }

    // -------------------- LONGITUDE LINES --------------------
    // Reset counter for debugging
    longitudeLineCount = 0;

    // Generate longitude lines one by one
    for (int thetaDeg = 0; thetaDeg <= 345; thetaDeg += 15) {
        // Increment counter
        longitudeLineCount++;

        // Mark prime meridian for special rendering
        if (thetaDeg == 0) {
            primeMeridianStartIndex = vertexOffset / 3;
        }

        float theta = thetaDeg * degToRad;

        // Create a line from south pole to north pole
        for (int i = 0; i <= longitudeSegments; i++) {
            // Generate points from south pole to north pole
            float phi = M_PI * float(i) / float(longitudeSegments) - M_PI / 2.0f;
            float rLat = gridRadiusOffset * cos(phi);  // Use offset radius
            float y = gridRadiusOffset * sin(phi);  // Use offset radius
            float x = rLat * cos(theta);
            float z = rLat * sin(theta);

            // Add vertex
            latLongLines.push_back(x);
            latLongLines.push_back(y);
            latLongLines.push_back(z);

            vertexOffset += 3;
        }
    }

    // Print debug info to console
    qDebug() << "Created" << latitudeLineCount << "latitude lines";
    qDebug() << "Created" << longitudeLineCount << "longitude lines";
    qDebug() << "Equator index:" << equatorStartIndex;
    qDebug() << "Prime Meridian index:" << primeMeridianStartIndex;
    qDebug() << "Total vertices:" << latLongLines.size() / 3;

    // Set up VAO and VBO for lines
    linesVAO.create();
    linesVAO.bind();

    linesVBO.create();
    linesVBO.bind();
    linesVBO.allocate(latLongLines.data(), latLongLines.size() * sizeof(float));

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    linesVAO.release();
}

void SphereRenderer::setupShaders() {
    qDebug() << "Setting up shaders for SphereRenderer";

    // Create and compile main shader program for sphere and grid lines
    shaderProgram = new QOpenGLShaderProgram();
    if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qDebug() << "Error compiling vertex shader:" << shaderProgram->log();
    }

    if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qDebug() << "Error compiling fragment shader:" << shaderProgram->log();
    }

    if (!shaderProgram->link()) {
        qDebug() << "Error linking shader program:" << shaderProgram->log();
    }

    // Create and compile axes shader program
    axesShaderProgram = new QOpenGLShaderProgram();
    if (!axesShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, axesVertexShaderSource)) {
        qDebug() << "Error compiling axes vertex shader:" << axesShaderProgram->log();
    }

    if (!axesShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, axesFragmentShaderSource)) {
        qDebug() << "Error compiling axes fragment shader:" << axesShaderProgram->log();
    }

    if (!axesShaderProgram->link()) {
        qDebug() << "Error linking axes shader program:" << axesShaderProgram->log();
    }

    qDebug() << "Shader setup complete";
}