// ---- SphereWidget.cpp ----

#include "SphereWidget.h"
#include "RadarBeam.h"
#include "ConicalBeam.h"
#include "EllipticalBeam.h"
#include "PhasedArrayBeam.h"
#include <QOpenGLContext>
#include <QActionGroup>  // For QActionGroup
#include <QMenu>         // For QMenu
#include <QAction>       // For QAction
#include <cmath>

// Constructor
SphereWidget::SphereWidget(QWidget* parent)
	: QOpenGLWidget(parent),
	sphereVBO(QOpenGLBuffer::VertexBuffer),
	sphereEBO(QOpenGLBuffer::IndexBuffer),
	linesVBO(QOpenGLBuffer::VertexBuffer),
	dotVBO(QOpenGLBuffer::VertexBuffer),
	axesVBO(QOpenGLBuffer::VertexBuffer),
	rotation_(QQuaternion()),
	inertiaTimer_(new QTimer(this)),
	contextMenu_(new QMenu(this)),
	showAxes_(true),  // Initialize axes as visible
	showSphere_(true),  // Initialize sphere as visible
	showGridLines_(true),  // Initialize grid lines as visible
	inertiaEnabled_(false),  // Initialize inertia as enabled
	showBeam_(true)

{
	// Set focus policy and mouse tracking (as before)
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(false);

	// Connect inertia timer to update slot
	connect(inertiaTimer_, &QTimer::timeout, this, [this]() {
		// Apply rotation based on velocity and time
		if (rotationVelocity_ > 0.05f) { // Increased from 0.01f - stop sooner
			// Apply rotation based on current velocity
			QQuaternion inertiaRotation = QQuaternion::fromAxisAndAngle(
				rotationAxis_, rotationVelocity_);

			// Update the rotation
			rotation_ = inertiaRotation * rotation_;

			// Decay the velocity
			rotationVelocity_ *= rotationDecay_;

			// Trigger redraw
			update();
		}
		else {
			// Stop the timer when velocity is negligible
			stopInertia();
		}
		});

	// Start frame timer
	frameTimer_.start();

	// Create context menu
	QAction* resetAction = contextMenu_->addAction("Reset View");
	connect(resetAction, &QAction::triggered, this, &SphereWidget::resetView);

	// Add separator for better organization
	contextMenu_->addSeparator();

	QAction* toggleAxesAction = contextMenu_->addAction("Toggle Axes");
	toggleAxesAction->setCheckable(true);
	toggleAxesAction->setChecked(true);
	connect(toggleAxesAction, &QAction::toggled, this, [this](bool checked) {
		showAxes_ = checked;
		update();
		});

	// Toggle Sphere action
	QAction* toggleSphereAction = contextMenu_->addAction("Toggle Sphere");
	toggleSphereAction->setCheckable(true);
	toggleSphereAction->setChecked(true);
	connect(toggleSphereAction, &QAction::toggled, this, [this](bool checked) {
		showSphere_ = checked;
		update();
		});

	// Add Toggle Grid Lines action
	QAction* toggleGridLinesAction = contextMenu_->addAction("Toggle Grid");
	toggleGridLinesAction->setCheckable(true);
	toggleGridLinesAction->setChecked(true);
	connect(toggleGridLinesAction, &QAction::toggled, this, [this](bool checked) {
		showGridLines_ = checked;
		update();  // Request a redraw
		});

	// Add separator for better organization
	contextMenu_->addSeparator();

	// Add Toggle Inertia action
	QAction* toggleInertiaAction = contextMenu_->addAction("Toggle Inertia");
	toggleInertiaAction->setCheckable(true);
	toggleInertiaAction->setChecked(false);
	connect(toggleInertiaAction, &QAction::toggled, this, &SphereWidget::setInertiaEnabled);
	
	// Add separator for better organization
	contextMenu_->addSeparator();

	// Add Toggle Beam action
	QAction* toggleBeamAction = contextMenu_->addAction("Toggle Beam");
	toggleBeamAction->setCheckable(true);
	toggleBeamAction->setChecked(true);
	connect(toggleBeamAction, &QAction::toggled, this, [this](bool checked) {
		showBeam_ = checked;
		if (radarBeam_) {
			radarBeam_->setVisible(checked);
		}
		update();
		});

	// Add beam type submenu
	QMenu* beamTypeMenu = contextMenu_->addMenu("Beam Type");

	QAction* conicalBeamAction = beamTypeMenu->addAction("Conical");
	conicalBeamAction->setCheckable(true);
	conicalBeamAction->setChecked(true);

	QAction* ellipticalBeamAction = beamTypeMenu->addAction("Elliptical");
	ellipticalBeamAction->setCheckable(true);

	QAction* phasedBeamAction = beamTypeMenu->addAction("Phased Array");
	phasedBeamAction->setCheckable(true);

	// Make them exclusive
	QActionGroup* beamTypeGroup = new QActionGroup(this);
	beamTypeGroup->addAction(conicalBeamAction);
	beamTypeGroup->addAction(ellipticalBeamAction);
	beamTypeGroup->addAction(phasedBeamAction);
	beamTypeGroup->setExclusive(true);

	// Connect beam type actions
	connect(conicalBeamAction, &QAction::triggered, this, [this]() {
		qDebug() << "Switching to Conical beam";
		setBeamType(BeamType::Conical);
		});

	connect(ellipticalBeamAction, &QAction::triggered, this, [this]() {
		qDebug() << "Switching to Elliptical beam";
		setBeamType(BeamType::Elliptical);
		});

	connect(phasedBeamAction, &QAction::triggered, this, [this]() {
		qDebug() << "Switching to Phased Array beam";
		setBeamType(BeamType::Phased);
		});

	connect(this, &QOpenGLWidget::aboutToCompose, this, [this]() {
		static bool firstFrame = true;
		if (firstFrame) {
			firstFrame = false;
			update();
		}
		});
}

