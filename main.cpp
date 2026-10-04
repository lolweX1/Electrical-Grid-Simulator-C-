#include <QApplication>

#include "UI/MainSimulationWindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainSimulationWindow window;
    window.show();
    return app.exec();
}