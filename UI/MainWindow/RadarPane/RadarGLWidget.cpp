// RadarGLWidget.cpp
#include "RadarGLWidget.h"
#include "FBORenderer.h"
#include "GLUtils.h"
#include "Constants.h"
#include <QDebug>
#include <QPainter>
#include <QFont>

using namespace RS::Constants;

RadarGLWidget::RadarGLWidget(QWidget* parent)
	: QOpenGLWidget(parent),
	radius_(Defaults::kSphereRadius),
	theta_(Defaults::kRadarTheta),
	phi_(Defaults::kRadarPhi),
	sphereRenderer_(nullptr),
	beamController_(nullptr),
	cameraController_(nullptr),
	modelManager_(nullptr),
	wireframeController_(nullptr)
{
	// Set focus policy to receive keyboard events
	setFocusPolicy(Qt::StrongFocus);
}

RadarGLWidget::~RadarGLWidget() {
	// Clean up OpenGL resources while context is still valid
	// Must do this before base class destructor destroys the context
	makeCurrent();
	cleanupGL();

	// Reset unique_ptrs while context is still current
	// This ensures VAO/VBO destructors run with valid context
	sphereRenderer_.reset();
	radarSiteRenderer_.reset();
	beamController_.reset();
	cameraController_.reset();
	modelManager_.reset();
	wireframeController_.reset();
	rcsCompute_.reset();
	reflectionRenderer_.reset();
	heatMapRenderer_.reset();
	slicingPlaneRenderer_.reset();
	fboRenderer_.reset();

	doneCurrent();
}

void RadarGLWidget::cleanupGL() {
	// Prevent double cleanup
	if (glCleanedUp_) {
		return;
	}
	glCleanedUp_ = true;

	// Try to make context current for cleanup
	// This may fail if the context is already being destroyed
	bool contextMadeCurrent = false;
	if (context() && context()->isValid()) {
		makeCurrent();
		contextMadeCurrent = QOpenGLContext::currentContext() != nullptr;
	}

	if (!contextMadeCurrent) {
		qWarning() << "RadarGLWidget::cleanupGL() - could not make context current";
	}

	// Clean up RCS compute resources
	if (rcsCompute_) {
		rcsCompute_->cleanup();
	}

	// Clean up reflection renderer
	if (reflectionRenderer_) {
		reflectionRenderer_->cleanup();
	}

	// Clean up heat map renderer
	if (heatMapRenderer_) {
		heatMapRenderer_->cleanup();
	}

	// Clean up slicing plane renderer
	if (slicingPlaneRenderer_) {
		slicingPlaneRenderer_->cleanup();
	}

	// Clean up component resources
	if (sphereRenderer_) {
		sphereRenderer_->cleanup();
	}

	if (radarSiteRenderer_) {
		radarSiteRenderer_->cleanup();
	}

	if (beamController_) {
		beamController_->cleanup();
	}

	if (wireframeController_) {
		wireframeController_->cleanup();
	}

	// Note: modelManager cleanup would go here if it has OpenGL resources

	if (contextMadeCurrent) {
		doneCurrent();
	}
}

void RadarGLWidget::initialize(SphereRenderer* sphereRenderer, BeamController* beamController, CameraController* cameraController, ModelManager* modelManager, WireframeTargetController* wireframeController) {
	// Take ownership of components via unique_ptr
	sphereRenderer_.reset(sphereRenderer);
	beamController_.reset(beamController);
	cameraController_.reset(cameraController);
	modelManager_.reset(modelManager);
	wireframeController_.reset(wireframeController);

	// Store the components but don't initialize them here
	// They will be initialized in initializeGL when a valid context exists
	// Set mouse tracking to improve responsiveness
	setMouseTracking(true);

	// Connect camera controller's viewChanged signal to trigger repaints
	// This is essential for smooth inertia animation
	if (cameraController_) {
		connect(cameraController_.get(), &CameraController::viewChanged,
			this, QOverload<>::of(&QWidget::update));
	}

}