// Destructor to clean up resources
SphereWidget::~SphereWidget() {
	makeCurrent();

	// Clean up OpenGL resources
	sphereVBO.destroy();
	sphereEBO.destroy();
	linesVBO.destroy();
	dotVBO.destroy();
	axesVBO.destroy();

	sphereVAO.destroy();
	linesVAO.destroy();
	dotVAO.destroy();
	axesVAO.destroy();

	delete shaderProgram;
	delete dotShaderProgram;
	delete axesShaderProgram;
	delete radarBeam_;
	
	// Clean up inertia timer
	if (inertiaTimer_) {
		inertiaTimer_->stop();
		delete inertiaTimer_;
	}

	// Clean up context menu
	delete contextMenu_;
	
	doneCurrent();
}


// Main vertex shader for sphere and lines
const char* vertexShaderSource = R"(
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

// Main fragment shader for sphere and lines
const char* fragmentShaderSource = R"(
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
// Vertex shader for dot (can be the same as the main vertex shader)
const char* dotVertexShaderSource = R"(
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

// Fragment shader with transparency support
const char* dotFragmentShaderSource = R"(
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

// Create a version of the vertex shader for axes that uses color directly
const char* axesVertexShaderSource = R"(
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

// Fragment shader for axes
const char* axesFragmentShaderSource = R"(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
)";

