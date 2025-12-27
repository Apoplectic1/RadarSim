#include "SphereRenderer.h"
#include <QtMath>
#include <qtimer.h>


SphereRenderer::SphereRenderer(QObject* parent)
	: QObject(parent),
	radius_(100.0f),
	showSphere_(true),
	showGridLines_(true),
	showAxes_(true),
	initialized_(false),
	theta_(45.0f),
	phi_(45.0f),
	// Initialize OpenGL buffer types
	sphereVBO(QOpenGLBuffer::VertexBuffer),
	sphereEBO(QOpenGLBuffer::IndexBuffer),
	linesVBO(QOpenGLBuffer::VertexBuffer),
	dotVBO(QOpenGLBuffer::VertexBuffer),
	axesVBO(QOpenGLBuffer::VertexBuffer),
	// Initialize inertia-related members
	inertiaTimer_(new QTimer(this)),
	rotationAxis_(0, 1, 0),
	rotationVelocity_(0.0f),
	rotationDecay_(0.95f),
	inertiaEnabled_(true),
	rotation_(QQuaternion())
{
	// Initialize main shader sources
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

	dotVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 Normal;
        out vec3 FragPos;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

	dotFragmentShaderSource = R"(
        #version 330 core
        in vec3 Normal;
        in vec3 FragPos;

        uniform vec3 lightPos;
        uniform vec3 color;
        uniform float opacity;

        out vec4 outColor;

        void main() {
            // Simple lighting calculation
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
            vec3 ambient = vec3(0.3, 0.3, 0.3);
            vec3 result = (ambient + diffuse) * color;
            
            // Apply opacity
            outColor = vec4(result, opacity);
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
		delete shaderProgram;
		shaderProgram = nullptr;
		delete axesShaderProgram;
		axesShaderProgram = nullptr;
		delete dotShaderProgram;
		dotShaderProgram = nullptr;
		return;
	}

	qDebug() << "SphereRenderer::cleanup() - cleaning up OpenGL resources";

	// Clean up sphere resources
	if (sphereVAO.isCreated()) {
		sphereVAO.destroy();
	}
	if (sphereVBO.isCreated()) {
		sphereVBO.destroy();
	}
	if (sphereEBO.isCreated()) {
		sphereEBO.destroy();
	}

	// Clean up grid lines resources
	if (linesVAO.isCreated()) {
		linesVAO.destroy();
	}
	if (linesVBO.isCreated()) {
		linesVBO.destroy();
	}

	// Clean up axes resources
	if (axesVAO.isCreated()) {
		axesVAO.destroy();
	}
	if (axesVBO.isCreated()) {
		axesVBO.destroy();
	}

	// Clean up dot resources
	if (dotVAO.isCreated()) {
		dotVAO.destroy();
	}
	if (dotVBO.isCreated()) {
		dotVBO.destroy();
	}

	// Clean up shader programs
	delete shaderProgram;
	shaderProgram = nullptr;
	delete axesShaderProgram;
	axesShaderProgram = nullptr;
	delete dotShaderProgram;
	dotShaderProgram = nullptr;

	initialized_ = false;
	qDebug() << "SphereRenderer::cleanup() complete";
}

SphereRenderer::~SphereRenderer() {
	// Clean up inertia timer
	if (inertiaTimer_) {
		inertiaTimer_->stop();
		delete inertiaTimer_;
		inertiaTimer_ = nullptr;
	}

	// OpenGL resources should already be cleaned up via cleanup() called from
	// RadarGLWidget::cleanupGL() before context destruction.
	// Only clean up non-GL resources here.

	// If shader programs still exist, delete them (safe without GL context)
	if (shaderProgram) {
		delete shaderProgram;
		shaderProgram = nullptr;
	}
	if (axesShaderProgram) {
		delete axesShaderProgram;
		axesShaderProgram = nullptr;
	}
	if (dotShaderProgram) {
		delete dotShaderProgram;
		dotShaderProgram = nullptr;
	}

	qDebug() << "SphereRenderer destructor called";
}

bool SphereRenderer::initialize() {
	qDebug() << "Starting initialization";

	// Initialize OpenGL functions
	initializeOpenGLFunctions();

	// Set up OpenGL
	glEnable(GL_DEPTH_TEST);

	// Initialize shaders using the new initializeShaders method
	if (!initializeShaders()) {
		qWarning() << "Shader initialization failed";
		return false;
	}

	// Create geometry
	createSphere();
	createGridLines();
	createAxesLines();
	createDot();

	// Mark as initialized
	initialized_ = true;

	qDebug() << "Initialization complete";
	return true;
}

