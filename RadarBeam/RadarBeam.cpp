// ---- RadarBeam.cpp ----

#include "RadarBeam.h"
#include "ConicalBeam.h"
#include "EllipticalBeam.h"
#include "PhasedArrayBeam.h"
#include <cmath>
#include <QDebug>

// Constructor
RadarBeam::RadarBeam(float sphereRadius, float beamWidthDegrees)
	: sphereRadius_(sphereRadius),
	beamWidthDegrees_(beamWidthDegrees),
	color_(1.0f, 0.5f, 0.0f), // Default to orange-red
	opacity_(0.3f), // Default semi-transparent
	visible_(true),
	beamLengthFactor_(1.0f), // Default to full sphere diameter
	beamDirection_(BeamDirection::ToOrigin),
	customDirection_(0.0f, 0.0f, 0.0f),
	beamVBO(QOpenGLBuffer::VertexBuffer),
	beamEBO(QOpenGLBuffer::IndexBuffer)
{
	// Vertex shader for beam
	beamVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

	// Fragment shader for beam
	beamFragmentShaderSource = R"(
		#version 330 core
		in vec3 FragPos;
		in vec3 Normal;

		uniform vec3 beamColor;
		uniform float opacity;
		uniform vec3 viewPos; // Camera position

		out vec4 FragColor;

		void main() {
			// Improved lighting calculation
			vec3 norm = normalize(Normal);
			vec3 viewDir = normalize(viewPos - FragPos);
        
			// Improved Fresnel effect that's more consistent during rotation
			// Using absolute value of dot product to handle any viewing angle
			float fresnel = 0.3 + 0.7 * pow(1.0 - abs(dot(norm, viewDir)), 2.0);
        
			// Add rim lighting for better edge definition
			float rim = 1.0 - max(dot(norm, viewDir), 0.0);
			rim = smoothstep(0.4, 0.8, rim);
        
			// Final color with opacity
			vec4 finalColor = vec4(beamColor, opacity * (fresnel + rim * 0.3));
        
			// Ensure a minimum opacity to prevent the beam from disappearing
			finalColor.a = clamp(finalColor.a, 0.1, 1.0);
        
			FragColor = finalColor;
		}
	)";
}

// Destructor
RadarBeam::~RadarBeam() {
	// Make sure we're in a valid context before destroying resources
	QOpenGLContext* currentContext = QOpenGLContext::currentContext();
	if (!currentContext) {
		qWarning() << "No OpenGL context in RadarBeam destructor, resources may leak";
		return;
	}

	// Clean up OpenGL resources
	if (beamVAO.isCreated()) {
		beamVAO.destroy();
	}
	if (beamVBO.isCreated()) {
		beamVBO.destroy();
	}
	if (beamEBO.isCreated()) {
		beamEBO.destroy();
	}
	if (beamShaderProgram) {
		delete beamShaderProgram;
		beamShaderProgram = nullptr;
	}

	qDebug() << "RadarBeam destructor called - cleaned up OpenGL resources";
}

void RadarBeam::uploadGeometryToGPU() {
	// Make sure we have a VAO, VBO, and EBO already created in initialize()
	beamVAO.bind();

	// Vertex buffer
	beamVBO.bind();
	beamVBO.allocate(vertices_.data(),
		static_cast<int>(vertices_.size() * sizeof(float)));

	// Index buffer
	beamEBO.bind();
	beamEBO.allocate(indices_.data(),
		static_cast<int>(indices_.size() * sizeof(unsigned int)));

	beamVAO.release();
}

void RadarBeam::initialize() {
	// Make sure we don't initialize twice
	if (beamVAO.isCreated()) {
		qDebug() << "RadarBeam already initialized, skipping";
		return;
	}

	initializeOpenGLFunctions();
	setupShaders();
	createBeamGeometry();
}

void RadarBeam::setupShaders() {
	// Check if shader sources are properly initialized
	if (!beamVertexShaderSource || !beamFragmentShaderSource) {
		qCritical() << "Shader sources not initialized!";
		return;
	}

	// Create and compile beam shader program
	beamShaderProgram = new QOpenGLShaderProgram();

	// Debug shader compilation
	if (!beamShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, beamVertexShaderSource)) {
		qCritical() << "Failed to compile vertex shader:" << beamShaderProgram->log();
		return;
	}

	if (!beamShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, beamFragmentShaderSource)) {
		qCritical() << "Failed to compile fragment shader:" << beamShaderProgram->log();
		return;
	}

	if (!beamShaderProgram->link()) {
		qCritical() << "Failed to link shader program:" << beamShaderProgram->log();
		return;
	}

	qDebug() << "Beam shader program compiled and linked successfully";
}

