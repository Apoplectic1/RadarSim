#pragma once

#include "Component.h"
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>
#include <vector>

class QOpenGLShaderProgram;

class SphereRenderer : public Component, protected QOpenGLFunctions {
public:
    SphereRenderer(float radius = 1.0f, int resolution = 36);
    virtual ~SphereRenderer();

    // Component interface
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Render() override;

    // Sphere properties
    void SetRadius(float radius);
    float GetRadius() const { return m_Radius; }

    void SetResolution(int resolution);
    int GetResolution() const { return m_Resolution; }

    void SetColor(const QVector4D& color) { m_Color = color; }
    const QVector4D& GetColor() const { return m_Color; }

    void SetVisible(bool visible) { m_Visible = visible; }
    bool IsVisible() const { return m_Visible; }

    void SetPosition(const QVector3D& position);
    const QVector3D& GetPosition() const { return m_Position; }

    // Shader and matrices
    void SetShaderProgram(QOpenGLShaderProgram* program) { m_ShaderProgram = program; }
    void SetProjectionMatrix(const QMatrix4x4& matrix) { m_ProjectionMatrix = matrix; }
    void SetViewMatrix(const QMatrix4x4& matrix) { m_ViewMatrix = matrix; }

private:
    void GenerateSphere();

private:
    // Sphere properties
    float m_Radius;
    int m_Resolution;
    QVector4D m_Color;
    bool m_Visible;
    QVector3D m_Position;
    QMatrix4x4 m_ModelMatrix;

    // OpenGL objects
    QOpenGLVertexArrayObject m_VAO;
    QOpenGLBuffer m_VBO;
    QOpenGLBuffer m_EBO;
    int m_IndexCount;

    // Mesh data
    std::vector<float> m_Vertices;
    std::vector<unsigned int> m_Indices;

    // Shader and matrices
    QOpenGLShaderProgram* m_ShaderProgram;
    QMatrix4x4 m_ProjectionMatrix;
    QMatrix4x4 m_ViewMatrix;

    // State tracking
    bool m_Initialized;
    bool m_NeedsRebuild;
};