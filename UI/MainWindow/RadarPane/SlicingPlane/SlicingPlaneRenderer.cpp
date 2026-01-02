// SlicingPlaneRenderer.cpp - Visualizes the RCS slicing plane in 3D scene
#include "SlicingPlaneRenderer.h"
#include "Constants.h"
#include <cmath>
#include <QDebug>

using namespace RS::Constants;

SlicingPlaneRenderer::SlicingPlaneRenderer(QObject* parent)
    : shaderProgram_(nullptr)
{
    Q_UNUSED(parent);
}

SlicingPlaneRenderer::~SlicingPlaneRenderer() {
    cleanup();
}

bool SlicingPlaneRenderer::initialize() {
    if (initialized_) {
        return true;
    }

    if (!initializeOpenGLFunctions()) {
        qWarning() << "SlicingPlaneRenderer: Failed to initialize OpenGL functions";
        return false;
    }

    createShaders();
    createGeometry();

    initialized_ = true;
    return true;
}

void SlicingPlaneRenderer::cleanup() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (outlineVao_ != 0) {
        glDeleteVertexArrays(1, &outlineVao_);
        outlineVao_ = 0;
    }
    if (outlineVbo_ != 0) {
        glDeleteBuffers(1, &outlineVbo_);
        outlineVbo_ = 0;
    }
    shaderProgram_.reset();
    initialized_ = false;
}

void SlicingPlaneRenderer::createShaders() {
    // Simple shader for translucent plane
    const char* vertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec3 aPos;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 450 core
        out vec4 FragColor;

        uniform vec4 planeColor;

        void main() {
            FragColor = planeColor;
        }
    )";

    shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();
    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qWarning() << "SlicingPlaneRenderer: Failed to compile vertex shader:" << shaderProgram_->log();
        return;
    }
    if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qWarning() << "SlicingPlaneRenderer: Failed to compile fragment shader:" << shaderProgram_->log();
        return;
    }
    if (!shaderProgram_->link()) {
        qWarning() << "SlicingPlaneRenderer: Failed to link shader program:" << shaderProgram_->log();
        return;
    }
}

void SlicingPlaneRenderer::createGeometry() {
    // Main plane fill geometry
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Outline geometry for thickness boundaries
    glGenVertexArrays(1, &outlineVao_);
    glGenBuffers(1, &outlineVbo_);

    glBindVertexArray(outlineVao_);
    glBindBuffer(GL_ARRAY_BUFFER, outlineVbo_);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    geometryDirty_ = true;
}

