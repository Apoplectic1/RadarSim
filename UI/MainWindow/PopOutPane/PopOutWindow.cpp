// ---- PopOutWindow.cpp ----

#include "PopOutWindow.h"
#include "TextureBlitWidget.h"
#include "RadarGLWidget.h"
#include "RadarSceneWidget.h"
#include "PolarRCSPlot.h"
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QKeyEvent>

PopOutWindow::PopOutWindow(Type type, QWidget* parent)
    : QWidget(parent, Qt::Window)  // Qt::Window makes it a separate top-level window
    , type_(type)
{
    // Set window flags for a proper floating window
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);

    // Create layout
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    // Set up UI based on type
    if (type_ == Type::RadarScene) {
        setupRadarSceneUI();
    } else {
        setupPolarPlotUI();
    }
}

PopOutWindow::~PopOutWindow()
{
    // Disconnect from data source if polar plot
    if (polarPlot_ && dataSource_) {
        disconnect(dataSource_, nullptr, polarPlot_, nullptr);
    }
}

void PopOutWindow::setupRadarSceneUI()
{
    blitWidget_ = new TextureBlitWidget(this);
    layout_->addWidget(blitWidget_);

    // Connect close request from blit widget (Shift+double-click)
    connect(blitWidget_, &TextureBlitWidget::closeRequested,
            this, &PopOutWindow::close);
}

void PopOutWindow::setupPolarPlotUI()
{
    polarPlot_ = new PolarRCSPlot(this);
    layout_->addWidget(polarPlot_);

    // Note: Data connection is set up in setSourcePolarPlot()
}

void PopOutWindow::setSourceRadarWidget(RadarGLWidget* source)
{
    if (type_ != Type::RadarScene || !blitWidget_) {
        return;
    }

    blitWidget_->setSourceWidget(source);
}

void PopOutWindow::setSourcePolarPlot(PolarRCSPlot* source, RadarSceneWidget* dataSource)
{
    if (type_ != Type::PolarPlot || !polarPlot_) {
        return;
    }

    dataSource_ = dataSource;

    // Copy scale settings from source
    if (source) {
        // PolarRCSPlot doesn't expose scale getters, so we use defaults
        // The data signal will update the plot automatically
    }

    // Connect to the same data source that feeds the original polar plot
    if (dataSource_) {
        connect(dataSource_, &RadarSceneWidget::polarPlotDataReady,
                polarPlot_, &PolarRCSPlot::setData);
    }
}

void PopOutWindow::closeEvent(QCloseEvent* event)
{
    emit windowClosed();
    event->accept();
}

void PopOutWindow::keyPressEvent(QKeyEvent* event)
{
    // ESC key closes the window
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }

    QWidget::keyPressEvent(event);
}
