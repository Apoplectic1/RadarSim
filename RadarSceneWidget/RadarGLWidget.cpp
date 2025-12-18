// RadarGLWidget.cpp
#include "RadarSceneWidget/RadarGLWidget.h"
#include <QActionGroup>
#include <QDebug>

RadarGLWidget::RadarGLWidget(QWidget* parent)
	: QOpenGLWidget(parent),
	radius_(100.0f),
	theta_(45.0f),
	phi_(45.0f),
	sphereRenderer_(nullptr),
	beamController_(nullptr),
	cameraController_(nullptr),
	modelManager_(nullptr),
	wireframeController_(nullptr),
	contextMenu_(new QMenu(this))
{
	qDebug() << "Creating RadarGLWidget";

	// Set focus policy to receive keyboard events
	setFocusPolicy(Qt::StrongFocus);

	// Set up context menu
	setupContextMenu();

	qDebug() << "RadarGLWidget constructor complete";
}

RadarGLWidget::~RadarGLWidget() {
	qDebug() << "RadarGLWidget destructor called";

	// Clean up context menu
	delete contextMenu_;
}

void RadarGLWidget::initialize(SphereRenderer* sphereRenderer, BeamController* beamController, CameraController* cameraController, ModelManager* modelManager, WireframeTargetController* wireframeController) {
	qDebug() << "RadarGLWidget::initialize called";

	sphereRenderer_ = sphereRenderer;
	beamController_ = beamController;
	cameraController_ = cameraController;
	modelManager_ = modelManager;
	wireframeController_ = wireframeController;

	// Store the components but don't initialize them here
	// They will be initialized in initializeGL when a valid context exists
	// Set mouse tracking to improve responsiveness
	setMouseTracking(true);

	// Connect camera controller's viewChanged signal to trigger repaints
	// This is essential for smooth inertia animation
	if (cameraController_) {
		connect(cameraController_, &CameraController::viewChanged,
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
	qDebug() << "RadarGLWidget::initializeGL called";

	// Initialize OpenGL functions
	initializeOpenGLFunctions();

	// Set up OpenGL
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	// Make sure the context is current
	if (!context()->isValid()) {
		qCritical() << "OpenGL context is not valid!";
		return;
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
			modelManager_->initialize();
		}

		if (wireframeController_) {
			wireframeController_->initialize();
		}
	}
	catch (const std::exception& e) {
		qCritical() << "Exception during component initialization:" << e.what();
	}
	catch (...) {
		qCritical() << "Unknown exception during component initialization";
	}

	qDebug() << "RadarGLWidget::initializeGL complete";
}

void RadarGLWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);
}

void RadarGLWidget::paintGL() {
	qDebug() << "RadarGLWidget::paintGL starting";

	// Re‐generate beam geometry now that we have a current context
	if (beamDirty_) {
		updateBeamPosition();
		beamDirty_ = false;

		// GPU: re-upload the new geometry into your VAO/VBO/EBO
		beamController_->rebuildBeamGeometry();
	}

	// Rebuild wireframe geometry if type changed
	if (wireframeController_) {
		wireframeController_->rebuildGeometry();
	}

	// Ensure blending and stencil are in clean state
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);

	// Clear all buffers including stencil for shadow volume rendering
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearStencil(0);
	glStencilMask(0xFF);  // Enable stencil writing for clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Check context
	if (!context() || !context()->isValid()) {
		qWarning() << "OpenGL context not valid in paintGL";
		return;
	}

	// Check camera controller
	if (!cameraController_) {
		qWarning() << "CameraController not available in paintGL";
		return;
	}

	// Get view and model matrices from camera controller
	QMatrix4x4 viewMatrix = cameraController_->getViewMatrix();
	QMatrix4x4 modelMatrix = cameraController_->getModelMatrix();

	// Debug camera matrices
	static QElapsedTimer debugTimer;
	static bool timerInitialized = false;

	if (!timerInitialized) {
		debugTimer.start();
		timerInitialized = true;
	}

	// Log matrices occasionally to avoid console spam
	if (debugTimer.elapsed() > 1000) { // Once per second
		qDebug() << "Camera matrices:";
		qDebug() << "  View:" << viewMatrix;
		qDebug() << "  Model:" << modelMatrix;
		debugTimer.restart();
	}

	// Set up projection matrix
	QMatrix4x4 projectionMatrix;
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(45.0f, float(width()) / float(height()), 0.1f, 2000.0f);

	// Log component state
	qDebug() << "  SphereRenderer available:" << (sphereRenderer_ != nullptr);
	qDebug() << "  BeamController available:" << (beamController_ != nullptr);
	qDebug() << "  ModelManager available:" << (modelManager_ != nullptr);
	qDebug() << "  WireframeController available:" << (wireframeController_ != nullptr);

	// Draw components with careful error trapping
	try {
		if (sphereRenderer_) {
			qDebug() << "  Rendering sphere...";
			sphereRenderer_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		if (modelManager_) {
			qDebug() << "  Rendering models...";
			modelManager_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		if (wireframeController_) {
			qDebug() << "  Rendering wireframe target...";
			// Pass radar position and sphere radius for shadow volume calculation
			QVector3D radarPos = sphericalToCartesian(radius_, theta_, phi_);
			wireframeController_->render(projectionMatrix, viewMatrix, modelMatrix, radarPos, radius_);
		}

		if (beamController_) {
			qDebug() << "  Rendering beam...";
			beamController_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		qDebug() << "RadarGLWidget::paintGL completed successfully";
	}
	catch (const std::exception& e) {
		qCritical() << "Exception in RadarGLWidget::paintGL:" << e.what();
	}
	catch (...) {
		qCritical() << "Unknown exception in RadarGLWidget::paintGL";
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
	const float toRad = float(M_PI / 180.0);
	float theta = thetaDeg * toRad;
	float phi = phiDeg * toRad;
	return QVector3D(
		r * cos(phi) * cos(theta),
		r * sin(phi),
		r * cos(phi) * sin(theta)
	);
}

void RadarGLWidget::setupContextMenu() {
	qDebug() << "Setting up context menu";

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
