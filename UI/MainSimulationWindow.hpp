#ifndef MSW_H 
#define MSW_H

#include <QMainWindow>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QImage>
#include <vector>

class MainSimulationWindow: public QMainWindow{
    public:
        explicit MainSimulationWindow(QWidget *parent = nullptr);
    private:
        void open_second_window();
        //void create_scene(int rows, int cols);
    
        QLabel *info_label;
        QGraphicsScene *view_scene; //world to draw on
        QGraphicsView *view_cam; //camera of the world
        vector<QGraphicsLineItem> graph_lines;

        const int SQUARE_LEN = 32;

        bool scene_instantiated;
};

#endif 
