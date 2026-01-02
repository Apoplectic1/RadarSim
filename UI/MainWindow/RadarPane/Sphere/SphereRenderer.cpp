#include "SphereRenderer.h"
#include "GLUtils.h"
#include "Constants.h"
#include <QtMath>
#include <qtimer.h>

using namespace RS::Constants;

SphereRenderer::SphereRenderer(QObject* parent)
	: QObject(parent),
	radius_(Defaults::kSphereRadius),
	showSphere_(true),
	showGridLines_(true),
	showAxes_(true),
	initialized_(false),
	// Initialize OpenGL buffer types
	sphereVBO_(QOpenGLBuffer::VertexBuffer),
	sphereEBO_(QOpenGLBuffer::IndexBuffer),
	linesVBO_(QOpenGLBuffer::VertexBuffer),
	axesVBO_(QOpenGLBuffer::VertexBuffer),
	// Initialize inertia-related members
	inertiaTimer_(new QTimer(this)),
	rotationAxis_(0, 1, 0),
	rotationVelocity_(0.0f),
	rotationDecay_(0.95f),
	inertiaEnabled_(true),
	rotation_(QQuaternion())
{
	// Initialize main shader sources
	vertexShaderSource_ = R"(
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

	fragmentShaderSource_ = R"(
        #version 330 core
        in vec3 FragColor;
        in vec3 Normal;
        in vec3 FragPos;

        uniform vec3 lightPos;
        uniform float opacity;

        out vec4 outColor;

        void main() {
            // Simple lighting calculation
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
            vec3 ambient = vec3(0.3, 0.3, 0.3);
            vec3 result = (ambient + diffuse) * FragColor;
            outColor = vec4(result, opacity);
        }
    )";

	// Initialize axes shader sources
	axesVertexShaderSource_ = R"(
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

	axesFragmentShaderSource_ = R"(
        #version 330 core
        in vec3 ourColor;
        out vec4 FragColor;

        void main() {
            FragColor = vec4(ourColor, 1.0);
        }
    )";

	// Connect inertia timer to update slot
	connect(inertiaTimer_, &QTimer::timeout, this, &SphereRenderer::updateInertia);

	// Start frame timer
	frameTimer_.start();
}

// Clean up OpenGL resources - must be called with valid GL context
void SphereRenderer::cleanup() {
	// Check if we were ever initialized
	if (!initialized_) {
		return;
	}

	// Check for valid OpenGL context
	QOpenGLContext* ctx = QOpenGLContext::currentContext();
	if (!ctx) {
		qWarning() << "SphereRenderer::cleanup() - no current context, skipping GL cleanup";
		// Still clean up shader programs (safe without context)
		shaderProgram_.reset();
		axesShaderProgram_.reset();
		return;
	}

	// Clean up sphere resources
	if (sphereVAO_.isCreated()) {
		sphereVAO_.destroy();
	}
	if (sphereVBO_.isCreated()) {
		sphereVBO_.destroy();
	}
	if (sphereEBO_.isCreated()) {
		sphereEBO_.destroy();
	}

	// Clean up grid lines resources
	if (linesVAO_.isCreated()) {
		linesVAO_.destroy();
	}
	if (linesVBO_.isCreated()) {
		linesVBO_.destroy();
	}

	// Clean up axes resources
	if (axesVAO_.isCreated()) {
		axesVAO_.destroy();
	}
	if (axesVBO_.isCreated()) {
		axesVBO_.destroy();
	}

	// Clean up shader programs
	shaderProgram_.reset();
	axesShaderProgram_.reset();

	initialized_ = false;
}

SphereRenderer::~SphereRenderer() {
	// Clean up inertia timer (parented to this, but stop it first)
	if (inertiaTimer_) {
		inertiaTimer_->stop();
	}
	// OpenGL resources should already be cleaned up via cleanup() called from
	// RadarGLWidget::cleanupGL() before context destruction.
	// Note: shader pointers should be nullptr at this point
}

