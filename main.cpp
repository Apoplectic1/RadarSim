// ---- main.cpp ----

#include <QApplication>
#include <QtGlobal>
#include <QDebug>
#include "RadarSim.h"

int main(int argc, char** argv) {
    qDebug() << "Qt version:" << QT_VERSION_STR;
    qDebug() << "Compiled with Qt version:" << QT_VERSION_STR;

    QApplication a(argc, argv);

    qDebug() << "Creating main window";
    RadarSim w;

    qDebug() << "Showing main window";
    w.show();

    qDebug() << "Entering event loop";
    return a.exec();
}
