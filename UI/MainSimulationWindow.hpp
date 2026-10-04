#ifndef MAIN_SIMULATION_WINDOW_HPP
#define MAIN_SIMULATION_WINDOW_HPP

#include "../Objects/ParentOfObjects.hpp"
#include "../Wires/Connections.hpp"

#include <QElapsedTimer>
#include <QMainWindow>
#include <QPointF>
#include <QTimer>
#include <memory>
#include <vector>

class QDoubleSpinBox;
class QCheckBox;
class QGraphicsEllipseItem;
class QGraphicsPathItem;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QListWidget;
class QAction;
class QFormLayout;
class QComboBox;
class QPushButton;
class QWidget;
class QDialog;
class QTableWidget;

class MainSimulationWindow : public QMainWindow {
public:
    explicit MainSimulationWindow(QWidget* parent = nullptr);

private:
    void selectTool(const QString& tool);
    void updateGhost(const QPointF& position);
    void rotateCurrent();
    void deleteSelectedObject();
    void selectWire(int id);
    std::unique_ptr<ParentOfObjects> createObject(
        const QString& tool, int id, int gridX, int gridY) const;
    void handleSceneClick(const QPointF& position, Qt::MouseButton button,
                          int objectId, int terminal);
    void placeObject(const QPointF& position);
    void selectObject(int id);
    void refreshObjectGraphics();
    void refreshWireGraphics();
    void refreshInspector();
    void runSimulation();
    void setSimulationRunning(bool running);
    void resetSimulation();
    void updateWireGhost(const QPointF& position);
    bool canPlace(const ParentOfObjects& candidate, int ignoredObjectId = -1) const;
    void saveSimulation();
    void loadSimulation();
    void displayAllInformation();
    void refreshInformationTable();

    QLabel* infoLabel = nullptr;
    QLabel* simulationLabel = nullptr;
    QFormLayout* propertyForm = nullptr;
    QGraphicsScene* viewScene = nullptr;
    QGraphicsView* view = nullptr;
    QGraphicsPixmapItem* ghostItem = nullptr;
    QGraphicsRectItem* ghostFrame = nullptr;
    QLabel* mouseCellLabel = nullptr;
    QLabel* wireReadout = nullptr;
    QListWidget* palette = nullptr;
    QDoubleSpinBox* generatorVoltage = nullptr;
    QDoubleSpinBox* generatorCurrent = nullptr;
    QDoubleSpinBox* houseDemand = nullptr;
    QDoubleSpinBox* houseVoltage = nullptr;
    QDoubleSpinBox* connectionHeight = nullptr;
    QCheckBox* separatorClosed = nullptr;
    QComboBox* wireMaterial = nullptr;
    QDoubleSpinBox* wireResistivity = nullptr;
    QWidget* sagGraph = nullptr;
    QPushButton* simulationButton = nullptr;
    QDialog* informationDialog = nullptr;
    QTableWidget* informationTable = nullptr;
    QTimer simulationTimer;
    QElapsedTimer elapsedTimer;

    std::vector<std::unique_ptr<ParentOfObjects>> objects;
    Connections connections;
    std::vector<QGraphicsPixmapItem*> objectItems;
    std::vector<QGraphicsRectItem*> objectFrames;
    std::vector<int> objectImageRotations;
    std::vector<bool> objectUnderpowered;
    std::vector<std::vector<QGraphicsEllipseItem*>> objectTerminals;
    std::vector<QGraphicsPathItem*> wireItems;
    QGraphicsPathItem* ghostWireItem = nullptr;
    std::unique_ptr<ParentOfObjects> ghostObject;

    QString activeTool = "none";
    int selectedObjectId = -1;
    int selectedWireId = -1;
    int pendingWireSourceId = -1;
    int pendingWireSourceTerminal = -1;
    int nextObjectId = 1;
    int nextWireId = 1;
    int lastGhostGridX = 0;
    int lastGhostGridY = 0;
    bool pointerOverGrid = false;
    bool simulationRunning = false;
    double simulationSpeedMultiplier = 10.0;
    static constexpr double gridSize = 64.0;
};

#endif