bool SphereRenderer::initialize() {
	// Initialize OpenGL functions
	if (!initializeOpenGLFunctions()) {
		qCritical() << "SphereRenderer: Failed to initialize OpenGL functions!";
		return false;
	}

	// Clear any pending GL errors
	GLUtils::clearGLErrors();

	// Set up OpenGL
	glEnable(GL_DEPTH_TEST);

	// Initialize shaders using the new initializeShaders method
	if (!initializeShaders()) {
		qWarning() << "Shader initialization failed";
		return false;
	}

	// Check for errors after shader init
	GLUtils::checkGLError("SphereRenderer::initialize after shaders");

	// Create geometry
	createSphere();
	createGridLines();
	createAxesLines();

	// Mark as initialized
	initialized_ = true;
	return true;
}

bool SphereRenderer::initializeShaders() {
	// Clean up existing shader programs to prevent memory leaks
	shaderProgram_.reset();
	axesShaderProgram_.reset();

	// Create main shader program
	shaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

	// Load and compile vertex shader
	if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource_.data())) {
		qWarning() << "Failed to compile vertex shader:" << shaderProgram_->log();
		return false;
	}

	// Load and compile fragment shader
	if (!shaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource_.data())) {
		qWarning() << "Failed to compile fragment shader:" << shaderProgram_->log();
		return false;
	}

	// Link shader program
	if (!shaderProgram_->link()) {
		qWarning() << "Failed to link main shader program:" << shaderProgram_->log();
		return false;
	}

	// Create axes shader program
	axesShaderProgram_ = std::make_unique<QOpenGLShaderProgram>();

	// Load and compile axes vertex shader
	if (!axesShaderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, axesVertexShaderSource_.data())) {
		qWarning() << "Failed to compile axes vertex shader:" << axesShaderProgram_->log();
		return false;
	}

	// Load and compile axes fragment shader
	if (!axesShaderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, axesFragmentShaderSource_.data())) {
		qWarning() << "Failed to compile axes fragment shader:" << axesShaderProgram_->log();
		return false;
	}

	// Link axes shader program
	if (!axesShaderProgram_->link()) {
		qWarning() << "Failed to link axes shader program:" << axesShaderProgram_->log();
		return false;
	}

	return true;
}

void SphereRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
	// Create a local copy of the model matrix that we can modify
	QMatrix4x4 localModel = model;

	// Apply our stored rotation to the model matrix
	localModel.rotate(rotation_);

#ifdef QT_DEBUG
	// Single shader validation check
	if (!shaderProgram_ || !axesShaderProgram_ ||
		!shaderProgram_->isLinked() || !axesShaderProgram_->isLinked()) {
		qCritical() << "ERROR: render called with invalid shaders";
	}