bool SphereRenderer::initializeShaders() {
	// Clean up existing shader programs to prevent memory leaks
	if (shaderProgram) {
		delete shaderProgram;
		shaderProgram = nullptr;
	}

	if (axesShaderProgram) {
		delete axesShaderProgram;
		axesShaderProgram = nullptr;
	}

	if (dotShaderProgram) {
		delete dotShaderProgram;
		dotShaderProgram = nullptr;
	}

	// Create main shader program
	shaderProgram = new QOpenGLShaderProgram();
	// (existing shader code)

	// Create the dot shader program
	dotShaderProgram = new QOpenGLShaderProgram();

	if (!dotShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, dotVertexShaderSource)) {
		qWarning() << "Failed to compile dot vertex shader:" << dotShaderProgram->log();
		return false;
	}

	if (!dotShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, dotFragmentShaderSource)) {
		qWarning() << "Failed to compile dot fragment shader:" << dotShaderProgram->log();
		return false;
	}

	if (!dotShaderProgram->link()) {
		qWarning() << "Failed to link dot shader program:" << dotShaderProgram->log();
		return false;
	}

	// Load and compile vertex shader
	if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
		qWarning() << "Failed to compile vertex shader:" << shaderProgram->log();
		return false;
	}

	// Load and compile fragment shader
	if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
		qWarning() << "Failed to compile fragment shader:" << shaderProgram->log();
		return false;
	}

	// Link shader program
	if (!shaderProgram->link()) {
		qWarning() << "Failed to link main shader program:" << shaderProgram->log();
		return false;
	}

	// Create axes shader program
	axesShaderProgram = new QOpenGLShaderProgram();

	// Load and compile axes vertex shader
	if (!axesShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, axesVertexShaderSource)) {
		qWarning() << "Failed to compile axes vertex shader:" << axesShaderProgram->log();
		return false;
	}

	// Load and compile axes fragment shader
	if (!axesShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, axesFragmentShaderSource)) {
		qWarning() << "Failed to compile axes fragment shader:" << axesShaderProgram->log();
		return false;
	}

	// Link axes shader program
	if (!axesShaderProgram->link()) {
		qWarning() << "Failed to link axes shader program:" << axesShaderProgram->log();
		return false;
	}

	return true;
}