void RadarBeam::update(const QVector3D& radarPosition) {
	currentRadarPosition_ = radarPosition;
	createBeamGeometry();
}

void RadarBeam::render(QOpenGLShaderProgram* program, const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
	// Early exit checks
	if (!visible_ || vertices_.empty()) {
		return;
	}

	// Check if OpenGL resources are valid
	if (!beamVAO.isCreated() || !beamVBO.isCreated() || !beamEBO.isCreated() || !beamShaderProgram) {
		qWarning() << "RadarBeam::render called with invalid OpenGL resources";
		return;
	}

	// Save current OpenGL state
	GLboolean depthMaskPrevious;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskPrevious);
	GLint blendSrcPrevious, blendDstPrevious;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcPrevious);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstPrevious);
	GLboolean blendPrevious = glIsEnabled(GL_BLEND);

	// Enable transparency with improved blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Disable depth writing for transparent objects but still do depth test
	glDepthMask(GL_FALSE);

	// Use the beam shader program
	beamShaderProgram->bind();

	// Set uniforms
	beamShaderProgram->setUniformValue("projection", projection);
	beamShaderProgram->setUniformValue("view", view);
	beamShaderProgram->setUniformValue("model", model);
	beamShaderProgram->setUniformValue("beamColor", color_);
	beamShaderProgram->setUniformValue("opacity", opacity_);

	// Extract camera position from view matrix for Fresnel effect
	QMatrix4x4 inverseView = view.inverted();
	QVector3D cameraPos = inverseView.column(3).toVector3D();
	beamShaderProgram->setUniformValue("viewPos", cameraPos);

	// Bind VAO and draw
	beamVAO.bind();

	// Check if we have valid indices before drawing
	if (!visible_ || vertices_.empty() || indices_.empty())
		return;
	
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	

	// Release VAO
	if (beamVAO.isCreated() && beamVAO.objectId() != 0) {
		beamVAO.release();
	}

	// Release shader program
	beamShaderProgram->release();

	// Restore previous OpenGL state
	glDepthMask(depthMaskPrevious);
	if (!blendPrevious) {
		glDisable(GL_BLEND);
	}
	glBlendFunc(blendSrcPrevious, blendDstPrevious);
}

void RadarBeam::setBeamWidth(float degrees) {
	beamWidthDegrees_ = degrees;
	createBeamGeometry();
}

void RadarBeam::setBeamDirection(BeamDirection direction) {
	beamDirection_ = direction;
	createBeamGeometry();
}

void RadarBeam::setCustomDirection(const QVector3D& direction) {
	customDirection_ = direction.normalized();
	beamDirection_ = BeamDirection::Custom;
	createBeamGeometry();
}

void RadarBeam::setColor(const QVector3D& color) {
	color_ = color;
}

void RadarBeam::setOpacity(float opacity) {
	opacity_ = opacity;
}

void RadarBeam::setVisible(bool visible) {
	visible_ = visible;
}

void RadarBeam::setBeamLength(float length) {
	beamLengthFactor_ = length;
	createBeamGeometry();
}

QVector3D RadarBeam::calculateBeamDirection(const QVector3D& radarPosition) {
	switch (beamDirection_) {
	case BeamDirection::ToOrigin:
		return -radarPosition.normalized();
	case BeamDirection::AwayFromOrigin:
		return radarPosition.normalized();
	case BeamDirection::Custom:
		return customDirection_.normalized();
	default:
		return -radarPosition.normalized();
	}
}

QVector3D RadarBeam::calculateOppositePoint(const QVector3D& radarPosition, const QVector3D& direction) {
	// Calculate where the beam would exit the sphere on the opposite side
	float a = direction.lengthSquared();
	float b = 2.0f * QVector3D::dotProduct(radarPosition, direction);
	float c = radarPosition.lengthSquared() - sphereRadius_ * sphereRadius_;

	// Discriminant for the quadratic equation
	float discriminant = b * b - 4 * a * c;

	if (discriminant < 0) {
		// No intersection (shouldn't happen if radar is on the sphere)
		return radarPosition + direction * 2.0f * sphereRadius_;
	}

	// Calculate the two solutions
	float t1 = (-b + sqrt(discriminant)) / (2.0f * a);
	float t2 = (-b - sqrt(discriminant)) / (2.0f * a);

	// We want the further intersection point (t should be positive)
	float t = (t1 > t2) ? t1 : t2;
	if (t < 0) {
		t = (t1 < 0 && t2 < 0) ? 0 : (t1 > 0 ? t1 : t2);
	}

	// Calculate the exit point
	return radarPosition + direction * t * beamLengthFactor_;
}