void RadarGLWidget::initializeGL() {
	// Initialize OpenGL functions
	if (!initializeOpenGLFunctions()) {
		qCritical() << "Failed to initialize OpenGL functions!";
		return;
	}

	// Clear any pending GL errors from context initialization
	GLUtils::clearGLErrors();

	// Make sure the context is current
	if (!context()->isValid()) {
		qCritical() << "OpenGL context is not valid!";
		return;
	}

	// Set up OpenGL
	glClearColor(Colors::kBackgroundGrey[0], Colors::kBackgroundGrey[1], Colors::kBackgroundGrey[2], 1.0f);
	glEnable(GL_DEPTH_TEST);

	// Check for errors after basic GL setup
	if (GLUtils::checkGLError("initializeGL: basic setup")) {
		qWarning() << "OpenGL error during basic setup, continuing...";
	}

	// Now initializeContext the components with the GL context
	try {
		if (sphereRenderer_) {
			sphereRenderer_->initialize();
		}

		// Create and initialize radar site renderer
		radarSiteRenderer_ = std::make_unique<RadarSiteRenderer>();
		if (!radarSiteRenderer_->initialize()) {
			qWarning() << "RadarSiteRenderer initialization failed";
			radarSiteRenderer_.reset();
		} else {
			radarSiteRenderer_->setPosition(theta_, phi_);
		}

		if (beamController_) {
			beamController_->initialize();
			// Synchronize sphere radius after initialization
			beamController_->setSphereRadius(radius_);
		}

		if (modelManager_) {
			if (!modelManager_->initialize()) {
				qWarning() << "ModelManager initialization failed";
			}
		}

		if (wireframeController_) {
			wireframeController_->initialize();
		}

		// Initialize RCS compute for GPU ray tracing
		rcsCompute_ = std::make_unique<RCS::RCSCompute>(this);
		if (!rcsCompute_->initialize()) {
			qWarning() << "RCSCompute initialization failed - ray tracing disabled";
			rcsCompute_.reset();
		} else {
			rcsCompute_->setSphereRadius(radius_);
		}

		// Initialize reflection lobe renderer
		reflectionRenderer_ = std::make_unique<ReflectionRenderer>(this);
		if (!reflectionRenderer_->initialize()) {
			qWarning() << "ReflectionRenderer initialization failed";
			reflectionRenderer_.reset();
		}

		// Initialize heat map renderer
		heatMapRenderer_ = std::make_unique<HeatMapRenderer>(this);
		if (!heatMapRenderer_->initialize()) {
			qWarning() << "HeatMapRenderer initialization failed";
			heatMapRenderer_.reset();
		} else {
			heatMapRenderer_->setSphereRadius(radius_);
		}

		// Initialize RCS samplers for polar plot
		azimuthSampler_ = std::make_unique<AzimuthCutSampler>();
		elevationSampler_ = std::make_unique<ElevationCutSampler>();
		currentSampler_ = azimuthSampler_.get();  // Default to azimuth cut
		currentCutType_ = CutType::Azimuth;
		polarPlotData_.resize(RS::Constants::kPolarPlotBins);

		// Initialize slicing plane renderer
		slicingPlaneRenderer_ = std::make_unique<SlicingPlaneRenderer>(this);
		if (!slicingPlaneRenderer_->initialize()) {
			qWarning() << "SlicingPlaneRenderer initialization failed";
			slicingPlaneRenderer_.reset();
		} else {
			slicingPlaneRenderer_->setSphereRadius(radius_);
			slicingPlaneRenderer_->setCutType(currentCutType_);
		}

		// Force beam geometry creation with initial position
		if (beamController_) {
			QVector3D initialPos = sphericalToCartesian(radius_, theta_, phi_);
			beamController_->updateBeamPosition(initialPos);
			beamController_->rebuildBeamGeometry();
		}

		// Initialize FBO renderer for pop-out windows
		fboRenderer_ = std::make_unique<FBORenderer>(this);
		if (!fboRenderer_->initialize(width(), height())) {
			qWarning() << "FBORenderer initialization failed - pop-out windows may not work";
			fboRenderer_.reset();
		}

		// Mark initialization as complete
		glInitialized_ = true;
	}
	catch (const std::exception& e) {
		qCritical() << "Exception during component initialization:" << e.what();
	}
	catch (...) {
		qCritical() << "Unknown exception during component initialization";
	}
}

