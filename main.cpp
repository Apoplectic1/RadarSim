// ---- main.cpp ----

#include <QApplication>
#include "RadarSim.h"

int main(int argc, char** argv) {
    // Set up OpenGL format for better debugging
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(4, 6);  // OpenGL 4.6 - latest version, full feature set
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);

    qSetMessagePattern("[%{time}] %{type} %{function}: %{message}");
    QApplication a(argc, argv);

    RadarSim w;
    w.show();

    return a.exec();
}
