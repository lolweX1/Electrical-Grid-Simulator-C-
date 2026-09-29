#ifndef MSW_H 
#define MSW_H

#include <QMainWindow>
#include <QLabel>

class MainSimulationWindow: public QMainWindow{
    public:
        explicit MainSimulationWindow(QWidget *parent = nullptr);
    private:
        void open_second_window();
        QLabel *info_label;
};

#endif 