void RadarBeam::createBeamGeometry() {
	// Base implementation for a simple beam
	// This will be overridden by derived classes

	// Calculate beam direction
	QVector3D direction = calculateBeamDirection(currentRadarPosition_);

	// Calculate beam end point
	QVector3D endPoint = calculateOppositePoint(currentRadarPosition_, direction);

	// Calculate beam length and base radius
	float beamLength = (endPoint - currentRadarPosition_).length();
	float baseRadius = tan(beamWidthDegrees_ * M_PI / 180.0f / 2.0f) * beamLength;

	// Generate beam vertices (simple cone for base class)
	calculateBeamVertices(currentRadarPosition_, direction, beamLength, baseRadius);
}

void RadarBeam::calculateBeamVertices(const QVector3D& apex, const QVector3D& direction, float length, float baseRadius) {
	// Clear previous data
	vertices_.clear();
	indices_.clear();

	// Sanity check
	if (length <= 0.0f || baseRadius <= 0.0f) {
		qWarning() << "Invalid beam dimensions: length =" << length << ", radius =" << baseRadius;
		return;
	}

	// Number of segments around the base circle
	const int segments = 32;

	// Normalized direction vector
	QVector3D normDirection = direction.normalized();

	// Find perpendicular vectors to create the base circle
	QVector3D up(0.0f, 1.0f, 0.0f);
	if (qAbs(QVector3D::dotProduct(normDirection, up)) > 0.99f) {
		// If direction is nearly parallel to up, use a different vector
		up = QVector3D(1.0f, 0.0f, 0.0f);
	}

	QVector3D right = QVector3D::crossProduct(normDirection, up).normalized();
	up = QVector3D::crossProduct(right, normDirection).normalized();

	// Base center
	QVector3D baseCenter = apex + normDirection * length;

	// Add apex vertex (with normal pointing to beam direction)
	vertices_.push_back(apex.x());
	vertices_.push_back(apex.y());
	vertices_.push_back(apex.z());

	// Normal at apex (improved to enhance visibility during rotation)
	// Instead of just using the beam direction, we'll use a normal that's 
	// more conducive to consistent lighting
	vertices_.push_back(normDirection.x());
	vertices_.push_back(normDirection.y());
	vertices_.push_back(normDirection.z());

	// Add base vertices
	for (int i = 0; i < segments; i++) {
		float angle = 2.0f * M_PI * i / segments;
		float cA = cos(angle);
		float sA = sin(angle);

		QVector3D circlePoint = baseCenter + (right * cA + up * sA) * baseRadius;

		// Calculate normal (improved calculation for better lighting)
		// We blend between the beam direction and the outward direction
		// for smoother shading that's consistent during rotation
		QVector3D toCircle = (circlePoint - baseCenter).normalized();
		QVector3D normal = (normDirection * 0.2f + toCircle * 0.8f).normalized();

		// Position
		vertices_.push_back(circlePoint.x());
		vertices_.push_back(circlePoint.y());
		vertices_.push_back(circlePoint.z());

		// Normal
		vertices_.push_back(normal.x());
		vertices_.push_back(normal.y());
		vertices_.push_back(normal.z());
	}

	// Create triangles (apex to each pair of adjacent base vertices)
	for (int i = 0; i < segments; i++) {
		int next = (i + 1) % segments;

		// Indices (apex is 0, base starts at 1)
		indices_.push_back(0);  // Apex
		indices_.push_back(i + 1);  // Current base vertex
		indices_.push_back(next + 1);  // Next base vertex
	}

	// Set up VAO and VBO for the beam
	if (!beamVAO.isCreated()) {
		beamVAO.create();
	}
	beamVAO.bind();

	if (!beamVBO.isCreated()) {
		beamVBO.create();
	}
	beamVBO.bind();
	beamVBO.allocate(vertices_.data(), vertices_.size() * sizeof(float));

	if (!beamEBO.isCreated()) {
		beamEBO.create();
	}
	beamEBO.bind();
	beamEBO.allocate(indices_.data(), indices_.size() * sizeof(unsigned int));

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//beamVAO.release();
}

RadarBeam* RadarBeam::createBeam(BeamType type, float sphereRadius, float beamWidthDegrees) {
	switch (type) {
	case BeamType::Conical:
		return new ConicalBeam(sphereRadius, beamWidthDegrees);
	case BeamType::Elliptical:
		return new EllipticalBeam(sphereRadius, beamWidthDegrees, beamWidthDegrees / 2.0f);
	case BeamType::Phased:
		return new PhasedArrayBeam(sphereRadius, beamWidthDegrees);
	case BeamType::Shaped:
		// For now, return conical but could implement shaped beam later
		return new ConicalBeam(sphereRadius, beamWidthDegrees);
	default:
		return new ConicalBeam(sphereRadius, beamWidthDegrees);
	}
}
