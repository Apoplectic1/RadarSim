// ---- RadarBeam.cpp ----

#include "RadarBeam.h"
#include "GLUtils.h"
#include "ConicalBeam.h"
#include "PhasedArrayBeam.h"
#include "Constants.h"
#include <cmath>

using namespace RadarSim::Constants;

// Constructor
RadarBeam::RadarBeam(float sphereRadius, float beamWidthDegrees)
	: sphereRadius_(sphereRadius),
	beamWidthDegrees_(beamWidthDegrees),
	color_(Colors::kBeamOrange[0], Colors::kBeamOrange[1], Colors::kBeamOrange[2]),
	opacity_(Defaults::kBeamOpacity),
	visible_(true),
	beamLengthFactor_(1.0f), // Default to full sphere diameter
	beamDirection_(BeamDirection::ToOrigin),
	customDirection_(0.0f, 0.0f, 0.0f)
{
	// Vertex shader for beam
	beamVertexShaderSource_ = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec3 LocalPos;  // Untransformed position for shadow lookup

        void main() {
            LocalPos = aPos;  // Pass untransformed position
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

	// Fragment shader for beam with GPU shadow map lookup
	beamFragmentShaderSource_ = R"(
		#version 330 core
		in vec3 FragPos;
		in vec3 Normal;
		in vec3 LocalPos;  // Untransformed position for shadow lookup

		uniform vec3 beamColor;
		uniform float opacity;
		uniform vec3 viewPos; // Camera position

		// GPU shadow map parameters
		uniform sampler2D shadowMap;
		uniform bool gpuShadowEnabled;
		uniform vec3 radarPos;
		uniform vec3 beamAxis;
		uniform float beamWidthRad;
		uniform int numRings;  // For correct UV mapping

		out vec4 FragColor;

		// Convert LOCAL position to shadow map UV coordinates
		// Uses untransformed positions to match ray tracing coordinate system
		vec2 worldToShadowMapUV(vec3 localPos) {
			vec3 toFrag = normalize(localPos - radarPos);

			// Elevation from beam axis (0 = on axis, beamWidthRad = at edge)
			float cosElev = dot(toFrag, beamAxis);
			float elevation = acos(clamp(cosElev, -1.0, 1.0));
			float elevNorm = elevation / beamWidthRad;  // [0, 1] within beam

			// Azimuth around beam axis
			vec3 perpComponent = toFrag - beamAxis * cosElev;
			float perpLen = length(perpComponent);

			if (perpLen < 0.001) return vec2(0.0, elevNorm);
			perpComponent /= perpLen;

			// Build coordinate frame (same as ray generation)
			vec3 up = abs(beamAxis.z) < 0.99 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
			vec3 right = normalize(cross(beamAxis, up));
			up = normalize(cross(right, beamAxis));

			float azimuth = atan(dot(perpComponent, up), dot(perpComponent, right));
			// atan returns [-π, π], but ray gen uses [0, 2π)
			if (azimuth < 0.0) azimuth += 2.0 * 3.14159265;
			float azNorm = azimuth / (2.0 * 3.14159265);  // [0, 1)

			// Ray ring r has elevation = beamWidthRad * (r + 1) / numRings
			// So elevNorm = (r + 1) / numRings, and texel Y = r
			// To map elevNorm to texel Y correctly:
			// texel Y = elevNorm * numRings - 1
			// UV.y should give texel Y when multiplied by texture height (numRings)
			// UV.y = (elevNorm * numRings - 1 + 0.5) / numRings = elevNorm - 0.5/numRings
			float uvY = elevNorm - 0.5 / float(numRings);

			return vec2(azNorm, uvY);
		}

		void main() {
			// GPU shadow map lookup - discard fragments where target blocks the beam
			// Use LocalPos (untransformed) to match ray tracing coordinate system
			if (gpuShadowEnabled) {
				vec2 uv = worldToShadowMapUV(LocalPos);
				if (uv.y >= 0.0 && uv.y <= 1.0) {
					float hitDistance = texture(shadowMap, uv).r;
					// hitDistance < 0 means no hit (ray missed target)
					// hitDistance > 0 means ray hit target at that distance
					if (hitDistance > 0.0) {
						// Calculate fragment's distance from radar
						float fragDistance = length(LocalPos - radarPos);
						// Only discard if fragment is behind the hit point
						if (fragDistance > hitDistance) {
							discard;
						}
					}
				}
			}

			// Improved lighting calculation
			vec3 norm = normalize(Normal);
			vec3 viewDir = normalize(viewPos - FragPos);

			// Improved Fresnel effect that's more consistent during rotation
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

// Clean up OpenGL resources - must be called with valid GL context
void RadarBeam::cleanup() {
	// Check for valid OpenGL context
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	if (!ctx) {
		qWarning() << "RadarBeam::cleanup() - no current context, skipping GL cleanup";
		beamShaderProgram_.reset();
		return;
	}

	// Clean up OpenGL resources
	if (beamVAO_.isCreated()) {
		beamVAO_.destroy();
	}
	if (vboId_ != 0) {
		glDeleteBuffers(1, &vboId_);
		vboId_ = 0;
	}
	if (eboId_ != 0) {
		glDeleteBuffers(1, &eboId_);
		eboId_ = 0;
	}
	beamShaderProgram_.reset();
}

// Destructor
RadarBeam::~RadarBeam() {
	// OpenGL resources should already be cleaned up via cleanup() called from
	// RadarGLWidget::cleanupGL() before context destruction.
	// Note: beamShaderProgram_ should be nullptr at this point
}

void RadarBeam::uploadGeometryToGPU() {
	// Check for valid OpenGL context
	if (!QOpenGLContext::currentContext()) {
		// No GL context - mark geometry as dirty for later upload
		geometryDirty_ = true;
		return;
	}

	// Make sure VAO exists
	if (!beamVAO_.isCreated()) {
		geometryDirty_ = true;
		return;
	}

	// Don't upload empty geometry
	if (vertices_.empty() || indices_.empty()) {
		return;
	}

	beamVAO_.bind();

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

	beamVAO_.release();
	geometryDirty_ = false;
}

void RadarBeam::initialize() {
	// Make sure we don't initialize twice
	if (beamVAO_.isCreated()) {
		return;
	}

	if (!initializeOpenGLFunctions()) {
		qCritical() << "RadarBeam: Failed to initialize OpenGL functions!";
		return;
	}

	// Clear pending GL errors
	GLUtils::clearGLErrors();

	setupShaders();

	// Create VAO only - buffers will be created in uploadGeometryToGPU
	if (!beamVAO_.isCreated()) {
		beamVAO_.create();
	}
	beamVAO_.bind();
	beamVAO_.release();

	// Mark geometry as dirty - actual geometry will be created when position is set
	geometryDirty_ = true;

	// Check for errors after initialization
	GLUtils::checkGLError("RadarBeam::initialize");
}


void RadarBeam::setupShaders() {
	// Check if shader sources are properly initialized
	if (beamVertexShaderSource_.empty() || beamFragmentShaderSource_.empty()) {
		qCritical() << "Shader sources not initialized!";
		return;
	}

	// Create and compile beam shader program
	beamShaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

	// Debug shader compilation
	if (!beamShaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, beamVertexShaderSource_.data())) {
		qCritical() << "Failed to compile vertex shader:" << beamShaderProgram_->log();
		return;
	}

	if (!beamShaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, beamFragmentShaderSource_.data())) {
		qCritical() << "Failed to compile fragment shader:" << beamShaderProgram_->log();
		return;
	}

	if (!beamShaderProgram_->link()) {
		qCritical() << "Failed to link shader program:" << beamShaderProgram_->log();
		return;
	}
}

void RadarBeam::update(const QVector3D& radarPosition) {
	currentRadarPosition_ = radarPosition;
	createBeamGeometry();
	uploadGeometryToGPU();
}

void RadarBeam::render(QOpenGLShaderProgram* program, const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
	if (!visible_ || vertices_.empty()) {
		return;
	}

	// Check if OpenGL resources are valid
	if (!beamVAO_.isCreated() || !beamShaderProgram_) {
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

	// No stencil test - shadow occlusion handled via fragment shader cone test
	glDisable(GL_STENCIL_TEST);

	// Enable face culling - only render front faces of beam cone
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Use the beam shader program
	beamShaderProgram_->bind();

	// Set uniforms
	beamShaderProgram_->setUniformValue("projection", projection);
	beamShaderProgram_->setUniformValue("view", view);
	beamShaderProgram_->setUniformValue("model", model);
	beamShaderProgram_->setUniformValue("beamColor", color_);
	beamShaderProgram_->setUniformValue("opacity", opacity_);

	// Extract camera position from view matrix for Fresnel effect
	QMatrix4x4 inverseView = view.inverted();
	QVector3D cameraPos = inverseView.column(3).toVector3D();
	beamShaderProgram_->setUniformValue("viewPos", cameraPos);

	// Set GPU shadow map uniforms
	beamShaderProgram_->setUniformValue("radarPos", currentRadarPosition_);
	beamShaderProgram_->setUniformValue("gpuShadowEnabled", gpuShadowEnabled_);
	beamShaderProgram_->setUniformValue("beamAxis", beamAxis_);
	beamShaderProgram_->setUniformValue("beamWidthRad", beamWidthRadians_);
	beamShaderProgram_->setUniformValue("numRings", numRings_);

	// Bind shadow map texture if enabled
	if (gpuShadowEnabled_ && gpuShadowMapTexture_ != 0) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuShadowMapTexture_);
		beamShaderProgram_->setUniformValue("shadowMap", 0);
	}

	// Bind VAO and draw
	beamVAO_.bind();

	// Bind buffers explicitly
	glBindBuffer(GL_ARRAY_BUFFER, vboId_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
	

	// Release VAO
	if (beamVAO_.isCreated() && beamVAO_.objectId() != 0) {
		beamVAO_.release();
	}

	// Release shader program
	beamShaderProgram_->release();

	// Restore previous OpenGL state
	glDepthMask(depthMaskPrevious);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	if (!blendPrevious) {
		glDisable(GL_BLEND);
	}
	glBlendFunc(blendSrcPrevious, blendDstPrevious);
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

void RadarBeam::setGPUShadowMap(GLuint textureId) {
	gpuShadowMapTexture_ = textureId;
}

void RadarBeam::setGPUShadowEnabled(bool enabled) {
	gpuShadowEnabled_ = enabled;
}

void RadarBeam::setBeamAxis(const QVector3D& axis) {
	beamAxis_ = axis.normalized();
}

void RadarBeam::setBeamWidthRadians(float radians) {
	beamWidthRadians_ = radians;
}

void RadarBeam::setNumRings(int numRings) {
	numRings_ = numRings;
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
	float baseRadius = tan(beamWidthDegrees_ * kDegToRadF / 2.0f) * beamLength;

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
	const int segments = kBeamConeSegments;
	const int capRings = kBeamCapRings;  // Number of rings for the spherical cap

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
		float angle = kTwoPiF * i / segments;
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
