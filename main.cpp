// ---- main.cpp ----

#include <QApplication>
#include <QtGlobal>
#include <QDebug>
#include "RadarSim.h"
#include <QSurfaceFormat>

int main(int argc, char** argv) {
    qDebug() << "Qt version:" << QT_VERSION_STR;
    qDebug() << "Compiled with Qt version:" << QT_VERSION_STR;

    // Set up OpenGL format for better debugging
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);

    qSetMessagePattern("[%{time}] %{type} %{function}: %{message}");
    QApplication a(argc, argv);

    qDebug() << "Creating main window";
    RadarSim w;

    qDebug() << "Showing main window";
    w.show();

    qDebug() << "Entering event loop";
    return a.exec();
}
