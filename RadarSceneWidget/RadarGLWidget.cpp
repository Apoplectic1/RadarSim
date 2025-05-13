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

void RadarGLWidget::initialize(SphereRenderer* sphereRenderer, BeamController* beamController, CameraController* cameraController, ModelManager* modelManager) {
	qDebug() << "RadarGLWidget::initialize called";

	sphereRenderer_ = sphereRenderer;
	beamController_ = beamController;
	cameraController_ = cameraController;
	modelManager_ = modelManager;

	// Store the components but don't initialize them here
	// They will be initialized in initializeGL when a valid context exists
}

void RadarGLWidget::initializeGL() {
	qDebug() << "RadarGLWidget::initializeGL called";

	// Initialize OpenGL functions
	initializeOpenGLFunctions();

	// Set up OpenGL
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
		}

		if (modelManager_) {
			modelManager_->initialize();
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
	// Clear the buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Verify context is valid
	if (!context() || !context()->isValid()) {
		qWarning() << "OpenGL context not valid in paintGL";
		return;
	}

	if (!cameraController_) {
		qWarning() << "CameraController not available in paintGL";
		return;
	}

	// Get view and model matrices from camera controller
	QMatrix4x4 viewMatrix = cameraController_->getViewMatrix();
	QMatrix4x4 modelMatrix = cameraController_->getModelMatrix();

	// Set up projection matrix
	QMatrix4x4 projectionMatrix;
	projectionMatrix.setToIdentity();
	projectionMatrix.perspective(45.0f, float(width()) / float(height()), 0.1f, 2000.0f);

	// Render sphere and grid
	try {
		if (sphereRenderer_) {
			sphereRenderer_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		// Render models
		if (modelManager_) {
			modelManager_->render(projectionMatrix, viewMatrix, modelMatrix);
		}

		// Render radar beam
		if (beamController_) {
			beamController_->render(projectionMatrix, viewMatrix, modelMatrix);
		}
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

		// Update sphere renderer
		if (sphereRenderer_) {
			sphereRenderer_->setRadius(radius);
		}

		// Update beam position
		updateBeamPosition();

		emit radiusChanged(radius);
		update();
	}
}

void RadarGLWidget::setAngles(float theta, float phi) {
	if (theta_ != theta || phi_ != phi) {
		theta_ = theta;
		phi_ = phi;

		// Update beam position
		updateBeamPosition();

		emit anglesChanged(theta, phi);
		update();
	}
}

void RadarGLWidget::mousePressEvent(QMouseEvent* event) {
	if (cameraController_) {
		cameraController_->mousePressEvent(event);
		update();
	}
}

void RadarGLWidget::mouseMoveEvent(QMouseEvent* event) {
	if (cameraController_) {
		cameraController_->mouseMoveEvent(event);
		update();
	}
}

void RadarGLWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (cameraController_) {
		cameraController_->mouseReleaseEvent(event);
		update();
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
		if (beamController_) {
			beamController_->setBeamVisible(checked);
			update();
		}
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
	connect(conicalBeamAction, &QAction::triggered, [this]() {
		if (beamController_) {
			beamController_->setBeamType(BeamType::Conical);
			update();
		}
		});

	connect(ellipticalBeamAction, &QAction::triggered, [this]() {
		if (beamController_) {
			beamController_->setBeamType(BeamType::Elliptical);
			update();
		}
		});

	connect(phasedBeamAction, &QAction::triggered, [this]() {
		if (beamController_) {
			beamController_->setBeamType(BeamType::Phased);
			update();
		}
		});
}
