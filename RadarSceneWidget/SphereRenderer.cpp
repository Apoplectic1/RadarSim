#include "SphereRenderer.h"
#include <QOpenGLShaderProgram>
#include <QtMath>

SphereRenderer::SphereRenderer(float radius, int resolution)
    : Component("SphereRenderer")
    , m_Radius(radius)
    , m_Resolution(resolution)
    , m_Color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_Visible(true)
    , m_Position(0.0f, 0.0f, 0.0f)
    , m_ShaderProgram(nullptr)
    , m_IndexCount(0)
    , m_Initialized(false)
    , m_NeedsRebuild(true)
{
    // Initialize buffers
    m_VBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_EBO = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
}

SphereRenderer::~SphereRenderer()
{
    if (m_Initialized) {
        m_VAO.destroy();
        m_VBO.destroy();
        m_EBO.destroy();
    }
}

void SphereRenderer::Initialize()
{
    initializeOpenGLFunctions();

    if (!m_Initialized) {
        m_VAO.create();
        m_VBO.create();
        m_EBO.create();
        m_Initialized = true;
    }

    GenerateSphere();
}

void SphereRenderer::Update(float deltaTime)
{
    if (m_NeedsRebuild) {
        GenerateSphere();
        m_NeedsRebuild = false;
    }

    // Update model matrix based on position
    m_ModelMatrix.setToIdentity();
    m_ModelMatrix.translate(m_Position);
}

void SphereRenderer::Render()
{
    if (!m_Visible || !m_ShaderProgram) {
        return;
    }

    m_ShaderProgram->bind();

    // Set uniforms
    m_ShaderProgram->setUniformValue("projectionMatrix", m_ProjectionMatrix);
    m_ShaderProgram->setUniformValue("viewMatrix", m_ViewMatrix);
    m_ShaderProgram->setUniformValue("modelMatrix", m_ModelMatrix);
    m_ShaderProgram->setUniformValue("objectColor", m_Color);

    // Draw the sphere
    m_VAO.bind();
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
    m_VAO.release();

    m_ShaderProgram->release();
}

void SphereRenderer::SetRadius(float radius)
{
    if (m_Radius != radius) {
        m_Radius = radius;
        m_NeedsRebuild = true;
    }
}

void SphereRenderer::SetResolution(int resolution)
{
    if (m_Resolution != resolution) {
        m_Resolution = resolution;
        m_NeedsRebuild = true;
    }
}

void SphereRenderer::SetPosition(const QVector3D& position)
{
    m_Position = position;

    // Update model matrix
    m_ModelMatrix.setToIdentity();
    m_ModelMatrix.translate(m_Position);
}

void SphereRenderer::GenerateSphere()
{
    m_Vertices.clear();
    m_Indices.clear();

    // Generate vertices
    for (int y = 0; y <= m_Resolution; y++) {
        for (int x = 0; x <= m_Resolution; x++) {
            float xSegment = (float)x / (float)m_Resolution;
            float ySegment = (float)y / (float)m_Resolution;
            float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
            float yPos = std::cos(ySegment * M_PI);
            float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

            // Position
            m_Vertices.push_back(xPos * m_Radius);
            m_Vertices.push_back(yPos * m_Radius);
            m_Vertices.push_back(zPos * m_Radius);

            // Normal
            m_Vertices.push_back(xPos);
            m_Vertices.push_back(yPos);
            m_Vertices.push_back(zPos);

            // Texture coordinate (if needed)
            m_Vertices.push_back(xSegment);
            m_Vertices.push_back(ySegment);
        }
    }

    // Generate indices
    for (int y = 0; y < m_Resolution; y++) {
        for (int x = 0; x < m_Resolution; x++) {
            unsigned int rowStart = y * (m_Resolution + 1);
            unsigned int nextRowStart = (y + 1) * (m_Resolution + 1);

            m_Indices.push_back(rowStart + x);
            m_Indices.push_back(nextRowStart + x);
            m_Indices.push_back(rowStart + x + 1);

            m_Indices.push_back(nextRowStart + x);
            m_Indices.push_back(nextRowStart + x + 1);
            m_Indices.push_back(rowStart + x + 1);
        }
    }

    m_IndexCount = m_Indices.size();

    // Upload to GPU
    m_VAO.bind();

    m_VBO.bind();
    m_VBO.allocate(m_Vertices.data(), m_Vertices.size() * sizeof(float));

    m_EBO.bind();
    m_EBO.allocate(m_Indices.data(), m_Indices.size() * sizeof(unsigned int));

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));

    m_VAO.release();
    m_VBO.release();
    m_EBO.release();
}