// In the initializeGL() method, add this code to create the dot shader program:
void SphereWidget::initializeGL() {
	initializeOpenGLFunctions();

	// Set up OpenGL with stronger state management
	glEnable(GL_DEPTH_TEST);

	// Completely disable blending during initialization
	glDisable(GL_BLEND);

	// Force viewport to cover the entire widget
	glViewport(0, 0, width(), height());

	// Use a distinct clear color (dark blue) to see if clearing is working
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

	// Clear both color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset to desired background color
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Create and compile main shader program
	shaderProgram = new QOpenGLShaderProgram(this);
	shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
	shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
	shaderProgram->link();

	// Create and compile dot shader program
	dotShaderProgram = new QOpenGLShaderProgram(this);
	dotShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, dotVertexShaderSource);
	dotShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, dotFragmentShaderSource);
	dotShaderProgram->link();

	// Create and compile axes shader program 
	axesShaderProgram = new QOpenGLShaderProgram(this);
	axesShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, axesVertexShaderSource);
	axesShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, axesFragmentShaderSource);
	axesShaderProgram->link();

	// Create geometry
	createSphere();
	createSphereGrid();
	createSphereAxes(); 
	createDot();
	
	// Set up view matrix
	viewMatrix.setToIdentity();
	viewMatrix.translate(0, 0, -300.0f); // Same as the fixed-distance camera

	// Initialize the radar beam (after OpenGL context is set up)
	radarBeam_ = RadarBeam::createBeam(BeamType::Conical, radius_, 15.0f);
	radarBeam_->initialize();
	radarBeam_->setColor(QVector3D(1.0f, 0.5f, 0.0f)); // Orange-red beam
	radarBeam_->setOpacity(0.3f); // Semi-transparent

	// Update the beam with initial radar position
	QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);
	radarBeam_->update(radarPos);

	// Force a complete redraw after initialization
	update();
}

void SphereWidget::setRadius(float r) {
	radius_ = r;

	// Regenerate geometry
	makeCurrent();

	// Update sphere and grid geometry
	createSphere();
	createSphereGrid();
	createSphereAxes();

	// Update beam size if it exists
	if (radarBeam_) {
		qDebug() << "Updating beam for new radius:" << r;

		// Save current beam properties
		float width = radarBeam_->getBeamWidth();
		QVector3D color = radarBeam_->getColor();
		float opacity = radarBeam_->getOpacity();
		bool visible = radarBeam_->isVisible();

		// Special case for EllipticalBeam
		float horizontalWidth = width;
		float verticalWidth = width / 2.0f;

		EllipticalBeam* ellipticalBeam = dynamic_cast<EllipticalBeam*>(radarBeam_);
		if (ellipticalBeam) {
			horizontalWidth = ellipticalBeam->getHorizontalWidth();
			verticalWidth = ellipticalBeam->getVerticalWidth();
			qDebug() << "Elliptical beam detected - Horizontal width:" << horizontalWidth
				<< "Vertical width:" << verticalWidth;
		}

		// Determine beam type
		BeamType currentType = BeamType::Conical; // Default
		if (ellipticalBeam) {
			currentType = BeamType::Elliptical;
		}
		else if (dynamic_cast<PhasedArrayBeam*>(radarBeam_)) {
			currentType = BeamType::Phased;
		}

		// Delete old beam
		delete radarBeam_;
		radarBeam_ = nullptr;

		// Create and initialize new beam
		if (currentType == BeamType::Elliptical) {
			// Create elliptical beam directly to preserve both widths
			radarBeam_ = new EllipticalBeam(radius_, horizontalWidth, verticalWidth);
		}
		else {
			// Use factory method for other types
			radarBeam_ = RadarBeam::createBeam(currentType, radius_, width);
		}

		if (radarBeam_) {
			radarBeam_->initialize();
			radarBeam_->setColor(color);
			radarBeam_->setOpacity(opacity);
			radarBeam_->setVisible(visible);

			// Calculate current radar position and update beam
			QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);
			radarBeam_->update(radarPos);
		}
	}

	doneCurrent();
	update();
}

void SphereWidget::setAngles(float t, float p) {
	theta_ = t;
	phi_ = p;

	// Update beam when radar position changes
	if (radarBeam_) {
		QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);
		radarBeam_->update(radarPos);
	}

	update();
}

void SphereWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);

	// Update projection matrix
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(45.0f, float(w) / float(h), 0.1f, 2000.0f);
}

// Update the paintGL method to improve grid line visibility
void SphereWidget::paintGL() {
	// STEP 1: Reset OpenGL state
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	// STEP 2: Initial clear
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// STEP 3: Extra pass with a full-screen shader to ensure complete clearing
	// Create a simple shader program for this if you don't have one
	QOpenGLShaderProgram fullscreenShader;
	if (!fullscreenShader.addShaderFromSourceCode(QOpenGLShader::Vertex,
		"attribute vec2 position;\n"
		"void main() {\n"
		"   gl_Position = vec4(position, 0.0, 1.0);\n"
		"}\n")) {
		qDebug() << "Failed to compile fullscreen vertex shader:" << fullscreenShader.log();
	}

	if (!fullscreenShader.addShaderFromSourceCode(QOpenGLShader::Fragment,
		"uniform vec4 color;\n"
		"void main() {\n"
		"   gl_FragColor = color;\n"
		"}\n")) {
		qDebug() << "Failed to compile fullscreen fragment shader:" << fullscreenShader.log();
	}

	if (!fullscreenShader.link()) {
		qDebug() << "Failed to link fullscreen shader:" << fullscreenShader.log();
	}

	// Use the shader to draw a full-screen quad
	if (fullscreenShader.isLinked()) {
		fullscreenShader.bind();

		// Set the background color
		fullscreenShader.setUniformValue("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));

		// Create a VAO for the full-screen quad
		QOpenGLVertexArrayObject vao;
		vao.create();
		vao.bind();

		// Vertex data for a full-screen quad
		const GLfloat vertices[] = {
			-1.0f, -1.0f,
			 1.0f, -1.0f,
			 1.0f,  1.0f,
			-1.0f,  1.0f
		};

		// Upload vertex data
		QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
		vbo.create();
		vbo.bind();
		vbo.allocate(vertices, sizeof(vertices));

		// Set up vertex attribute
		fullscreenShader.enableAttributeArray("position");
		fullscreenShader.setAttributeBuffer("position", GL_FLOAT, 0, 2);

		// Draw the quad
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// Clean up
		vbo.release();
		vao.release();
		fullscreenShader.release();
	}

	// STEP 4: Continue with normal rendering
	
	// Begin with known OpenGL state
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	// Ensure viewport covers entire widget
	glViewport(0, 0, width(), height());

	// Clear with solid background color
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLError("clear in paintGL");

	// Set up model matrix with current rotation, zoom, and translation
	modelMatrix.setToIdentity();

	// Apply zoom
	modelMatrix.scale(zoomFactor_);

	// Apply translation
	modelMatrix.translate(translation_);

	// Apply rotation
	modelMatrix.rotate(rotation_);

	// 1. First, draw the sphere
	if (showSphere_) {
		shaderProgram->bind();
		shaderProgram->setUniformValue("projection", projectionMatrix);
		shaderProgram->setUniformValue("view", viewMatrix);
		shaderProgram->setUniformValue("model", modelMatrix);
		shaderProgram->setUniformValue("color", QVector3D(0.95f, 0.95f, 0.95f));
		shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

		sphereVAO.bind();

		// Enable polygon offset to prevent z-fighting
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);

		glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

		// Disable polygon offset after drawing the sphere
		glDisable(GL_POLYGON_OFFSET_FILL);

		sphereVAO.release();
		shaderProgram->release();
	}

	// 2. Then draw grid lines
	if (showGridLines_) {
		shaderProgram->bind();
		shaderProgram->setUniformValue("projection", projectionMatrix);
		shaderProgram->setUniformValue("view", viewMatrix);
		shaderProgram->setUniformValue("model", modelMatrix);
		shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

		linesVAO.bind();

		// Enable line smoothing and wider lines for better visibility
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		// Enable depth test but with LESS_OR_EQUAL instead of LESS
		// This gives grid lines a slight advantage in the depth test
		glDepthFunc(GL_LEQUAL);

		// Constants - must match those used in createLatitudeLongitudeLines
		const int latitudeSegments = 72;
		const int longitudeSegments = 50;

		// Regular latitude lines
		glLineWidth(1.5f);
		shaderProgram->setUniformValue("color", QVector3D(0.25f, 0.25f, 0.25f));

		// Loop through all latitude lines
		for (int lat = 0; lat < latitudeLineCount; lat++) {
			// Calculate starting index for this latitude line
			int startIdx = lat * (latitudeSegments + 1);

			// Skip equator, which will be drawn specially
			if (startIdx == equatorStartIndex) {
				continue;
			}

			// Draw this latitude line
			glDrawArrays(GL_LINE_STRIP, startIdx, latitudeSegments + 1);
		}

		// Regular longitude lines
		int longitudeStartOffset = latitudeLineCount * (latitudeSegments + 1);

		for (int lon = 0; lon < longitudeLineCount; lon++) {
			// Calculate starting index for this longitude line
			int startIdx = longitudeStartOffset + lon * (longitudeSegments + 1);

			// Skip prime meridian, which will be drawn specially
			if (startIdx == primeMeridianStartIndex) {
				continue;
			}

			// Draw this longitude line
			glDrawArrays(GL_LINE_STRIP, startIdx, longitudeSegments + 1);
		}

		// Special lines (equator and prime meridian)
		glLineWidth(3.5f);

		// Equator (green)
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.9f, 0.0f));
		glDrawArrays(GL_LINE_STRIP, equatorStartIndex, latitudeSegments + 1);

		// Prime Meridian (blue)
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 0.9f));
		glDrawArrays(GL_LINE_STRIP, primeMeridianStartIndex, longitudeSegments + 1);

		// Restore default depth function
		glDepthFunc(GL_LESS);

		// Clean up
		glDisable(GL_LINE_SMOOTH);
		linesVAO.release();
		shaderProgram->release();
	}

	// 3. Draw coordinate major axes
	if (showAxes_) {
		// Use the standard shader for the axes (which processes normal/color)
		shaderProgram->bind();
		shaderProgram->setUniformValue("projection", projectionMatrix);
		shaderProgram->setUniformValue("view", viewMatrix);
		shaderProgram->setUniformValue("model", modelMatrix);
		shaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));

		axesVAO.bind();

		// Enable line smoothing
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		// Draw with depth offset to prevent z-fighting with other elements
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);

		// Draw main axis lines
		glLineWidth(3.0f);
		// Set color for X axis (red)
		shaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
		glDrawArrays(GL_LINES, 0, 2);  // First line (X axis)

		// Set color for Y axis (green)
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
		glDrawArrays(GL_LINES, 2, 2);  // Second line (Y axis)

		// Set color for Z axis (blue)
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
		glDrawArrays(GL_LINES, 4, 2);  // Third line (Z axis)

		// *** IMPORTANT: Correctly drawing the arrowheads ***
		// Arrowheads start at vertex 6
		// Each axis has 'segments' triangles (12 in this case)
		// Each triangle has 3 vertices
		// So total arrowhead vertices = 3 axes * segments * 3 vertices per triangle

		int segments = 12; // Must match what is used in createCoordinateAxes()
		int arrowheadVertices = 3 * segments * 3; // 3 axes, segments triangles per axis, 3 vertices per triangle

		// Draw X axis arrowhead (red)
		shaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
		glDrawArrays(GL_TRIANGLES, 6, segments * 3); // First arrowhead

		// Draw Y axis arrowhead (green)
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
		glDrawArrays(GL_TRIANGLES, 6 + segments * 3, segments * 3); // Second arrowhead

		// Draw Z axis arrowhead (blue)
		shaderProgram->setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
		glDrawArrays(GL_TRIANGLES, 6 + 2 * segments * 3, segments * 3); // Third arrowhead

		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_LINE_SMOOTH);
		axesVAO.release();
		shaderProgram->release();
	}

	// 4. Draw radar dot
	{
		QVector3D dotPos = sphericalToCartesian(radius_, theta_, phi_);
		QMatrix4x4 dotModelMatrix = modelMatrix;
		dotModelMatrix.translate(dotPos);

		// 4.1. First, draw opaque parts of the dot (front-facing parts)
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);  // Standard depth test
		glDepthMask(GL_TRUE);  // Write to depth buffer

		dotShaderProgram->bind();
		dotShaderProgram->setUniformValue("projection", projectionMatrix);
		dotShaderProgram->setUniformValue("view", viewMatrix);
		dotShaderProgram->setUniformValue("model", dotModelMatrix);
		dotShaderProgram->setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
		dotShaderProgram->setUniformValue("lightPos", QVector3D(500.0f, 500.0f, 500.0f));
		dotShaderProgram->setUniformValue("opacity", 1.0f);  // Fully opaque

		dotVAO.bind();
		// Only draw front-facing polygons in this pass
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDrawArrays(GL_TRIANGLES, 0, dotVertices.size() / 6);
		glDisable(GL_CULL_FACE);
		dotVAO.release();

		dotShaderProgram->release();

		// 4.2. Then, draw transparent parts of the dot (back-facing parts)
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);  // Don't write to depth buffer for transparent parts

		dotShaderProgram->bind();
		dotShaderProgram->setUniformValue("opacity", 0.2f);  // Partially transparent

		dotVAO.bind();

		// Draw with special depth test for transparency
		glDepthFunc(GL_GREATER);  // Only draw if behind other objects

		glDrawArrays(GL_TRIANGLES, 0, dotVertices.size() / 6);

		// Restore default depth function and settings
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);

		dotVAO.release();
		dotShaderProgram->release();
	}

	// 5. Draw radar beam if visible
	if (showBeam_ && radarBeam_ && radarBeam_->isVisible()) {
		// Debug check
		if (dynamic_cast<ConicalBeam*>(radarBeam_)) {
			qDebug() << "Rendering ConicalBeam";
		}
		else if (dynamic_cast<EllipticalBeam*>(radarBeam_)) {
			qDebug() << "Rendering EllipticalBeam";
		}
		else if (dynamic_cast<PhasedArrayBeam*>(radarBeam_)) {
			qDebug() << "Rendering PhasedArrayBeam";
		}
		else {
			qDebug() << "Rendering unknown beam type";
		}

		radarBeam_->render(shaderProgram, projectionMatrix, viewMatrix, modelMatrix);
	}
	else if (showBeam_) {
		if (!radarBeam_) {
			qDebug() << "Beam object is null";
		}
		else if (!radarBeam_->isVisible()) {
			qDebug() << "Beam is not visible";
		}
	}
}

