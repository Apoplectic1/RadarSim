// RadarGLWidget.cpp
#include "RadarSceneWidget/RadarGLWidget.h"
#include "GLUtils.h"
#include "Constants.h"
#include <QActionGroup>
#include <QDebug>
#include <QPainter>
#include <QFont>

using namespace RadarSim::Constants;

RadarGLWidget::RadarGLWidget(QWidget* parent)
	: QOpenGLWidget(parent),
	radius_(Defaults::kSphereRadius),
	theta_(Defaults::kRadarTheta),
	phi_(Defaults::kRadarPhi),
	sphereRenderer_(nullptr),
	beamController_(nullptr),
	cameraController_(nullptr),
	modelManager_(nullptr),
	wireframeController_(nullptr),
	contextMenu_(new QMenu(this))
{
	// Set focus policy to receive keyboard events
	setFocusPolicy(Qt::StrongFocus);

	// Set up context menu
	setupContextMenu();
}

RadarGLWidget::~RadarGLWidget() {
	// Clean up OpenGL resources while context is still valid
	// Must do this before base class destructor destroys the context
	makeCurrent();
	cleanupGL();

	// Reset unique_ptrs while context is still current
	// This ensures VAO/VBO destructors run with valid context
	sphereRenderer_.reset();
	beamController_.reset();
	cameraController_.reset();
	modelManager_.reset();
	wireframeController_.reset();
	rcsCompute_.reset();

	doneCurrent();

	// contextMenu_ is parented to 'this' and auto-deleted by Qt
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

	// Clean up component resources
	if (sphereRenderer_) {
		sphereRenderer_->cleanup();
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

	// Enable context menu for right-click
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested,
		this, [this](const QPoint& pos) {
			if (contextMenu_) {
				contextMenu_->popup(mapToGlobal(pos));
			}
		});

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

		// Force beam geometry creation with initial position
		if (beamController_) {
			QVector3D initialPos = sphericalToCartesian(radius_, theta_, phi_);
			beamController_->updateBeamPosition(initialPos);
			beamController_->rebuildBeamGeometry();
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
	QMatrix4x4 projectionMatrix;
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(View::kPerspectiveFOV, float(width()) / float(height()), View::kNearPlane, View::kFarPlane);

	// Draw components
	try {
		if (sphereRenderer_) {
			sphereRenderer_->render(projectionMatrix, viewMatrix, modelMatrix);
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
				rcsCompute_->compute();
			}
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
	}
	catch (const std::exception& e) {
		qCritical() << "Exception in RadarGLWidget::paintGL:" << e.what();
	}
	catch (...) {
		qCritical() << "Unknown exception in RadarGLWidget::paintGL";
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

void RadarGLWidget::setRadius(float radius) {
	if (radius_ != radius) {
		radius_ = radius;
		beamDirty_ = true;

		if (sphereRenderer_) {
			sphereRenderer_->setRadius(radius);
		}

		// ADD THIS:
		if (beamController_) {
			beamController_->setSphereRadius(radius);
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
		if (sphereRenderer_) {
			sphereRenderer_->setRadarPosition(theta, phi);
		}
		
		emit anglesChanged(theta, phi);
		update();
	}
}

void RadarGLWidget::mousePressEvent(QMouseEvent* event) {
	// If right-click is for context menu, let it bubble up
	if (event->button() == Qt::RightButton && contextMenu_) {
		// Let the context menu event handle it
		QOpenGLWidget::mousePressEvent(event);
		return;
	}

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
	if (cameraController_) {
		cameraController_->mouseDoubleClickEvent(event);
		update();
	}
}

void RadarGLWidget::contextMenuEvent(QContextMenuEvent* event) {
	contextMenu_->popup(event->globalPos());
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

void RadarGLWidget::setupContextMenu() {
	// Reset view
	QAction* resetAction = contextMenu_->addAction("Reset View");
	connect(resetAction, &QAction::triggered, [this]() {
		if (cameraController_) {
			cameraController_->resetView();
			update();
		}
		});

	// Add separator
	contextMenu_->addSeparator();

	// Toggle axes
	QAction* toggleAxesAction = contextMenu_->addAction("Toggle Axes");
	toggleAxesAction->setCheckable(true);
	toggleAxesAction->setChecked(true);
	connect(toggleAxesAction, &QAction::toggled, [this](bool checked) {
		if (sphereRenderer_) {
			sphereRenderer_->setAxesVisible(checked);
			update();
		}
		});

	// Toggle sphere
	QAction* toggleSphereAction = contextMenu_->addAction("Toggle Sphere");
	toggleSphereAction->setCheckable(true);
	toggleSphereAction->setChecked(true);
	connect(toggleSphereAction, &QAction::toggled, [this](bool checked) {
		if (sphereRenderer_) {
			sphereRenderer_->setSphereVisible(checked);
			update();
		}
		});

	// Toggle grid lines
	QAction* toggleGridLinesAction = contextMenu_->addAction("Toggle Grid");
	toggleGridLinesAction->setCheckable(true);
	toggleGridLinesAction->setChecked(true);
	connect(toggleGridLinesAction, &QAction::toggled, [this](bool checked) {
		if (sphereRenderer_) {
			sphereRenderer_->setGridLinesVisible(checked);
			update();
		}
		});

	// Toggle inertia
	QAction* toggleInertiaAction = contextMenu_->addAction("Toggle Inertia");
	toggleInertiaAction->setCheckable(true);
	toggleInertiaAction->setChecked(false);
	connect(toggleInertiaAction, &QAction::toggled, [this](bool checked) {
		if (cameraController_) {
			cameraController_->setInertiaEnabled(checked);
		}
		});

	// Toggle beam
	QAction* toggleBeamAction = contextMenu_->addAction("Toggle Beam");
	toggleBeamAction->setCheckable(true);
	toggleBeamAction->setChecked(true);
	connect(toggleBeamAction, &QAction::toggled, [this](bool checked) {
		// Update BeamController's beam (SphereRenderer's beam is disabled)
		if (beamController_) {
			beamController_->setBeamVisible(checked);
		}

		update();
		});

	// Add beam type submenu
	QMenu* beamTypeMenu = contextMenu_->addMenu("Beam Type");

	QAction* conicalBeamAction = beamTypeMenu->addAction("Conical");
	conicalBeamAction->setCheckable(true);
	conicalBeamAction->setChecked(true);

	QAction* phasedBeamAction = beamTypeMenu->addAction("Phased Array");
	phasedBeamAction->setCheckable(true);

	// Make them exclusive
	QActionGroup* beamTypeGroup = new QActionGroup(this);
	beamTypeGroup->addAction(conicalBeamAction);
	beamTypeGroup->addAction(phasedBeamAction);
	beamTypeGroup->setExclusive(true);

	// Connect beam type actions
	connect(conicalBeamAction, &QAction::triggered, [this]() {
		if (beamController_) {
			beamController_->setBeamType(BeamType::Conical);
			beamDirty_ = true;  // Force geometry rebuild with GL context
			update();
		}
		});

	connect(phasedBeamAction, &QAction::triggered, [this]() {
		if (beamController_) {
			beamController_->setBeamType(BeamType::Phased);
			beamDirty_ = true;  // Force geometry rebuild with GL context
			update();
		}
		});

	// Add separator before target menu
	contextMenu_->addSeparator();

	// Add target submenu
	QMenu* targetMenu = contextMenu_->addMenu("Target");

	// Toggle target visibility
	QAction* toggleTargetAction = targetMenu->addAction("Show Target");
	toggleTargetAction->setCheckable(true);
	toggleTargetAction->setChecked(true);
	connect(toggleTargetAction, &QAction::toggled, [this](bool checked) {
		if (wireframeController_) {
			wireframeController_->setVisible(checked);
			update();
		}
		});

	// Target type submenu
	QMenu* targetTypeMenu = targetMenu->addMenu("Type");

	QAction* cubeTargetAction = targetTypeMenu->addAction("Cube");
	cubeTargetAction->setCheckable(true);
	cubeTargetAction->setChecked(true);

	QAction* cylinderTargetAction = targetTypeMenu->addAction("Cylinder");
	cylinderTargetAction->setCheckable(true);

	QAction* aircraftTargetAction = targetTypeMenu->addAction("Aircraft");
	aircraftTargetAction->setCheckable(true);

	// Make target types exclusive
	QActionGroup* targetTypeGroup = new QActionGroup(this);
	targetTypeGroup->addAction(cubeTargetAction);
	targetTypeGroup->addAction(cylinderTargetAction);
	targetTypeGroup->addAction(aircraftTargetAction);
	targetTypeGroup->setExclusive(true);

	// Connect target type actions
	connect(cubeTargetAction, &QAction::triggered, [this]() {
		if (wireframeController_) {
			wireframeController_->setTargetType(WireframeType::Cube);
			update();
		}
		});

	connect(cylinderTargetAction, &QAction::triggered, [this]() {
		if (wireframeController_) {
			wireframeController_->setTargetType(WireframeType::Cylinder);
			update();
		}
		});

	connect(aircraftTargetAction, &QAction::triggered, [this]() {
		if (wireframeController_) {
			wireframeController_->setTargetType(WireframeType::Aircraft);
			update();
		}
		});
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