void SphereRenderer::createDot() {
	// Create a small sphere for the dot
	dotVertices.clear();

	float dotRadius = 5.0f;
	int segments = 16;

	// Generate icosphere vertices for the dot (more efficient than full sphere)
	// This is a simplified version - a proper icosphere would be better
	std::vector<QVector3D> positions;

	// Start with icosahedron vertices
	float t = (1.0f + sqrt(5.0f)) / 2.0f;

	positions.push_back(QVector3D(-1, t, 0).normalized() * dotRadius);
	positions.push_back(QVector3D(1, t, 0).normalized() * dotRadius);
	positions.push_back(QVector3D(-1, -t, 0).normalized() * dotRadius);
	positions.push_back(QVector3D(1, -t, 0).normalized() * dotRadius);

	positions.push_back(QVector3D(0, -1, t).normalized() * dotRadius);
	positions.push_back(QVector3D(0, 1, t).normalized() * dotRadius);
	positions.push_back(QVector3D(0, -1, -t).normalized() * dotRadius);
	positions.push_back(QVector3D(0, 1, -t).normalized() * dotRadius);

	positions.push_back(QVector3D(t, 0, -1).normalized() * dotRadius);
	positions.push_back(QVector3D(t, 0, 1).normalized() * dotRadius);
	positions.push_back(QVector3D(-t, 0, -1).normalized() * dotRadius);
	positions.push_back(QVector3D(-t, 0, 1).normalized() * dotRadius);

	// Define the triangles of the icosahedron
	std::vector<std::array<int, 3>> faces = {
		{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
		{1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
		{3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
		{4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
	};

	// Create vertex data for dot
	for (const auto& face : faces) {
		for (int idx : face) {
			QVector3D pos = positions[idx];
			QVector3D normal = pos.normalized();

			dotVertices.push_back(pos.x());
			dotVertices.push_back(pos.y());
			dotVertices.push_back(pos.z());
			dotVertices.push_back(normal.x());
			dotVertices.push_back(normal.y());
			dotVertices.push_back(normal.z());
		}
	}

	// Set up VAO and VBO for dot
	dotVAO.create();
	dotVAO.bind();

	dotVBO.create();
	dotVBO.bind();
	dotVBO.allocate(dotVertices.data(), dotVertices.size() * sizeof(float));

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	dotVAO.release();
}

void SphereRenderer::render(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
	// Create a local copy of the model matrix that we can modify
	QMatrix4x4 localModel = model;

	// Apply our stored rotation to the model matrix
	localModel.rotate(rotation_);

#ifdef QT_DEBUG
	// Single shader validation check
	if (!shaderProgram || !axesShaderProgram || !dotShaderProgram ||
		!shaderProgram->isLinked() || !axesShaderProgram->isLinked() || !dotShaderProgram->isLinked()) {
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
		if (currentShader && currentShader != shaderProgram) {
			currentShader->release();
			currentShader = nullptr;
		}

		// Bind shader if not already bound
		if (currentShader != shaderProgram) {
			shaderProgram->bind();
			currentShader = shaderProgram;

			// Set common uniforms only when binding
			shaderProgram->setUniformValue("projection", projection);
			shaderProgram->setUniformValue("view", view);
			shaderProgram->setUniformValue("model", localModel);  // Use the rotated model
		}

		// Set sphere-specific uniforms
		shaderProgram->setUniformValue("color", QVector3D(0.95f, 0.95f, 0.95f));
		shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, -500.0f, 500.0f));

		// Enable blending for transparency
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);  // Don't write to depth buffer for transparent sphere

		sphereVAO.bind();
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);

		// Pass 1: Draw back faces (darker, more transparent for depth effect)
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);  // Cull front faces, draw back faces
		shaderProgram->setUniformValue("opacity", 0.20f);
		glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

		// Pass 2: Draw front faces (lighter, less transparent)
		glCullFace(GL_BACK);  // Cull back faces, draw front faces
		shaderProgram->setUniformValue("opacity", 0.35f);
		glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

		glDisable(GL_POLYGON_OFFSET_FILL);
		sphereVAO.release();

		glDisable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);  // Re-enable depth writing
		glDisable(GL_BLEND);
	}

	// 2. Draw grid lines if visible
	if (showGridLines_) {
		// Release any currently bound shader that's not the one we need
		if (currentShader && currentShader != shaderProgram) {
			currentShader->release();
			currentShader = nullptr;
		}

		// Bind shader if not already bound
		if (currentShader != shaderProgram) {
			shaderProgram->bind();
			currentShader = shaderProgram;

			// Set common uniforms only when binding
			shaderProgram->setUniformValue("projection", projection);
			shaderProgram->setUniformValue("view", view);
			shaderProgram->setUniformValue("model", localModel);  // Use the rotated model
		}

		// Always update light position for grid lines
		shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, -500.0f, 500.0f));

		linesVAO.bind();

		// Enable line smoothing for better appearance
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		// Use LEQUAL depth function to prevent z-fighting with the sphere
		glDepthFunc(GL_LEQUAL);

		// Regular latitude lines
		glLineWidth(1.0f);
		shaderProgram->setUniformValue("color", QVector3D(0.4f, 0.4f, 0.4f));

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

		// Equator (green) - match SphereWidget
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.9f, 0.0f));
		glDrawArrays(GL_LINE_STRIP, equatorStartIndex, 72 + 1);

		// Prime Meridian (blue) - match SphereWidget
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 0.9f));
		glDrawArrays(GL_LINE_STRIP, primeMeridianStartIndex, 50 + 1);

		// Restore default depth function
		glDepthFunc(GL_LESS);
		glDisable(GL_LINE_SMOOTH);
		linesVAO.release();
	}

	// 3. Draw coordinate axes if visible
	if (showAxes_) {
		// Release any currently bound shader that's not the one we need
		if (currentShader && currentShader != axesShaderProgram) {
			currentShader->release();
			currentShader = nullptr;
		}

		// Bind shader if not already bound
		if (currentShader != axesShaderProgram) {
			axesShaderProgram->bind();
			currentShader = axesShaderProgram;
		}

		axesShaderProgram->setUniformValue("projection", projection);
		axesShaderProgram->setUniformValue("view", view);
		axesShaderProgram->setUniformValue("model", localModel);  // Use the rotated model

		axesVAO.bind();

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

		axesVAO.release();

		// Release axesShaderProgram before switching to another shader
		if (currentShader == axesShaderProgram) {
			currentShader->release();
			currentShader = nullptr;
		}
	}

	// 4. Draw radar dot
	// Calculate dot position in spherical coordinates
	QVector3D dotPos = sphericalToCartesian(radius_, theta_, phi_);

	// Create a separate model matrix for the dot that includes our rotation
	QMatrix4x4 dotModelMatrix = localModel;
	dotModelMatrix.translate(dotPos);

	// 4.1. First, draw opaque parts of the dot (front-facing parts)
	// Switch to dot shader program
	if (currentShader && currentShader != dotShaderProgram) {
		currentShader->release();
		currentShader = nullptr;
	}

	dotShaderProgram->bind();
	currentShader = dotShaderProgram;

	// Ensure proper depth testing for opaque rendering
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	dotShaderProgram->setUniformValue("projection", projection);
	dotShaderProgram->setUniformValue("view", view);
	dotShaderProgram->setUniformValue("model", dotModelMatrix);
	dotShaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));  // Red dot
	dotShaderProgram->setUniformValue("lightPos", QVector3D(500.0f, -500.0f, 500.0f));
	dotShaderProgram->setUniformValue("opacity", 1.0f);  // Fully opaque

	dotVAO.bind();

	// Only draw front-facing polygons in this pass
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDrawArrays(GL_TRIANGLES, 0, dotVertices.size() / 6);
	glDisable(GL_CULL_FACE);

	dotVAO.release();

	// 4.2. Then, draw transparent parts of the dot (back-facing parts)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);  // Don't write to depth buffer for transparent parts

	dotShaderProgram->setUniformValue("opacity", 0.2f);  // Partially transparent

	dotVAO.bind();

	// Draw with special depth test for transparency
	glDepthFunc(GL_GREATER);  // Only draw if behind other objects

	glDrawArrays(GL_TRIANGLES, 0, dotVertices.size() / 6);

	// Restore default depth function and settings
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	dotVAO.release();

	if (currentShader == dotShaderProgram) {
		currentShader->release();
		currentShader = nullptr;
	}

	// Note: Beam rendering is handled by BeamController

	// Final cleanup - ensure all shaders are released
	if (currentShader) {
		currentShader->release();
	}
}

