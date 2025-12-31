// ---- PolarPlot/PolarRCSPlot.cpp ----

#include "PolarRCSPlot.h"
#include "../Constants.h"
#include <QPainter>
#include <QFont>
#include <cmath>

using namespace RS::Constants;

PolarRCSPlot::PolarRCSPlot(QWidget* parent)
    : QOpenGLWidget(parent)
{
    // Pre-allocate data vector for 360 bins
    data_.resize(360);
    for (int i = 0; i < 360; ++i) {
        data_[i] = RCSDataPoint(static_cast<float>(i), -60.0f, false);
    }
}

PolarRCSPlot::~PolarRCSPlot() {
    makeCurrent();

    if (gridVao_ != 0) {
        glDeleteVertexArrays(1, &gridVao_);
    }
    if (gridVbo_ != 0) {
        glDeleteBuffers(1, &gridVbo_);
    }
    if (dataVao_ != 0) {
        glDeleteVertexArrays(1, &dataVao_);
    }
    if (dataVbo_ != 0) {
        glDeleteBuffers(1, &dataVbo_);
    }

    doneCurrent();
}

void PolarRCSPlot::setData(const std::vector<RCSDataPoint>& data) {
    if (data.size() != 360) {
        qWarning("PolarRCSPlot::setData: expected 360 data points, got %zu", data.size());
        return;
    }
    data_ = data;
    dataDirty_ = true;
    update();
}

void PolarRCSPlot::setScale(float minDBsm, float maxDBsm) {
    if (minDBsm >= maxDBsm) {
        qWarning("PolarRCSPlot::setScale: min must be less than max");
        return;
    }
    minDBsm_ = minDBsm;
    maxDBsm_ = maxDBsm;
    gridDirty_ = true;
    dataDirty_ = true;
    update();
}

void PolarRCSPlot::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);  // Dark gray background

    setupShaders();
    setupBuffers();
}

void PolarRCSPlot::resizeGL(int w, int h) {
    viewWidth_ = w;
    viewHeight_ = h;

    // Calculate plot center and radius to fit in viewport with margin
    float margin = 50.0f;  // Pixels for labels
    float availableSize = std::min(static_cast<float>(w), static_cast<float>(h)) - 2.0f * margin;
    plotRadius_ = availableSize / 2.0f;
    plotCenterX_ = w / 2.0f;
    plotCenterY_ = h / 2.0f;

    gridDirty_ = true;
    dataDirty_ = true;
}

void PolarRCSPlot::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up orthographic projection for 2D rendering
    QMatrix4x4 projection;
    projection.ortho(0.0f, static_cast<float>(viewWidth_),
                     static_cast<float>(viewHeight_), 0.0f,
                     -1.0f, 1.0f);

    if (lineShader_) {
        lineShader_->bind();
        lineShader_->setUniformValue("projection", projection);
    }

    drawPolarGrid();
    drawDataCurve();

    if (lineShader_) {
        lineShader_->release();
    }

    // Draw labels using QPainter (after OpenGL)
    drawAxisLabels();
}

void PolarRCSPlot::setupShaders() {
    // Simple 2D line shader
    static const char* vertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec3 color;

        uniform mat4 projection;

        out vec3 fragColor;

        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
            fragColor = color;
        }
    )";

    static const char* fragmentShaderSource = R"(
        #version 450 core
        in vec3 fragColor;
        out vec4 outColor;

        void main() {
            outColor = vec4(fragColor, 1.0);
        }
    )";

    lineShader_ = std::make_unique<QOpenGLShaderProgram>();
    lineShader_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    lineShader_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    lineShader_->link();
}

void PolarRCSPlot::setupBuffers() {
    // Grid buffers
    glGenVertexArrays(1, &gridVao_);
    glGenBuffers(1, &gridVbo_);

    // Data curve buffers
    glGenVertexArrays(1, &dataVao_);
    glGenBuffers(1, &dataVbo_);
}

