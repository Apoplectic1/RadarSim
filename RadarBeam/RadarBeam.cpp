// ---- RadarBeam.cpp ----

#include "RadarBeam.h"
#include "ConicalBeam.h"
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
	if (vboId_ != 0) {
		glDeleteBuffers(1, &vboId_);
		vboId_ = 0;
	}
	if (eboId_ != 0) {
		glDeleteBuffers(1, &eboId_);
		eboId_ = 0;
	}
	if (beamShaderProgram) {
		delete beamShaderProgram;
		beamShaderProgram = nullptr;
	}

	qDebug() << "RadarBeam destructor called - cleaned up OpenGL resources";
}

void RadarBeam::uploadGeometryToGPU() {
	// Check for valid OpenGL context
	if (!QOpenGLContext::currentContext()) {
		// No GL context - mark geometry as dirty for later upload
		geometryDirty_ = true;
		return;
	}

	// Make sure VAO exists
	if (!beamVAO.isCreated()) {
		geometryDirty_ = true;
		return;
	}

	// Don't upload empty geometry
	if (vertices_.empty() || indices_.empty()) {
		return;
	}

	beamVAO.bind();

	// Create VBO if needed
	if (vboId_ == 0) {
		glGenBuffers(1, &vboId_);
	}
	glBindBuffer(GL_ARRAY_BUFFER, vboId_);
	glBufferData(GL_ARRAY_BUFFER,
		vertices_.size() * sizeof(float),
		vertices_.data(),
		GL_DYNAMIC_DRAW);

	// Setup vertex attributes while VBO is bound
	// Position attribute (location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute (location 1)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Create EBO if needed
	if (eboId_ == 0) {
		glGenBuffers(1, &eboId_);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		indices_.size() * sizeof(unsigned int),
		indices_.data(),
		GL_DYNAMIC_DRAW);

	beamVAO.release();
	geometryDirty_ = false;
}

void RadarBeam::initialize() {
	// Make sure we don't initialize twice
	if (beamVAO.isCreated()) {
		qDebug() << "RadarBeam already initialized, skipping";
		return;
	}

	initializeOpenGLFunctions();
	setupShaders();

	// Create VAO only - buffers will be created in uploadGeometryToGPU
	beamVAO.create();
	beamVAO.bind();
	beamVAO.release();

	// Now create the initial geometry and upload to GPU
	createBeamGeometry();
	uploadGeometryToGPU();
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
	uploadGeometryToGPU();
}

void RadarBeam::render(QOpenGLShaderProgram* program, const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
	static int frameCount = 0;
	bool shouldLog = (++frameCount % 60 == 0); // Log every 60 frames

	if (shouldLog) {
		qDebug() << "RadarBeam::render() called:";
		qDebug() << "  visible_:" << visible_;
		qDebug() << "  vertices_.empty():" << vertices_.empty();
		qDebug() << "  indices_.size():" << indices_.size();
	}

	// Early exit checks
	if (!visible_ || vertices_.empty()) {
		if (shouldLog) {
			qDebug() << "  EARLY EXIT - not rendering";
		}
		return;
	}

	if (shouldLog) {
		qDebug() << "  RENDERING beam";
	}

	// Check if OpenGL resources are valid
	if (!beamVAO.isCreated() || !beamShaderProgram) {
		qWarning() << "RadarBeam::render called with invalid OpenGL resources";
		return;
	}

	// Upload geometry if it was deferred (no GL context when setter was called)
	if (geometryDirty_) {
		uploadGeometryToGPU();
	}

	// Verify buffers exist
	if (vboId_ == 0 || eboId_ == 0) {
		qWarning() << "RadarBeam::render - buffers not created";
		return;
	}

	// Verify we have valid index count before drawing
	if (indices_.empty()) {
		qWarning() << "RadarBeam::render - indices_ is empty, skipping draw";
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

	// Enable depth testing so beam is occluded by solid objects (targets)
	// but disable depth writing for transparent objects
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);

	// Use stencil test for shadow volume occlusion
	// Target render sets stencil > 0 in shadow regions
	// Only draw beam where stencil == 0 (not in shadow)
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 0, 0xFF);  // Pass only where stencil == 0
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);  // Don't modify stencil

	// Enable face culling - only render front faces of beam cone
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

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

	// Bind buffers explicitly
	glBindBuffer(GL_ARRAY_BUFFER, vboId_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	

	// Release VAO
	if (beamVAO.isCreated() && beamVAO.objectId() != 0) {
		beamVAO.release();
	}

	// Release shader program
	beamShaderProgram->release();

	// Restore previous OpenGL state
	glDepthMask(depthMaskPrevious);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	if (!blendPrevious) {
		glDisable(GL_BLEND);
	}
	glBlendFunc(blendSrcPrevious, blendDstPrevious);
}