void SlicingPlaneRenderer::updateGeometry() {
    vertices_.clear();
    std::vector<float> outlineVertices;

    int azimuthSegments = 64;  // Segments around the sphere
    int elevationSegments = 8; // Segments for the thickness band

    if (cutType_ == CutType::Azimuth) {
        // Create a solid volume between the thickness boundaries
        float upperElevRad = (offset_ + thickness_) * kDegToRadF;
        float lowerElevRad = (offset_ - thickness_) * kDegToRadF;

        // Clamp to valid range
        upperElevRad = std::min(upperElevRad, kPiF / 2.0f - 0.01f);
        lowerElevRad = std::max(lowerElevRad, -kPiF / 2.0f + 0.01f);

        float zUpper = sphereRadius_ * std::sin(upperElevRad);
        float zLower = sphereRadius_ * std::sin(lowerElevRad);
        float radiusUpper = sphereRadius_ * std::cos(upperElevRad);
        float radiusLower = sphereRadius_ * std::cos(lowerElevRad);

        // 1. Create the outer curved surface (spherical band) - slightly above sphere
        float rOuter = sphereRadius_ * 1.01f;
        for (int ei = 0; ei < elevationSegments; ei++) {
            float elev1 = lowerElevRad + (upperElevRad - lowerElevRad) * (float)ei / elevationSegments;
            float elev2 = lowerElevRad + (upperElevRad - lowerElevRad) * (float)(ei + 1) / elevationSegments;

            float cosElev1 = std::cos(elev1);
            float sinElev1 = std::sin(elev1);
            float cosElev2 = std::cos(elev2);
            float sinElev2 = std::sin(elev2);

            for (int ai = 0; ai < azimuthSegments; ai++) {
                float az1 = (float)ai / azimuthSegments * kTwoPiF;
                float az2 = (float)(ai + 1) / azimuthSegments * kTwoPiF;

                float cosAz1 = std::cos(az1);
                float sinAz1 = std::sin(az1);
                float cosAz2 = std::cos(az2);
                float sinAz2 = std::sin(az2);

                // Four corners on the sphere surface
                float x00 = rOuter * cosElev1 * cosAz1;
                float y00 = rOuter * cosElev1 * sinAz1;
                float z00 = rOuter * sinElev1;

                float x10 = rOuter * cosElev1 * cosAz2;
                float y10 = rOuter * cosElev1 * sinAz2;
                float z10 = rOuter * sinElev1;

                float x01 = rOuter * cosElev2 * cosAz1;
                float y01 = rOuter * cosElev2 * sinAz1;
                float z01 = rOuter * sinElev2;

                float x11 = rOuter * cosElev2 * cosAz2;
                float y11 = rOuter * cosElev2 * sinAz2;
                float z11 = rOuter * sinElev2;

                // Two triangles for this quad
                vertices_.push_back(x00); vertices_.push_back(y00); vertices_.push_back(z00);
                vertices_.push_back(x10); vertices_.push_back(y10); vertices_.push_back(z10);
                vertices_.push_back(x11); vertices_.push_back(y11); vertices_.push_back(z11);

                vertices_.push_back(x00); vertices_.push_back(y00); vertices_.push_back(z00);
                vertices_.push_back(x11); vertices_.push_back(y11); vertices_.push_back(z11);
                vertices_.push_back(x01); vertices_.push_back(y01); vertices_.push_back(z01);
            }
        }

        // 2. Add top cap (upper disc) - fills the internal volume
        for (int i = 0; i < azimuthSegments; i++) {
            float angle1 = (float)i / azimuthSegments * kTwoPiF;
            float angle2 = (float)(i + 1) / azimuthSegments * kTwoPiF;

            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(zUpper);
            vertices_.push_back(radiusUpper * std::cos(angle1));
            vertices_.push_back(radiusUpper * std::sin(angle1));
            vertices_.push_back(zUpper);
            vertices_.push_back(radiusUpper * std::cos(angle2));
            vertices_.push_back(radiusUpper * std::sin(angle2));
            vertices_.push_back(zUpper);
        }

        // 3. Add bottom cap (lower disc) - fills the internal volume
        for (int i = 0; i < azimuthSegments; i++) {
            float angle1 = (float)i / azimuthSegments * kTwoPiF;
            float angle2 = (float)(i + 1) / azimuthSegments * kTwoPiF;

            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(zLower);
            vertices_.push_back(radiusLower * std::cos(angle2));
            vertices_.push_back(radiusLower * std::sin(angle2));
            vertices_.push_back(zLower);
            vertices_.push_back(radiusLower * std::cos(angle1));
            vertices_.push_back(radiusLower * std::sin(angle1));
            vertices_.push_back(zLower);
        }

        // Outline circles at the boundaries
        for (int i = 0; i <= azimuthSegments; i++) {
            float angle = (float)i / azimuthSegments * kTwoPiF;
            outlineVertices.push_back(radiusUpper * std::cos(angle));
            outlineVertices.push_back(radiusUpper * std::sin(angle));
            outlineVertices.push_back(zUpper);
        }

        for (int i = 0; i <= azimuthSegments; i++) {
            float angle = (float)i / azimuthSegments * kTwoPiF;
            outlineVertices.push_back(radiusLower * std::cos(angle));
            outlineVertices.push_back(radiusLower * std::sin(angle));
            outlineVertices.push_back(zLower);
        }

    } else {
        // Elevation cut: Create a spherical wedge between the two azimuth boundaries
        float upperAzimuthRad = (offset_ + thickness_) * kDegToRadF;
        float lowerAzimuthRad = (offset_ - thickness_) * kDegToRadF;

        // Slight offset to render above the sphere surface
        float rOuter = sphereRadius_ * 1.01f;

        int azimuthSteps = 4;  // Steps across the wedge thickness
        int elevationSteps = 32;  // Steps from bottom to top of sphere

        // 1. Create outer spherical wedge surface
        for (int ai = 0; ai < azimuthSteps; ai++) {
            float az1 = lowerAzimuthRad + (upperAzimuthRad - lowerAzimuthRad) * (float)ai / azimuthSteps;
            float az2 = lowerAzimuthRad + (upperAzimuthRad - lowerAzimuthRad) * (float)(ai + 1) / azimuthSteps;

            float cosAz1 = std::cos(az1);
            float sinAz1 = std::sin(az1);
            float cosAz2 = std::cos(az2);
            float sinAz2 = std::sin(az2);

            for (int ei = 0; ei < elevationSteps; ei++) {
                float elev1 = -kPiF / 2.0f + kPiF * (float)ei / elevationSteps;
                float elev2 = -kPiF / 2.0f + kPiF * (float)(ei + 1) / elevationSteps;

                float cosElev1 = std::cos(elev1);
                float sinElev1 = std::sin(elev1);
                float cosElev2 = std::cos(elev2);
                float sinElev2 = std::sin(elev2);

                // Four corners on the sphere surface
                float x00 = rOuter * cosElev1 * cosAz1;
                float y00 = rOuter * cosElev1 * sinAz1;
                float z00 = rOuter * sinElev1;

                float x10 = rOuter * cosElev1 * cosAz2;
                float y10 = rOuter * cosElev1 * sinAz2;
                float z10 = rOuter * sinElev1;

                float x01 = rOuter * cosElev2 * cosAz1;
                float y01 = rOuter * cosElev2 * sinAz1;
                float z01 = rOuter * sinElev2;

                float x11 = rOuter * cosElev2 * cosAz2;
                float y11 = rOuter * cosElev2 * sinAz2;
                float z11 = rOuter * sinElev2;

                // Two triangles for this quad
                vertices_.push_back(x00); vertices_.push_back(y00); vertices_.push_back(z00);
                vertices_.push_back(x10); vertices_.push_back(y10); vertices_.push_back(z10);
                vertices_.push_back(x11); vertices_.push_back(y11); vertices_.push_back(z11);

                vertices_.push_back(x00); vertices_.push_back(y00); vertices_.push_back(z00);
                vertices_.push_back(x11); vertices_.push_back(y11); vertices_.push_back(z11);
                vertices_.push_back(x01); vertices_.push_back(y01); vertices_.push_back(z01);
            }
        }

        // 2. Add internal vertical planes at the boundary azimuths (fills the wedge)
        float r = sphereRadius_;

        // Upper azimuth boundary plane (half-disc from center to sphere edge)
        for (int ei = 0; ei < elevationSteps; ei++) {
            float elev1 = -kPiF / 2.0f + kPiF * (float)ei / elevationSteps;
            float elev2 = -kPiF / 2.0f + kPiF * (float)(ei + 1) / elevationSteps;

            float r1 = r * std::cos(elev1);
            float z1 = r * std::sin(elev1);
            float r2 = r * std::cos(elev2);
            float z2 = r * std::sin(elev2);

            // Triangle from center to edge
            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(z1);
            vertices_.push_back(r1 * std::cos(upperAzimuthRad));
            vertices_.push_back(r1 * std::sin(upperAzimuthRad));
            vertices_.push_back(z1);
            vertices_.push_back(r2 * std::cos(upperAzimuthRad));
            vertices_.push_back(r2 * std::sin(upperAzimuthRad));
            vertices_.push_back(z2);

            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(z1);
            vertices_.push_back(r2 * std::cos(upperAzimuthRad));
            vertices_.push_back(r2 * std::sin(upperAzimuthRad));
            vertices_.push_back(z2);
            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(z2);
        }

        // Lower azimuth boundary plane
        for (int ei = 0; ei < elevationSteps; ei++) {
            float elev1 = -kPiF / 2.0f + kPiF * (float)ei / elevationSteps;
            float elev2 = -kPiF / 2.0f + kPiF * (float)(ei + 1) / elevationSteps;

            float r1 = r * std::cos(elev1);
            float z1 = r * std::sin(elev1);
            float r2 = r * std::cos(elev2);
            float z2 = r * std::sin(elev2);

            // Triangle from center to edge (opposite winding for back face)
            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(z1);
            vertices_.push_back(r2 * std::cos(lowerAzimuthRad));
            vertices_.push_back(r2 * std::sin(lowerAzimuthRad));
            vertices_.push_back(z2);
            vertices_.push_back(r1 * std::cos(lowerAzimuthRad));
            vertices_.push_back(r1 * std::sin(lowerAzimuthRad));
            vertices_.push_back(z1);

            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(z1);
            vertices_.push_back(0.0f); vertices_.push_back(0.0f); vertices_.push_back(z2);
            vertices_.push_back(r2 * std::cos(lowerAzimuthRad));
            vertices_.push_back(r2 * std::sin(lowerAzimuthRad));
            vertices_.push_back(z2);
        }

        // Outline arcs at the boundaries (great circle arcs on sphere surface)
        for (int i = 0; i <= elevationSteps; i++) {
            float elev = -kPiF / 2.0f + kPiF * (float)i / elevationSteps;
            float cosElev = std::cos(elev);
            float sinElev = std::sin(elev);
            outlineVertices.push_back(rOuter * cosElev * std::cos(upperAzimuthRad));
            outlineVertices.push_back(rOuter * cosElev * std::sin(upperAzimuthRad));
            outlineVertices.push_back(rOuter * sinElev);
        }

        for (int i = 0; i <= elevationSteps; i++) {
            float elev = -kPiF / 2.0f + kPiF * (float)i / elevationSteps;
            float cosElev = std::cos(elev);
            float sinElev = std::sin(elev);
            outlineVertices.push_back(rOuter * cosElev * std::cos(lowerAzimuthRad));
            outlineVertices.push_back(rOuter * cosElev * std::sin(lowerAzimuthRad));
            outlineVertices.push_back(rOuter * sinElev);
        }
    }

    vertexCount_ = static_cast<int>(vertices_.size() / 3);
    outlineVertexCount_ = static_cast<int>(outlineVertices.size() / 3);

    // Upload plane geometry to GPU
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float), vertices_.data(), GL_DYNAMIC_DRAW);

    // Upload outline geometry to GPU
    glBindBuffer(GL_ARRAY_BUFFER, outlineVbo_);
    glBufferData(GL_ARRAY_BUFFER, outlineVertices.size() * sizeof(float), outlineVertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    geometryDirty_ = false;
}

