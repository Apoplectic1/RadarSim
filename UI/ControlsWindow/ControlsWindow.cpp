// ---- ControlsWindow.cpp ----
// Floating window for radar/target/RCS plane controls

#include "ControlsWindow.h"

#include <QVBoxLayout>
#include <QCloseEvent>

ControlsWindow::ControlsWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)  // Qt::Window makes it a separate top-level window
{
    setWindowTitle("Controls");
    setupUI();
}

void ControlsWindow::setupUI()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    mainLayout_->setSpacing(0);

    // Set fixed width to match the controls panel
    setFixedWidth(270);
}

void ControlsWindow::setContent(QWidget* content)
{
    if (content) {
        // Remove fixed width from content widget since window handles sizing
        content->setFixedWidth(QWIDGETSIZE_MAX);
        content->setMinimumWidth(0);
        mainLayout_->addWidget(content);
    }
}

void ControlsWindow::closeEvent(QCloseEvent* event)
{
    // Hide instead of close (window can be reopened via View menu)
    hide();
    event->ignore();
}