void SphereRenderer::setRadarPosition(float theta, float phi) {
	theta_ = theta;
	phi_ = phi;
}

QVector3D SphereRenderer::getRadarPosition() const {
	return sphericalToCartesian(radius_, theta_, phi_);
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
			createDot();
		}

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

void SphereRenderer::createAxesLines() {
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

			// Position (Z-up convention)
			float x = radius_ * sinPhi * cosTheta;
			float y = radius_ * sinPhi * sinTheta;  // Y is now horizontal
			float z = radius_ * cosPhi;             // Z is now vertical

			// Normal (normalized position for a sphere)
			float nx = sinPhi * cosTheta;
			float ny = sinPhi * sinTheta;  // Y is now horizontal
			float nz = cosPhi;             // Z is now vertical

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
	if (!sphereVAO.isCreated()) {
		sphereVAO.create();
	}
	sphereVAO.bind();

	if (!sphereVBO.isCreated()) {
		sphereVBO.create();
	}
	sphereVBO.bind();
	sphereVBO.allocate(sphereVertices.data(), sphereVertices.size() * sizeof(float));

	if (!sphereEBO.isCreated()) {
		sphereEBO.create();
	}
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

void SphereRenderer::createGridLines() {
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
		float z = gridRadiusOffset * sin(phi);  // Z is now vertical
		float rLat = gridRadiusOffset * cos(phi);  // Use offset radius

		// Create a complete circle for this latitude
		for (int i = 0; i <= latitudeSegments; i++) {
			float theta = 2.0f * M_PI * float(i) / float(latitudeSegments);
			float x = rLat * cos(theta);
			float y = rLat * sin(theta);  // Y is now horizontal

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
			float z = gridRadiusOffset * sin(phi);  // Z is now vertical
			float x = rLat * cos(theta);
			float y = rLat * sin(theta);  // Y is now horizontal

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

// Helper method for spherical to cartesian conversion (Z-up convention)
QVector3D SphereRenderer::sphericalToCartesian(float r, float thetaDeg, float phiDeg) const {
	const float toRad = float(M_PI / 180.0);
	float theta = thetaDeg * toRad;
	float phi = phiDeg * toRad;
	return QVector3D(
		r * cos(phi) * cos(theta),
		r * cos(phi) * sin(theta),  // Y is now horizontal
		r * sin(phi)                // Z is now vertical (elevation)
	);
}

void SphereRenderer::startInertia(const QVector3D& axis, float velocity) {
	if (!inertiaEnabled_)
		return;

	rotationAxis_ = axis.normalized();
	rotationVelocity_ = velocity;

	// Start timer if not already running
	if (!inertiaTimer_->isActive()) {
		inertiaTimer_->start(16); // ~60 FPS
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