void RadarBeam::renderDepthOnly(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
	if (!visible_) return;

	// Verify VAO exists
	if (!beamVAO.isCreated() || beamVAO.objectId() == 0) {
		return;
	}

	// Verify buffers exist
	if (vboId_ == 0 || eboId_ == 0 || indices_.empty()) {
		return;
	}

	// Render to depth buffer only - no color output
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_STENCIL_TEST);  // No stencil for depth pre-pass
	glDisable(GL_BLEND);

	// Enable face culling - only render front faces
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Use the beam shader program
	beamShaderProgram->bind();

	// Set uniforms
	beamShaderProgram->setUniformValue("projection", projection);
	beamShaderProgram->setUniformValue("view", view);
	beamShaderProgram->setUniformValue("model", model);
	beamShaderProgram->setUniformValue("beamColor", color_);
	beamShaderProgram->setUniformValue("opacity", 1.0f);  // Full opacity for depth

	// Bind VAO and draw
	beamVAO.bind();
	glBindBuffer(GL_ARRAY_BUFFER, vboId_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	beamVAO.release();

	beamShaderProgram->release();

	// Restore state
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_CULL_FACE);
}

void RadarBeam::setBeamWidth(float degrees) {
	beamWidthDegrees_ = degrees;
	createBeamGeometry();
	uploadGeometryToGPU();
}

void RadarBeam::setSphereRadius(float radius) {
	if (sphereRadius_ != radius) {
		sphereRadius_ = radius;

		// Scale the current radar position to the new sphere radius
		// This ensures the radar stays on the sphere surface
		if (!currentRadarPosition_.isNull()) {
			currentRadarPosition_ = currentRadarPosition_.normalized() * radius;
		}

		// Regenerate geometry and upload to GPU
		createBeamGeometry();
		uploadGeometryToGPU();
	}
}

void RadarBeam::setBeamDirection(BeamDirection direction) {
	beamDirection_ = direction;
	createBeamGeometry();
	uploadGeometryToGPU();
}

