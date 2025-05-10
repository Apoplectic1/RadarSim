// ---- main.cpp ----

#include <QApplication>
#include "RadarSim.h"

int main(int argc, char** argv) {
    QApplication a(argc, argv);
    RadarSim w;
    w.show();
    return a.exec();
}
