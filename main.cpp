#include <QApplication>
#include <QPushButton>

int main(int argc, char *argv[]) {
    // 1. Initialize the application framework and manage resources
    QApplication app(argc, argv);

    // 2. Create a standard push button widget
    QPushButton button("Hello, Qt World!");
    button.resize(200, 60);
    button.show(); // Windows/Widgets are hidden by default

    // 3. Enter the main event loop and wait for user interaction
    return app.exec();
}