void RadarGLWidget::resizeGL(int w, int h) {
	if (!glInitialized_) {
		return;
	}
	glViewport(0, 0, w, h);

	// Resize FBO for pop-out windows
	if (fboRenderer_) {
		fboRenderer_->resize(w, h);
	}
}

void RadarGLWidget::paintGL() {
	// Check context and initialization FIRST - before any GL calls
	if (!context() || !context()->isValid()) {
		qWarning() << "OpenGL context not valid in paintGL";
		return;
	}

	if (!glInitialized_) {
		qWarning() << "paintGL called before initialization complete";
		return;
	}

	// Check camera controller
	if (!cameraController_) {
		qWarning() << "CameraController not available in paintGL";
		return;
	}

	// Bind FBO if rendering to texture for pop-out window
	if (renderToFBO_ && fboRenderer_ && fboRenderer_->isValid()) {
		fboRenderer_->bind();
	}

	// Now safe to use GL functions - update beam position and geometry
	updateBeamPosition();
	beamController_->rebuildBeamGeometry();
	beamDirty_ = false;

	// Rebuild wireframe geometry if type changed
	if (wireframeController_) {
		wireframeController_->rebuildGeometry();
	}

	// Ensure blending and stencil are in clean state
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);

	// Clear all buffers including stencil
	glClearColor(Colors::kBackgroundGrey[0], Colors::kBackgroundGrey[1], Colors::kBackgroundGrey[2], 1.0f);
	glClearStencil(0);
	glStencilMask(0xFF);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Get view and model matrices from camera controller
	QMatrix4x4 viewMatrix = cameraController_->getViewMatrix();
	QMatrix4x4 modelMatrix = cameraController_->getModelMatrix();

	// Set up projection matrix
	// When rendering to FBO, use FBO dimensions for correct aspect ratio
	int renderWidth = width();
	int renderHeight = height();
	if (renderToFBO_ && fboRenderer_ && fboRenderer_->isValid()) {
		renderWidth = fboRenderer_->width();
		renderHeight = fboRenderer_->height();
	}
	QMatrix4x4 projectionMatrix;
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(View::kPerspectiveFOV, float(renderWidth) / float(renderHeight), View::kNearPlane, View::kFarPlane);

	// Draw components
	try {
		if (sphereRenderer_) {
			sphereRenderer_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		// Render radar site dot (after sphere, using sphere's rotation)
		if (radarSiteRenderer_) {
			// Apply sphere rotation to model matrix for consistent dot positioning
			QMatrix4x4 rotatedModel = modelMatrix;
			if (sphereRenderer_) {
				rotatedModel.rotate(sphereRenderer_->getRotation());
			}
			radarSiteRenderer_->render(projectionMatrix, viewMatrix, rotatedModel, radius_);
		}

		if (modelManager_) {
			modelManager_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		if (wireframeController_) {
			wireframeController_->render(projectionMatrix, viewMatrix, modelMatrix);

			// Get radar position for RCS ray tracing
			QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);

			// Run RCS ray tracing if available
			if (rcsCompute_ && wireframeController_->getTarget()) {
				auto* target = wireframeController_->getTarget();

				rcsCompute_->setTargetGeometry(
					target->getVertices(),
					target->getIndices(),
					target->getModelMatrix()
				);
				rcsCompute_->setRadarPosition(radarPos);
				rcsCompute_->setBeamDirection(-radarPos.normalized());
				// Set beam width for ray generation to cover full visual extent (4× for SincBeam side lobes)
				float visualExtent = beamController_ ? beamController_->getVisualExtentDegrees() : 15.0f;
				rcsCompute_->setBeamWidth(visualExtent);
				rcsCompute_->compute();

				// Read hit results for visualization
				bool needHitResults = (reflectionRenderer_ && reflectionRenderer_->isVisible()) ||
				                       (heatMapRenderer_ && heatMapRenderer_->isVisible());
				if (needHitResults) {
					rcsCompute_->readHitBuffer();

					// Update reflection lobes
					if (reflectionRenderer_ && reflectionRenderer_->isVisible()) {
						reflectionRenderer_->updateLobes(rcsCompute_->getHitResults());
					}

					// Update heat map
					if (heatMapRenderer_ && heatMapRenderer_->isVisible()) {
						heatMapRenderer_->updateFromHits(rcsCompute_->getHitResults(), radius_);
					}

					// Sample RCS data for polar plot
					if (currentSampler_) {
						currentSampler_->sample(rcsCompute_->getHitResults(), polarPlotData_);
						emit polarPlotDataReady(polarPlotData_);
					}
				}
			}
		}

		// Render heat map (semi-transparent, render after sphere before beam)
		if (heatMapRenderer_ && heatMapRenderer_->isVisible()) {
			heatMapRenderer_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		// Render slicing plane visualization
		if (slicingPlaneRenderer_ && slicingPlaneRenderer_->isVisible()) {
			slicingPlaneRenderer_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		if (beamController_) {
			// Pass GPU shadow map from RCS compute to beam for ray-traced shadow
			if (rcsCompute_ && rcsCompute_->hasShadowMap() && wireframeController_ && wireframeController_->isVisible()) {
				QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);
				beamController_->setGPUShadowMap(rcsCompute_->getShadowMapTexture());
				beamController_->setGPUShadowEnabled(true);
				beamController_->setBeamAxis(-radarPos.normalized());
				beamController_->setBeamWidthRadians(rcsCompute_->getBeamWidthRadians());
				beamController_->setNumRings(rcsCompute_->getNumRings());
			} else {
				beamController_->setGPUShadowEnabled(false);
			}

			beamController_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		// Render reflection lobes (transparent, so render last)
		if (reflectionRenderer_ && reflectionRenderer_->isVisible()) {
			reflectionRenderer_->render(projectionMatrix, viewMatrix, modelMatrix);
		}
	}
	catch (const std::exception& e) {
		qCritical() << "Exception in RadarGLWidget::paintGL:" << e.what();
	}
	catch (...) {
		qCritical() << "Unknown exception in RadarGLWidget::paintGL";
	}

	// Release FBO if rendering to texture (before QPainter which doesn't work with FBO)
	if (renderToFBO_ && fboRenderer_ && fboRenderer_->isValid()) {
		fboRenderer_->release();
		// Skip 2D text rendering when in FBO mode - it would render to screen, not FBO
		return;
	}

	// Render 2D text labels for axes on top of 3D scene
	if (sphereRenderer_ && sphereRenderer_->areAxesVisible()) {
		// Calculate axis tip positions (same as in SphereRenderer::createAxesLines)
		float axisLength = radius_ * View::kAxisLengthMultiplier;
		QVector3D xTip(axisLength, 0.0f, 0.0f);
		QVector3D yTip(0.0f, axisLength, 0.0f);
		QVector3D zTip(0.0f, 0.0f, axisLength);

		// Project 3D positions to screen space
		QPointF xScreen = projectToScreen(xTip, projectionMatrix, viewMatrix, modelMatrix);
		QPointF yScreen = projectToScreen(yTip, projectionMatrix, viewMatrix, modelMatrix);
		QPointF zScreen = projectToScreen(zTip, projectionMatrix, viewMatrix, modelMatrix);

		// Use QPainter to render text
		QPainter painter(this);
		painter.setPen(Qt::white);
		QFont font = painter.font();
		font.setPointSize(UI::kAxisLabelFontSize);
		font.setBold(true);
		painter.setFont(font);

		// Offset text slightly from the arrow tip position
		int textOffset = UI::kTextOffsetPixels;

		// Draw labels
		painter.setPen(Qt::red);
		painter.drawText(QPointF(xScreen.x() + textOffset, xScreen.y()), "X");

		painter.setPen(QColor(0, 255, 0)); // Green
		painter.drawText(QPointF(yScreen.x() + textOffset, yScreen.y()), "Y");

		painter.setPen(Qt::blue);
		painter.drawText(QPointF(zScreen.x() + textOffset, zScreen.y()), "Z");

		painter.end();
	}
}

void RadarGLWidget::updateBeamPosition() {
	if (beamController_) {
		// Convert spherical coordinates to Cartesian
		QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);

		// Update beam position in the beam controller
		beamController_->updateBeamPosition(radarPos);
	}
}

void RadarGLWidget::setShowShadow(bool show) {
	if (beamController_) {
		beamController_->setShowShadow(show);
	}
	update();
}

bool RadarGLWidget::isShowShadow() const {
	return beamController_ ? beamController_->isShowShadow() : true;
}

void RadarGLWidget::setReflectionLobesVisible(bool visible) {
	if (reflectionRenderer_) {
		reflectionRenderer_->setVisible(visible);
	}
	update();
}

bool RadarGLWidget::areReflectionLobesVisible() const {
	return reflectionRenderer_ ? reflectionRenderer_->isVisible() : false;
}

void RadarGLWidget::setHeatMapVisible(bool visible) {
	if (heatMapRenderer_) {
		heatMapRenderer_->setVisible(visible);
	}
	update();
}

bool RadarGLWidget::isHeatMapVisible() const {
	return heatMapRenderer_ ? heatMapRenderer_->isVisible() : false;
}

void RadarGLWidget::setRenderToFBO(bool enable) {
	if (renderToFBO_ != enable) {
		renderToFBO_ = enable;
		update();  // Trigger repaint to update FBO content
	}
}

void RadarGLWidget::requestFBOResize(int width, int height) {
	if (fboRenderer_ && renderToFBO_) {
		// Only resize if larger than current (don't shrink for main widget)
		if (width > fboRenderer_->width() || height > fboRenderer_->height()) {
			makeCurrent();
			fboRenderer_->resize(width, height);
			doneCurrent();
			update();  // Trigger repaint with new FBO size
		}
	}
}

void RadarGLWidget::setRadius(float radius) {
	if (radius_ != radius) {
		radius_ = radius;
		beamDirty_ = true;

		if (sphereRenderer_) {
			sphereRenderer_->setRadius(radius);
		}

		if (beamController_) {
			beamController_->setSphereRadius(radius);
		}

		// Update slicing plane size
		if (slicingPlaneRenderer_) {
			slicingPlaneRenderer_->setSphereRadius(radius);
		}

		emit radiusChanged(radius);
		update();
	}
}

void RadarGLWidget::setAngles(float theta, float phi) {
	if (theta_ != theta || phi_ != phi) {
		theta_ = theta;
		beamDirty_ = true;

		phi_ = phi;
		if (radarSiteRenderer_) {
			radarSiteRenderer_->setPosition(theta, phi);
		}

		emit anglesChanged(theta, phi);
		update();
	}
}

void RadarGLWidget::mousePressEvent(QMouseEvent* event) {
	// Forward to camera controller
	if (cameraController_) {
		cameraController_->mousePressEvent(event);
		update(); // Request update after state change
	}
}

void RadarGLWidget::mouseMoveEvent(QMouseEvent* event) {
	if (cameraController_) {
		cameraController_->mouseMoveEvent(event);
		update(); // Request update after state change
	}
}

void RadarGLWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (cameraController_) {
		cameraController_->mouseReleaseEvent(event);
		update(); // Request update after state change
	}
}