#endif

	// Basic OpenGL state setup
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	// Track which shader is currently bound
	QOpenGLShaderProgram* currentShader = nullptr;

	// 1. Draw the sphere with two-pass transparency for 3D effect
	if (showSphere_) {
		// Release any currently bound shader that's not the one we need
		if (currentShader && currentShader != shaderProgram_.get()) {
			currentShader->release();
			currentShader = nullptr;
		}

		// Bind shader if not already bound
		if (currentShader != shaderProgram_.get()) {
			shaderProgram_->bind();
			currentShader = shaderProgram_.get();

			// Set common uniforms only when binding
			shaderProgram_->setUniformValue("projection", projection);
			shaderProgram_->setUniformValue("view", view);
			shaderProgram_->setUniformValue("model", localModel);  // Use the rotated model
		}

		// Set sphere-specific uniforms
		shaderProgram_->setUniformValue("color", QVector3D(Colors::kSphereOffWhite[0], Colors::kSphereOffWhite[1], Colors::kSphereOffWhite[2]));
		shaderProgram_->setUniformValue("lightPos", QVector3D(Lighting::kLightPosition[0], Lighting::kLightPosition[1], Lighting::kLightPosition[2]));

		// Enable blending for transparency
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);  // Don't write to depth buffer for transparent sphere

		sphereVAO_.bind();
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);

		// Pass 1: Draw back faces (darker, more transparent for depth effect)
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);  // Cull front faces, draw back faces
		shaderProgram_->setUniformValue("opacity", 0.20f);
		glDrawElements(GL_TRIANGLES, sphereIndices_.size(), GL_UNSIGNED_INT, 0);

		// Pass 2: Draw front faces (lighter, less transparent)
		glCullFace(GL_BACK);  // Cull back faces, draw front faces
		shaderProgram_->setUniformValue("opacity", 0.35f);
		glDrawElements(GL_TRIANGLES, sphereIndices_.size(), GL_UNSIGNED_INT, 0);

		glDisable(GL_POLYGON_OFFSET_FILL);
		sphereVAO_.release();

		glDisable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);  // Re-enable depth writing
		glDisable(GL_BLEND);
	}

	// 2. Draw grid lines if visible
	if (showGridLines_) {
		// Release any currently bound shader that's not the one we need
		if (currentShader && currentShader != shaderProgram_.get()) {
			currentShader->release();
			currentShader = nullptr;
		}

		// Bind shader if not already bound
		if (currentShader != shaderProgram_.get()) {
			shaderProgram_->bind();
			currentShader = shaderProgram_.get();

			// Set common uniforms only when binding
			shaderProgram_->setUniformValue("projection", projection);
			shaderProgram_->setUniformValue("view", view);
			shaderProgram_->setUniformValue("model", localModel);  // Use the rotated model
		}

		// Always update light position for grid lines
		shaderProgram_->setUniformValue("lightPos", QVector3D(Lighting::kLightPosition[0], Lighting::kLightPosition[1], Lighting::kLightPosition[2]));

		linesVAO_.bind();

		// Enable line smoothing for better appearance
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		// Use LEQUAL depth function to prevent z-fighting with the sphere
		glDepthFunc(GL_LEQUAL);

		// Regular latitude lines
		glLineWidth(kGridLineWidthNormal);
		shaderProgram_->setUniformValue("color", QVector3D(Colors::kGridLineGrey[0], Colors::kGridLineGrey[1], Colors::kGridLineGrey[2]));

		// Draw latitude lines
		for (int lat = 0; lat < latitudeLineCount_; lat++) {
			int startIdx = lat * (kSphereLatSegments + 1);
			if (startIdx == equatorStartIndex_) {
				continue; // Skip equator, drawn separately
			}
			glDrawArrays(GL_LINE_STRIP, startIdx, kSphereLatSegments + 1);
		}

		// Draw longitude lines
		int longitudeStartOffset = latitudeLineCount_ * (kSphereLatSegments + 1);
		for (int lon = 0; lon < longitudeLineCount_; lon++) {
			int startIdx = longitudeStartOffset + lon * (kSphereLongSegments + 1);
			if (startIdx == primeMeridianStartIndex_) {
				continue; // Skip prime meridian, drawn separately
			}
			glDrawArrays(GL_LINE_STRIP, startIdx, kSphereLongSegments + 1);
		}

		// Special lines (equator and prime meridian)
		glLineWidth(kGridLineWidthSpecial);

		// Equator (green)
		shaderProgram_->setUniformValue("color", QVector3D(Colors::kEquatorGreen[0], Colors::kEquatorGreen[1], Colors::kEquatorGreen[2]));
		glDrawArrays(GL_LINE_STRIP, equatorStartIndex_, kSphereLatSegments + 1);

		// Prime Meridian (blue)
		shaderProgram_->setUniformValue("color", QVector3D(Colors::kPrimeMeridianRed[0], Colors::kPrimeMeridianRed[1], Colors::kPrimeMeridianRed[2]));
		glDrawArrays(GL_LINE_STRIP, primeMeridianStartIndex_, kSphereLongSegments + 1);

		// Restore default depth function
		glDepthFunc(GL_LESS);
		glDisable(GL_LINE_SMOOTH);
		linesVAO_.release();
	}

	// 3. Draw coordinate axes if visible
	if (showAxes_) {
		// Release any currently bound shader that's not the one we need
		if (currentShader && currentShader != axesShaderProgram_.get()) {
			currentShader->release();
			currentShader = nullptr;
		}

		// Bind shader if not already bound
		if (currentShader != axesShaderProgram_.get()) {
			axesShaderProgram_->bind();
			currentShader = axesShaderProgram_.get();
		}

		axesShaderProgram_->setUniformValue("projection", projection);
		axesShaderProgram_->setUniformValue("view", view);
		axesShaderProgram_->setUniformValue("model", localModel);  // Use the rotated model

		axesVAO_.bind();

		// Enable line smoothing
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		// Enable polygon offset to prevent z-fighting
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

		axesVAO_.release();

		// Release axesShaderProgram_ before switching to another shader
		if (currentShader == axesShaderProgram_.get()) {
			currentShader->release();
			currentShader = nullptr;
		}
	}

	// Note: Radar site dot rendering is handled by RadarSiteRenderer
	// Note: Beam rendering is handled by BeamController

	// Final cleanup - ensure all shaders are released
	if (currentShader) {
		currentShader->release();
	}
}