void RadarBeam::setCustomDirection(const QVector3D& direction) {
	customDirection_ = direction.normalized();
	beamDirection_ = BeamDirection::Custom;
	createBeamGeometry();
	uploadGeometryToGPU();
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
	uploadGeometryToGPU();
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

	// Skip geometry creation if position hasn't been set yet
	// (avoids creating stray geometry at the origin)
	if (currentRadarPosition_.isNull()) {
		vertices_.clear();
		indices_.clear();
		return;
	}

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
	const int capRings = 8;  // Number of rings for the spherical cap

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

	// Base center (conceptual center of cone base before projection)
	QVector3D baseCenter = apex + normDirection * length;

	// Cap center is where the beam axis exits the sphere on the opposite side
	// This should be the antipodal point from the radar position (apex)
	QVector3D capCenter = -apex.normalized() * sphereRadius_;

	// Add apex vertex (with normal pointing outward from cone surface)
	// Vertex 0
	vertices_.push_back(apex.x());
	vertices_.push_back(apex.y());
	vertices_.push_back(apex.z());
	vertices_.push_back(normDirection.x());
	vertices_.push_back(normDirection.y());
	vertices_.push_back(normDirection.z());

	// Store the outer rim vertices on the sphere (where cone meets sphere)
	// These will be vertices 1 to segments
	std::vector<QVector3D> outerRimPoints;
	for (int i = 0; i < segments; i++) {
		float angle = 2.0f * M_PI * i / segments;
		float cA = cos(angle);
		float sA = sin(angle);

		QVector3D circlePoint = baseCenter + (right * cA + up * sA) * baseRadius;

		// Project point onto sphere surface
		circlePoint = circlePoint.normalized() * sphereRadius_;
		outerRimPoints.push_back(circlePoint);

		QVector3D toCircle = (circlePoint - baseCenter).normalized();
		QVector3D normal = (normDirection * 0.2f + toCircle * 0.8f).normalized();

		// Position
		vertices_.push_back(circlePoint.x());
		vertices_.push_back(circlePoint.y());
		vertices_.push_back(circlePoint.z());

		// Normal (pointing outward for cone surface)
		vertices_.push_back(normal.x());
		vertices_.push_back(normal.y());
		vertices_.push_back(normal.z());
	}

	// Create cone side triangles (apex to each pair of adjacent rim vertices)
	for (int i = 0; i < segments; i++) {
		int next = (i + 1) % segments;
		indices_.push_back(0);  // Apex
		indices_.push_back(i + 1);  // Current rim vertex
		indices_.push_back(next + 1);  // Next rim vertex
	}

	// Now add the spherical cap to fill in the "dish"
	// We'll create concentric rings from the outer rim toward the cap center

	int capStartVertex = segments + 1;  // First vertex of cap interior

	// Add vertices for inner rings of the cap
	for (int ring = 1; ring <= capRings; ring++) {
		float t = (float)ring / (float)capRings;  // 0 = outer rim, 1 = center

		for (int i = 0; i < segments; i++) {
			// Interpolate between outer rim point and cap center on the sphere surface
			QVector3D outerPoint = outerRimPoints[i];

			// Spherical interpolation (slerp) between outer point and cap center
			QVector3D interpolated = (outerPoint * (1.0f - t) + capCenter * t).normalized() * sphereRadius_;

			// Normal points inward (toward sphere center) for the cap
			QVector3D normal = -interpolated.normalized();

			vertices_.push_back(interpolated.x());
			vertices_.push_back(interpolated.y());
			vertices_.push_back(interpolated.z());
			vertices_.push_back(normal.x());
			vertices_.push_back(normal.y());
			vertices_.push_back(normal.z());
		}
	}

	// Create triangles for the cap
	// First ring: connect outer rim (vertices 1..segments) to first inner ring
	for (int i = 0; i < segments; i++) {
		int next = (i + 1) % segments;
		int outerCurr = i + 1;  // Outer rim vertex
		int outerNext = next + 1;
		int innerCurr = capStartVertex + i;  // First inner ring vertex
		int innerNext = capStartVertex + next;

		// Two triangles per quad
		indices_.push_back(outerCurr);
		indices_.push_back(innerCurr);
		indices_.push_back(outerNext);

		indices_.push_back(outerNext);
		indices_.push_back(innerCurr);
		indices_.push_back(innerNext);
	}

	// Remaining rings: connect ring to ring
	for (int ring = 1; ring < capRings; ring++) {
		int outerRingStart = capStartVertex + (ring - 1) * segments;
		int innerRingStart = capStartVertex + ring * segments;

		for (int i = 0; i < segments; i++) {
			int next = (i + 1) % segments;
			int outerCurr = outerRingStart + i;
			int outerNext = outerRingStart + next;
			int innerCurr = innerRingStart + i;
			int innerNext = innerRingStart + next;

			// Two triangles per quad
			indices_.push_back(outerCurr);
			indices_.push_back(innerCurr);
			indices_.push_back(outerNext);

			indices_.push_back(outerNext);
			indices_.push_back(innerCurr);
			indices_.push_back(innerNext);
		}
	}
}

RadarBeam* RadarBeam::createBeam(BeamType type, float sphereRadius, float beamWidthDegrees) {
	switch (type) {
	case BeamType::Conical:
		return new ConicalBeam(sphereRadius, beamWidthDegrees);
	case BeamType::Phased:
		return new PhasedArrayBeam(sphereRadius, beamWidthDegrees);
	case BeamType::Shaped:
		// For now, return conical but could implement shaped beam later
		return new ConicalBeam(sphereRadius, beamWidthDegrees);
	default:
		return new ConicalBeam(sphereRadius, beamWidthDegrees);
	}
}