void RadarGLWidget::wheelEvent(QWheelEvent* event) {
	if (cameraController_) {
		cameraController_->wheelEvent(event);
		update();
	}
}

void RadarGLWidget::mouseDoubleClickEvent(QMouseEvent* event) {
	// Shift+double-click triggers pop-out
	if (event->modifiers() & Qt::ShiftModifier) {
		emit popoutRequested();
		return;
	}

	// Regular double-click resets camera view
	if (cameraController_) {
		cameraController_->mouseDoubleClickEvent(event);
		update();
	}
}

QVector3D RadarGLWidget::sphericalToCartesian(float r, float thetaDeg, float phiDeg) {
	float theta = thetaDeg * kDegToRadF;
	float phi = phiDeg * kDegToRadF;
	return QVector3D(
		r * cos(phi) * cos(theta),
		r * cos(phi) * sin(theta),  // Y is now horizontal
		r * sin(phi)                // Z is now vertical (elevation)
	);
}

QPointF RadarGLWidget::projectToScreen(const QVector3D& worldPos, const QMatrix4x4& projection,
                                        const QMatrix4x4& view, const QMatrix4x4& model) {
	// Transform world position through model, view, and projection matrices
	QVector4D clipSpace = projection * view * model * QVector4D(worldPos, 1.0f);

	// Perform perspective divide to get normalized device coordinates (NDC)
	if (clipSpace.w() != 0.0f) {
		clipSpace /= clipSpace.w();
	}

	// Convert NDC to screen coordinates
	// NDC range is [-1, 1], screen range is [0, width] and [0, height]
	float screenX = (clipSpace.x() + 1.0f) * 0.5f * width();
	float screenY = (1.0f - clipSpace.y()) * 0.5f * height(); // Flip Y axis

	return QPointF(screenX, screenY);
}