QPointF PolarRCSPlot::polarToScreen(float angleDeg, float radius) const {
    // Convert angle to radians (0 degrees = right, counter-clockwise)
    // Adjust so 0 degrees points up (north)
    float angleRad = (90.0f - angleDeg) * kDegToRadF;
    float x = plotCenterX_ + radius * std::cos(angleRad);
    float y = plotCenterY_ - radius * std::sin(angleRad);  // Y inverted for screen coords
    return QPointF(x, y);
}

float PolarRCSPlot::dBsmToRadius(float dBsm) const {
    // Clamp to scale range
    float clampedDBsm = std::max(minDBsm_, std::min(maxDBsm_, dBsm));
    // Normalize to [0, 1] then scale to plot radius
    float normalized = (clampedDBsm - minDBsm_) / (maxDBsm_ - minDBsm_);
    return normalized * plotRadius_;
}

void PolarRCSPlot::drawPolarGrid() {
    if (gridDirty_) {
        gridVertices_.clear();

        // Grid color (light gray)
        const float gc[3] = {0.4f, 0.4f, 0.4f};

        // Draw radial rings (every 10 dB)
        float dBsmRange = maxDBsm_ - minDBsm_;
        int numRings = static_cast<int>(dBsmRange / 10.0f);

        for (int ring = 0; ring <= numRings; ++ring) {
            float dBsm = minDBsm_ + ring * 10.0f;
            float radius = dBsmToRadius(dBsm);

            // Draw circle as line segments
            for (int i = 0; i < kCircleSegments; ++i) {
                float angle1 = static_cast<float>(i);
                float angle2 = static_cast<float>((i + 1) % kCircleSegments);

                QPointF p1 = polarToScreen(angle1, radius);
                QPointF p2 = polarToScreen(angle2, radius);

                // Vertex 1: position (x, y), color (r, g, b)
                gridVertices_.push_back(static_cast<float>(p1.x()));
                gridVertices_.push_back(static_cast<float>(p1.y()));
                gridVertices_.push_back(gc[0]);
                gridVertices_.push_back(gc[1]);
                gridVertices_.push_back(gc[2]);

                // Vertex 2
                gridVertices_.push_back(static_cast<float>(p2.x()));
                gridVertices_.push_back(static_cast<float>(p2.y()));
                gridVertices_.push_back(gc[0]);
                gridVertices_.push_back(gc[1]);
                gridVertices_.push_back(gc[2]);
            }
        }

        // Draw angular lines (every 30 degrees)
        for (int i = 0; i < kAngularGridLines; ++i) {
            float angleDeg = static_cast<float>(i * 30);
            QPointF pInner = polarToScreen(angleDeg, 0.0f);
            QPointF pOuter = polarToScreen(angleDeg, plotRadius_);

            // Line from center to outer edge
            gridVertices_.push_back(static_cast<float>(pInner.x()));
            gridVertices_.push_back(static_cast<float>(pInner.y()));
            gridVertices_.push_back(gc[0]);
            gridVertices_.push_back(gc[1]);
            gridVertices_.push_back(gc[2]);

            gridVertices_.push_back(static_cast<float>(pOuter.x()));
            gridVertices_.push_back(static_cast<float>(pOuter.y()));
            gridVertices_.push_back(gc[0]);
            gridVertices_.push_back(gc[1]);
            gridVertices_.push_back(gc[2]);
        }

        // Upload to GPU
        glBindVertexArray(gridVao_);
        glBindBuffer(GL_ARRAY_BUFFER, gridVbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(gridVertices_.size() * sizeof(float)),
                     gridVertices_.data(), GL_DYNAMIC_DRAW);

        // Position attribute (location 0)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        // Color attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        gridDirty_ = false;
    }

    // Draw grid
    glBindVertexArray(gridVao_);
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(gridVertices_.size() / 5));
    glBindVertexArray(0);
}