void SphereWidget::createSphere(int latDivisions, int longDivisions) {
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

void SphereWidget::createSphereGrid() {
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

void SphereWidget::createDot() {
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

bool SphereWidget::isDotVisible() {
	// Calculate dot position in spherical coordinates
	QVector3D dotPos = sphericalToCartesian(radius_, theta_, phi_);

	// Calculate vector from camera to dot
	QVector3D cameraPos(0, 0, -300.0f); // From your fixed camera position
	QVector3D dotToCameraVec = cameraPos - dotPos;

	// Calculate vector from sphere center to dot
	QVector3D sphereCenter(0, 0, 0);
	QVector3D sphereToDotVec = dotPos - sphereCenter;

	// Dot is visible if the angle between these vectors is less than 90 degrees
	// (i.e., if dot product is positive)
	return QVector3D::dotProduct(dotToCameraVec.normalized(),
		sphereToDotVec.normalized()) > 0;
}

QVector3D SphereWidget::sphericalToCartesian(float r, float thetaDeg, float phiDeg) {
	const float toRad = float(M_PI / 180.0);
	float theta = thetaDeg * toRad;
	float phi = phiDeg * toRad;
	return QVector3D(
		r * cos(phi) * cos(theta),
		r * sin(phi),
		r * cos(phi) * sin(theta)
	);
}

void SphereWidget::createSphereAxes() {
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

// ******************************************************************************************************************
// Beam Control Methods
// ******************************************************************************************************************

void SphereWidget::setBeamType(BeamType type) {
	qDebug() << "Changing beam type to:" << static_cast<int>(type);

	if (radarBeam_) {
		// Store current properties
		float width = radarBeam_->getBeamWidth();
		QVector3D color = radarBeam_->getColor();
		float opacity = radarBeam_->getOpacity();
		bool visible = radarBeam_->isVisible();

		makeCurrent();

		// Delete old beam
		delete radarBeam_;
		radarBeam_ = nullptr;

		// Create new beam of specified type
		radarBeam_ = RadarBeam::createBeam(type, radius_, width);

		if (radarBeam_) {
			radarBeam_->initialize();
			radarBeam_->setColor(color);
			radarBeam_->setOpacity(opacity);
			radarBeam_->setVisible(visible);

			// Update the beam with current radar position
			QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);
			radarBeam_->update(radarPos);

			qDebug() << "New beam created and initialized successfully";
		}
		else {
			qDebug() << "ERROR: Failed to create new beam of type:" << static_cast<int>(type);
		}

		doneCurrent();
		update();
	}
	else {
		qDebug() << "WARNING: No existing beam to change type";

		// Create a new beam if none exists
		makeCurrent();
		radarBeam_ = RadarBeam::createBeam(type, radius_, 15.0f);
		if (radarBeam_) {
			radarBeam_->initialize();
			radarBeam_->setColor(QVector3D(1.0f, 0.5f, 0.0f));
			radarBeam_->setOpacity(0.3f);

			// Update position
			QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);
			radarBeam_->update(radarPos);

			qDebug() << "Created new beam since none existed";
		}
		doneCurrent();
		update();
	}
}

void SphereWidget::setBeamWidth(float degrees) {
	if (radarBeam_) {
		radarBeam_->setBeamWidth(degrees);
		update();
	}
}

void SphereWidget::setBeamColor(const QVector3D& color) {
	if (radarBeam_) {
		radarBeam_->setColor(color);
		update();
	}
}

void SphereWidget::setBeamOpacity(float opacity) {
	if (radarBeam_) {
		radarBeam_->setOpacity(opacity);
		update();
	}
}


// ******************************************************************************************************************
// // Eevent handlers
// ******************************************************************************************************************

// Context menu event handler
void SphereWidget::contextMenuEvent(QContextMenuEvent* event) {
	contextMenu_->popup(event->globalPos());
}

//  Context menu Reset view to default
void SphereWidget::resetView() {
	// Stop inertia
	stopInertia();

	// Reset rotation and zoom
	rotation_ = QQuaternion();
	zoomFactor_ = 1.0f;

	// Reset translation
	translation_ = QVector3D(0, 0, 0);

	// Trigger redraw
	update();
}

// Context menu Double-click event handler
void SphereWidget::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		resetView();
	}
	QOpenGLWidget::mouseDoubleClickEvent(event);
}