// RCS slicing plane control methods
void RadarGLWidget::setRCSCutType(CutType type) {
	if (currentCutType_ != type) {
		currentCutType_ = type;
		if (type == CutType::Azimuth) {
			currentSampler_ = azimuthSampler_.get();
		} else {
			currentSampler_ = elevationSampler_.get();
		}
		// Update slicing plane visualization
		if (slicingPlaneRenderer_) {
			slicingPlaneRenderer_->setCutType(type);
		}
		// Update heat map slice parameters
		if (heatMapRenderer_) {
			heatMapRenderer_->setSliceParameters(type,
				currentSampler_ ? currentSampler_->getOffset() : 0.0f,
				currentSampler_ ? currentSampler_->getThickness() : 10.0f);
		}
		update();  // Trigger repaint to update polar plot
	}
}

void RadarGLWidget::setRCSPlaneOffset(float degrees) {
	if (currentSampler_) {
		currentSampler_->setOffset(degrees);
	}
	// Update slicing plane visualization
	if (slicingPlaneRenderer_) {
		slicingPlaneRenderer_->setOffset(degrees);
	}
	// Update heat map slice parameters
	if (heatMapRenderer_) {
		heatMapRenderer_->setSliceParameters(currentCutType_,
			degrees,
			currentSampler_ ? currentSampler_->getThickness() : 10.0f);
	}
	update();
}