void SlicingPlaneRenderer::setCutType(CutType type) {
    if (cutType_ != type) {
        cutType_ = type;
        geometryDirty_ = true;
    }
}

void SlicingPlaneRenderer::setOffset(float degrees) {
    if (offset_ != degrees) {
        offset_ = degrees;
        geometryDirty_ = true;
    }
}

void SlicingPlaneRenderer::setThickness(float degrees) {
    if (thickness_ != degrees) {
        thickness_ = degrees;
        geometryDirty_ = true;  // Outline positions depend on thickness
    }
}

void SlicingPlaneRenderer::setSphereRadius(float radius) {
    if (sphereRadius_ != radius) {
        sphereRadius_ = radius;
        geometryDirty_ = true;
    }
}

void SlicingPlaneRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
    if (!initialized_ || !visible_ || !shaderProgram_) {
        return;
    }

    // Update geometry if parameters changed
    if (geometryDirty_) {
        updateGeometry();
    }

    if (vertexCount_ == 0) {
        return;
    }

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth write but keep depth test
    glDepthMask(GL_FALSE);

    // Disable face culling to see both sides
    glDisable(GL_CULL_FACE);

    shaderProgram_->bind();
    shaderProgram_->setUniformValue("projection", projection);
    shaderProgram_->setUniformValue("view", view);
    shaderProgram_->setUniformValue("model", model);

    // Draw filled volume if enabled
    if (showFill_ && vertexCount_ > 0) {
        // Translucent cyan/teal color for the plane fill
        QVector4D planeColor(0.0f, 0.8f, 0.8f, 0.25f);  // Cyan with 25% opacity
        shaderProgram_->setUniformValue("planeColor", planeColor);

        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
        glBindVertexArray(0);
    }

    // Draw thickness boundary outlines (always visible)
    if (outlineVertexCount_ > 0) {
        // Black opaque lines for the boundaries
        QVector4D outlineColor(0.0f, 0.0f, 0.0f, 1.0f);
        shaderProgram_->setUniformValue("planeColor", outlineColor);

        // Set line width for visibility
        glLineWidth(2.0f);

        glBindVertexArray(outlineVao_);

        if (cutType_ == CutType::Azimuth) {
            // Two circles: draw each as LINE_STRIP
            int segmentsPerCircle = (outlineVertexCount_ / 2);
            glDrawArrays(GL_LINE_STRIP, 0, segmentsPerCircle);
            glDrawArrays(GL_LINE_STRIP, segmentsPerCircle, segmentsPerCircle);
        } else {
            // Four vertical lines: draw as GL_LINES (pairs of vertices)
            glDrawArrays(GL_LINES, 0, outlineVertexCount_);
        }

        glBindVertexArray(0);
        glLineWidth(1.0f);
    }

    shaderProgram_->release();

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