// Context menu Start inertia effect
void SphereWidget::startInertia(QVector3D axis, float velocity) {
	rotationAxis_ = axis.normalized();
	rotationVelocity_ = velocity;

	// Start timer if not already running
	if (!inertiaTimer_->isActive()) {
		inertiaTimer_->start(16); // ~60 FPS
	}
}

// Context menu Stop inertia effect
void SphereWidget::stopInertia() {
	inertiaTimer_->stop();
	rotationVelocity_ = 0.0f;
}

// Context menu Pan method
void SphereWidget::pan(const QPoint& delta) {
	// Scale factor for panning (adjust as needed)
	float panScale = 0.5f / zoomFactor_;

	// Update translation (invert Y for natural panning)
	translation_.setX(translation_.x() + delta.x() * panScale);
	translation_.setY(translation_.y() - delta.y() * panScale);

	// Trigger redraw
	update();
}


// Mouse press event handler
void SphereWidget::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		// Stop any ongoing inertia when user grabs the sphere
		stopInertia();

		isDragging_ = true;
		lastMousePos_ = event->pos();

		// Reset frame timer for velocity calculation
		frameTimer_.restart();
	}
	else if (event->button() == Qt::RightButton) {
		// Right button for panning
		isPanning_ = true;
		panStartPos_ = event->pos();
	}
	else if (event->button() == Qt::MiddleButton) {
		// Middle button to reset view
		resetView();
	}

	QOpenGLWidget::mousePressEvent(event);
}