float RadarGLWidget::getRCSPlaneOffset() const {
	return currentSampler_ ? currentSampler_->getOffset() : 0.0f;
}

void RadarGLWidget::setRCSSliceThickness(float degrees) {
	// Set on both samplers so switching doesn't reset the value
	if (azimuthSampler_) {
		azimuthSampler_->setThickness(degrees);
	}
	if (elevationSampler_) {
		elevationSampler_->setThickness(degrees);
	}
	// Update slicing plane visualization
	if (slicingPlaneRenderer_) {
		slicingPlaneRenderer_->setThickness(degrees);
	}
	// Update heat map slice parameters
	if (heatMapRenderer_) {
		heatMapRenderer_->setSliceParameters(currentCutType_,
			currentSampler_ ? currentSampler_->getOffset() : 0.0f,
			degrees);
	}
	update();
}

float RadarGLWidget::getRCSSliceThickness() const {
	return currentSampler_ ? currentSampler_->getThickness() : RS::Constants::kDefaultSliceThickness;
}

void RadarGLWidget::setRCSPlaneShowFill(bool show) {
	if (slicingPlaneRenderer_) {
		slicingPlaneRenderer_->setShowFill(show);
	}
	update();
}

bool RadarGLWidget::isRCSPlaneShowFill() const {
	return slicingPlaneRenderer_ ? slicingPlaneRenderer_->isShowFill() : true;
}