void PolarRCSPlot::drawDataCurve() {
    if (dataDirty_) {
        dataVertices_.clear();

        // Data color (orange to match beam)
        const float dc[3] = {1.0f, 0.5f, 0.0f};

        // Build line strip from data
        bool firstValid = true;
        QPointF lastPoint;

        for (int i = 0; i <= 360; ++i) {
            int idx = i % 360;
            const auto& point = data_[idx];

            float radius = dBsmToRadius(point.dBsm);
            QPointF screenPos = polarToScreen(point.angleDegrees, radius);

            if (!firstValid) {
                // Add line segment from last point to this point
                dataVertices_.push_back(static_cast<float>(lastPoint.x()));
                dataVertices_.push_back(static_cast<float>(lastPoint.y()));
                dataVertices_.push_back(dc[0]);
                dataVertices_.push_back(dc[1]);
                dataVertices_.push_back(dc[2]);

                dataVertices_.push_back(static_cast<float>(screenPos.x()));
                dataVertices_.push_back(static_cast<float>(screenPos.y()));
                dataVertices_.push_back(dc[0]);
                dataVertices_.push_back(dc[1]);
                dataVertices_.push_back(dc[2]);
            }

            lastPoint = screenPos;
            firstValid = false;
        }

        // Upload to GPU
        glBindVertexArray(dataVao_);
        glBindBuffer(GL_ARRAY_BUFFER, dataVbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(dataVertices_.size() * sizeof(float)),
                     dataVertices_.data(), GL_DYNAMIC_DRAW);

        // Position attribute (location 0)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        // Color attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        dataDirty_ = false;
    }

    // Draw data curve
    if (!dataVertices_.empty()) {
        glBindVertexArray(dataVao_);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(dataVertices_.size() / 5));
        glBindVertexArray(0);
    }
}

void PolarRCSPlot::drawAxisLabels() {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw title
    QFont titleFont = painter.font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(25, 8, viewWidth_ - 25, 20), Qt::AlignLeft, "RCS Polar Plot");

    // Draw subtitle
    QFont subtitleFont = painter.font();
    subtitleFont.setPointSize(10);
    subtitleFont.setBold(false);
    painter.setFont(subtitleFont);
    painter.setPen(QColor(180, 180, 180));  // Light gray
    painter.drawText(QRectF(25, 28, viewWidth_ - 25, 16), Qt::AlignLeft, "dBsm");

    // Reset font for other labels
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    painter.setPen(Qt::white);

    // Draw angle labels at cardinal and intercardinal directions
    const char* angleLabels[] = {"0", "30", "60", "90", "120", "150", "180", "210", "240", "270", "300", "330"};
    for (int i = 0; i < 12; ++i) {
        float angleDeg = static_cast<float>(i * 30);
        float labelRadius = plotRadius_ + 20.0f;
        QPointF pos = polarToScreen(angleDeg, labelRadius);

        QString label = QString::fromLatin1(angleLabels[i]) + QString::fromUtf8("\u00B0");
        QRectF rect(pos.x() - 20, pos.y() - 10, 40, 20);
        painter.drawText(rect, Qt::AlignCenter, label);
    }

    // Draw dBsm values along the 45-degree line (just numbers, no unit)
    float dBsmRange = maxDBsm_ - minDBsm_;
    int numRings = static_cast<int>(dBsmRange / 10.0f);

    for (int ring = 0; ring <= numRings; ++ring) {
        float dBsm = minDBsm_ + ring * 10.0f;
        float radius = dBsmToRadius(dBsm);

        // Position label at 45 degrees for less clutter
        QPointF pos = polarToScreen(45.0f, radius);

        QString label = QString::number(static_cast<int>(dBsm));
        QRectF rect(pos.x() + 5, pos.y() - 8, 40, 16);
        painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, label);
    }

    painter.end();
}