// Mouse move event handler
void SphereWidget::mouseMoveEvent(QMouseEvent* event) {
	if (isDragging_) {
		// Calculate elapsed time for velocity calculation
		float dt = frameTimer_.elapsed() / 1000.0f; // Convert to seconds
		frameTimer_.restart();

		// Avoid division by zero
		if (dt < 0.001f) dt = 0.016f; // Default to ~60 FPS if timer is too fast

		// Calculate mouse movement delta
		QPoint delta = event->pos() - lastMousePos_;

		// Exit if no movement
		if (delta.isNull()) {
			return;
		}

		// Convert to rotation (scale down the rotation speed)
		float rotationSpeed = 0.3f;
		float angleX = delta.x() * rotationSpeed;
		float angleY = delta.y() * rotationSpeed;

		// Create combined rotation axis and angle
		QVector3D axis(angleY, angleX, 0.0f);
		float angle = axis.length();
		axis.normalize();

		// Store axis and velocity for inertia calculation
		rotationAxis_ = axis;
		rotationVelocity_ = angle / dt * 0.1f; // Scale down for smoother inertia

		// Create quaternion from axis and angle
		QQuaternion newRotation = QQuaternion::fromAxisAndAngle(axis, angle);

		// Update rotation
		rotation_ = newRotation * rotation_;

		// Store current mouse position for next frame
		lastMousePos_ = event->pos();

		// Trigger redraw
		update();
	}
	else if (isPanning_) {
		// Calculate pan delta
		QPoint delta = event->pos() - panStartPos_;

		// Apply panning
		pan(delta);

		// Update starting position
		panStartPos_ = event->pos();
	}

	QOpenGLWidget::mouseMoveEvent(event);
}

// Mouse release event handler
void SphereWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton && isDragging_) {
		isDragging_ = false;

		// Only start inertia if it's enabled and velocity is significant
		if (inertiaEnabled_ && rotationVelocity_ > 0.2f) {
			startInertia(rotationAxis_, rotationVelocity_);
		}
	}
	else if (event->button() == Qt::RightButton) {
		isPanning_ = false;
	}

	QOpenGLWidget::mouseReleaseEvent(event);
}

// Mouse wheel event handler for zooming
void SphereWidget::wheelEvent(QWheelEvent* event) {
	// Calculate zoom change based on wheel delta
	// QWheelEvent::angleDelta() returns degrees * 8
	float zoomSpeed = 0.001f;
	float zoomChange = event->angleDelta().y() * zoomSpeed;

	// Update zoom factor with limits
	zoomFactor_ += zoomChange;
	zoomFactor_ = qBound(minZoom_, zoomFactor_, maxZoom_);

	// Trigger redraw
	update();

	QOpenGLWidget::wheelEvent(event);
}

void SphereWidget::checkGLError(const char* operation) {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		qWarning() << "OpenGL error after" << operation << ":" << error;
	}
}