void SphereRenderer::setRadius(float radius) {
	if (radius_ != radius) {
		radius_ = radius;

		// Only regenerate if already initialized
		if (initialized_) {
			// Regenerate geometry
			createSphere();
			createGridLines();
			createAxesLines();
		}

		emit radiusChanged(radius);
	}
}

void SphereRenderer::setSphereVisible(bool visible) {
	if (showSphere_ != visible) {
		showSphere_ = visible;
	}
}

void SphereRenderer::setGridLinesVisible(bool visible) {
	if (showGridLines_ != visible) {
		showGridLines_ = visible;
	}
}

void SphereRenderer::createAxesLines() {
	// Create a local vector to collect all vertices
	std::vector<float> vertices;

	float axisLength = radius_ * View::kAxisLengthMultiplier;   // Make axes slightly longer than radius
	float arrowLength = radius_ * 0.06f; // Length of the conical arrowhead
	float arrowRadius = radius_ * 0.02f; // Radius of the conical arrowhead base
	int segments = kAxisArrowSegments;   // Number of segments for the cone

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
			float angle = kTwoPiF * i / segments;
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
	axesVertices_ = vertices;

	// Set up VAO and VBO for axes
	if (!axesVAO_.isCreated()) {
		axesVAO_.create();
	}
	axesVAO_.bind();

	if (!axesVBO_.isCreated()) {
		axesVBO_.create();
	}
	axesVBO_.bind();
	axesVBO_.allocate(axesVertices_.data(), axesVertices_.size() * sizeof(float));

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	axesVAO_.release();
}

void SphereRenderer::setAxesVisible(bool visible) {
	if (showAxes_ != visible) {
		showAxes_ = visible;
	}
}

