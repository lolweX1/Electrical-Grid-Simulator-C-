#include <QApplication>
#include <QPushButton>

#include "Wires/Wire.hpp"
#include "UI/MainSimulationWindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainSimulationWindow window;
    window.show();
    return app.exec();
}