#include "MainSimulationWindow.hpp"

#include <QAction>
#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

MainSimulationWindow::MainSimulationWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Grid Simulator");
    resize(900, 600);

    // ---- central widget: a container with a layout ----
    auto *central = new QWidget;
    auto *layout  = new QVBoxLayout(central);
    info_label = new QLabel("Nothing selected");
    auto *button  = new QPushButton("Open second window");
    layout->addWidget(info_label, 1);   // the 1 = stretch: label takes extra space
    layout->addWidget(button);
    setCentralWidget(central);

    // ---- actions: one object, reusable in menus AND toolbars ----
    auto *quit = new QAction("&Quit", this);
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);

    auto *about = new QAction("&About", this);
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::information(this, "About", "Electrical Grid Simulator");
    });

    // ---- menu bar, toolbar, status bar ----
    menuBar()->addMenu("&File")->addAction(quit);
    menuBar()->addMenu("&Help")->addAction(about);

    addToolBar("Tools")->addAction(quit);

    statusBar()->showMessage("Ready", 3000);   // disappears after 3 s

    // ---- dock widget: a dockable side panel ----
    auto *dock = new QDockWidget("Palette", this);
    auto *list = new QListWidget;
    list->addItems({"House 8x4", "Utility pole", "Power generator"});
    dock->setWidget(list);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        info_label->setText("Selected: " + item->text());
    });

    // ---- second window ----
    connect(button, &QPushButton::clicked, this, [this] { open_second_window(); });
}

void MainSimulationWindow::open_second_window() {
    auto *w = new QWidget(this, Qt::Window);      // child of this, but its own window
    w->setAttribute(Qt::WA_DeleteOnClose);        // free memory when closed
    w->setWindowTitle("Second window");
    auto *l = new QVBoxLayout(w);
    l->addWidget(new QLabel("I'm a separate window"));
    w->resize(300, 200);
    w->show();
}