void SphereRenderer::createSphere(int latDivisions, int longDivisions) {
	// Clear previous data
	sphereVertices_.clear();
	sphereIndices_.clear();

	// Generate vertices
	for (int lat = 0; lat <= latDivisions; lat++) {
		float phi = kPiF * float(lat) / float(latDivisions);
		float sinPhi = sin(phi);
		float cosPhi = cos(phi);

		for (int lon = 0; lon <= longDivisions; lon++) {
			float theta = kTwoPiF * float(lon) / float(longDivisions);
			float sinTheta = sin(theta);
			float cosTheta = cos(theta);

			// Position (Z-up convention)
			float x = radius_ * sinPhi * cosTheta;
			float y = radius_ * sinPhi * sinTheta;  // Y is now horizontal
			float z = radius_ * cosPhi;             // Z is now vertical

			// Normal (normalized position for a sphere)
			float nx = sinPhi * cosTheta;
			float ny = sinPhi * sinTheta;  // Y is now horizontal
			float nz = cosPhi;             // Z is now vertical

			// Add vertex data
			sphereVertices_.push_back(x);
			sphereVertices_.push_back(y);
			sphereVertices_.push_back(z);
			sphereVertices_.push_back(nx);
			sphereVertices_.push_back(ny);
			sphereVertices_.push_back(nz);
		}
	}

	// Generate indices
	for (int lat = 0; lat < latDivisions; lat++) {
		for (int lon = 0; lon < longDivisions; lon++) {
			int first = lat * (longDivisions + 1) + lon;
			int second = first + longDivisions + 1;

			// First triangle
			sphereIndices_.push_back(first);
			sphereIndices_.push_back(second);
			sphereIndices_.push_back(first + 1);

			// Second triangle
			sphereIndices_.push_back(second);
			sphereIndices_.push_back(second + 1);
			sphereIndices_.push_back(first + 1);
		}
	}

	// Set up VAO and VBO for sphere
	if (!sphereVAO_.isCreated()) {
		sphereVAO_.create();
	}
	sphereVAO_.bind();

	if (!sphereVBO_.isCreated()) {
		sphereVBO_.create();
	}
	sphereVBO_.bind();
	sphereVBO_.allocate(sphereVertices_.data(), sphereVertices_.size() * sizeof(float));

	if (!sphereEBO_.isCreated()) {
		sphereEBO_.create();
	}
	sphereEBO_.bind();
	sphereEBO_.allocate(sphereIndices_.data(), sphereIndices_.size() * sizeof(unsigned int));


	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	sphereVAO_.release();
}

void SphereRenderer::createGridLines() {
	// Similar to before, but with a small offset to the grid lines
	latLongLines_.clear();

	// Constants for grid generation
	const float degToRad = kDegToRadF;
	const int latitudeSegments = kSphereLatSegments;  // Higher number for smoother circles
	const int longitudeSegments = kSphereLongSegments; // Higher number for smoother arcs

	// Add a small radius offset to ensure grid lines render above the sphere surface
	const float gridRadiusOffset = radius_ * kGridRadiusOffset;

	// Track our progress in the vertex array
	int vertexOffset = 0;

	// -------------------- LATITUDE LINES --------------------
	// Reset counter for debugging
	latitudeLineCount_ = 0;

	// Generate latitude lines one by one
	for (int phiDeg = -75; phiDeg <= 75; phiDeg += 15) {
		// Increment counter
		latitudeLineCount_++;

		// Mark equator for special rendering
		if (phiDeg == 0) {
			equatorStartIndex_ = vertexOffset / 3;
		}

		float phi = phiDeg * degToRad;
		float z = gridRadiusOffset * sin(phi);  // Z is now vertical
		float rLat = gridRadiusOffset * cos(phi);  // Use offset radius

		// Create a complete circle for this latitude
		for (int i = 0; i <= latitudeSegments; i++) {
			float theta = kTwoPiF * float(i) / float(latitudeSegments);
			float x = rLat * cos(theta);
			float y = rLat * sin(theta);  // Y is now horizontal

			// Add vertex
			latLongLines_.push_back(x);
			latLongLines_.push_back(y);
			latLongLines_.push_back(z);

			vertexOffset += 3;
		}
	}

	// -------------------- LONGITUDE LINES --------------------
	// Reset counter for debugging
	longitudeLineCount_ = 0;

	// Generate longitude lines one by one
	for (int thetaDeg = 0; thetaDeg <= 345; thetaDeg += 15) {
		// Increment counter
		longitudeLineCount_++;

		// Mark prime meridian for special rendering
		if (thetaDeg == 0) {
			primeMeridianStartIndex_ = vertexOffset / 3;
		}

		float theta = thetaDeg * degToRad;

		// Create a line from south pole to north pole
		for (int i = 0; i <= longitudeSegments; i++) {
			// Generate points from south pole to north pole
			float phi = kPiF * float(i) / float(longitudeSegments) - kPiF / 2.0f;
			float rLat = gridRadiusOffset * cos(phi);  // Use offset radius
			float z = gridRadiusOffset * sin(phi);  // Z is now vertical
			float x = rLat * cos(theta);
			float y = rLat * sin(theta);  // Y is now horizontal

			// Add vertex
			latLongLines_.push_back(x);
			latLongLines_.push_back(y);
			latLongLines_.push_back(z);

			vertexOffset += 3;
		}
	}

	// Set up VAO and VBO for lines
	if (!linesVAO_.isCreated()) {
		linesVAO_.create();
	}
	linesVAO_.bind();

	if (!linesVBO_.isCreated()) {
		linesVBO_.create();
	}
	linesVBO_.bind();
	linesVBO_.allocate(latLongLines_.data(), latLongLines_.size() * sizeof(float));

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	linesVAO_.release();
}

void SphereRenderer::startInertia(const QVector3D& axis, float velocity) {
	if (!inertiaEnabled_)
		return;

	rotationAxis_ = axis.normalized();
	rotationVelocity_ = velocity;

	// Start timer if not already running
	if (!inertiaTimer_->isActive()) {
		inertiaTimer_->start(kCameraInertiaTimerMs);
	}
}

void SphereRenderer::stopInertia() {
	inertiaTimer_->stop();
	rotationVelocity_ = 0.0f;
}

void SphereRenderer::updateInertia() {
	// Apply rotation based on velocity and time
	if (rotationVelocity_ > 0.05f) { // Stop when velocity gets very small
		// Apply rotation based on current velocity
		QQuaternion inertiaRotation = QQuaternion::fromAxisAndAngle(
			rotationAxis_, rotationVelocity_);

		// Update the rotation
		rotation_ = inertiaRotation * rotation_;

		// Decay the velocity
		rotationVelocity_ *= rotationDecay_;

		// Signal that the view has changed - this will trigger a redraw in the parent
		emit radiusChanged(radius_); // Reuse existing signal as a view change notification
	}
	else {
		// Stop the timer when velocity is negligible
		stopInertia();
	}
}

void SphereRenderer::setInertiaEnabled(bool enabled) {
	inertiaEnabled_ = enabled;

	// If disabling inertia, stop any ongoing inertia
	if (!enabled) {
		stopInertia();
	}
}

void SphereRenderer::setInertiaParameters(float decay, float velocityScale) {
	// Clamp decay value between 0.8 and 0.99
	rotationDecay_ = qBound(0.8f, decay, 0.99f);

	// This would be used when calculating initial velocity from input
	// Not used directly in this implementation but provided for API completeness
}

void SphereRenderer::applyRotation(const QVector3D& axis, float angle, bool withInertia) {
	// Create quaternion from axis and angle
	QQuaternion newRotation = QQuaternion::fromAxisAndAngle(axis, angle);

	// Apply the rotation
	rotation_ = newRotation * rotation_;

	// Start inertia if requested
	if (withInertia && inertiaEnabled_) {
		float dt = frameTimer_.elapsed() / 1000.0f; // Convert to seconds
		frameTimer_.restart();

		// Avoid division by zero
		if (dt < 0.001f) dt = 0.016f; // Default to ~60 FPS

		// Calculate velocity based on angle and time
		float velocity = angle / dt * 0.1f; // Scale down for smoother inertia

		startInertia(axis, velocity);
	}

	// Signal that the view has changed
	emit radiusChanged(radius_); // Reuse existing signal
}

void SphereRenderer::resetView() {
	// Stop inertia
	stopInertia();

	// Reset rotation to identity
	rotation_ = QQuaternion();

	// Signal that the view has changed
	emit radiusChanged(radius_);
}

void SphereRenderer::setRotation(const QQuaternion& rotation) {
	// Stop any ongoing inertia
	stopInertia();

	// Set new rotation
	rotation_ = rotation;

	// Signal that the view has changed
	emit radiusChanged(radius_);
}
