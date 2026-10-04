#include "MainSimulationWindow.hpp"

#include "../Objects/Apartment.hpp"
#include "../Objects/GroundWireConnection.hpp"
#include "../Objects/House.hpp"
#include "../Objects/PowerGenerator.hpp"
#include "../Objects/UtilityPole.hpp"
#include "../Objects/WireSeparator.hpp"

#include <QAction>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsColorizeEffect>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QKeyEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QSaveFile>
#include <QTableWidget>
#include <QHeaderView>
#include <QResizeEvent>
#include <QStatusBar>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QToolBar>
#include <QTransform>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>

namespace {
QPointF terminalScenePosition(const ParentOfObjects& object, int terminal,
                              double gridSize)
{
    const QPointF center((object.x() + object.width() / 2.0) * gridSize,
                         (object.y() + object.height() / 2.0) * gridSize);
    if (object.kind() == ObjectKind::UtilityPole ||
        object.kind() == ObjectKind::GroundWireConnection) {
        return center;
    }
    if (object.kind() == ObjectKind::PowerGenerator) {
        const double offset =
            (static_cast<double>(terminal % 4) + 0.5) / 4.0;
        return terminal < 4
            ? QPointF(center.x() - object.width() * gridSize / 2.0,
                      center.y() + (offset - 0.5) * object.height() * gridSize)
            : QPointF(center.x() + object.width() * gridSize / 2.0,
                      center.y() + (offset - 0.5) * object.height() * gridSize);
    }
    PortSide side = object.terminalSide(terminal);
    for (int turn = 0; turn < object.rotation(); ++turn) {
        switch (side) {
        case PortSide::Left:
            side = PortSide::Top;
            break;
        case PortSide::Top:
            side = PortSide::Right;
            break;
        case PortSide::Right:
            side = PortSide::Bottom;
            break;
        case PortSide::Bottom:
            side = PortSide::Left;
            break;
        }
    }
    switch (side) {
    case PortSide::Left:
        return center + QPointF(-object.width() * gridSize / 2.0, 0.0);
    case PortSide::Top:
        return center + QPointF(0.0, -object.height() * gridSize / 2.0);
    case PortSide::Right:
        return center + QPointF(object.width() * gridSize / 2.0, 0.0);
    case PortSide::Bottom:
        return center + QPointF(0.0, object.height() * gridSize / 2.0);
    }
    return center;
}

class GridLinesItem : public QGraphicsItem {
public:
    QRectF boundingRect() const override
    {
        return QRectF(-32000.0, -32000.0, 64000.0, 64000.0);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget*) override
    {
        const QRectF rect = option->exposedRect;
        painter->save();
        painter->setClipRect(rect);
        QPen gridPen(QColor(145, 163, 182), 0);
        painter->setPen(gridPen);
        const int firstX = static_cast<int>(std::floor(rect.left() / 64.0)) * 64;
        const int firstY = static_cast<int>(std::floor(rect.top() / 64.0)) * 64;
        for (int x = firstX; x <= rect.right(); x += 64) {
            painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
        }
        for (int y = firstY; y <= rect.bottom(); y += 64) {
            painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
        }
        QPen majorPen(QColor(115, 137, 160), 0);
        painter->setPen(majorPen);
        const int majorX = static_cast<int>(std::floor(rect.left() / 320.0)) * 320;
        const int majorY = static_cast<int>(std::floor(rect.top() / 320.0)) * 320;
        for (int x = majorX; x <= rect.right(); x += 320) {
            painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
        }
        for (int y = majorY; y <= rect.bottom(); y += 320) {
            painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
        }
        painter->restore();
    }
};

class SagGraph : public QWidget {
public:
    explicit SagGraph(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(240, 150);
        setMaximumHeight(190);
    }

    void setWire(const Wire* selectedWire)
    {
        wire = selectedWire;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(250, 251, 253));
        const QRectF plot(48.0, 24.0, width() - 62.0, height() - 52.0);
        painter.setPen(QPen(QColor(110, 125, 140), 1.0));
        painter.drawLine(plot.bottomLeft(), plot.bottomRight());
        painter.drawLine(plot.bottomLeft(), plot.topLeft());
        painter.drawText(QRectF(4.0, 0.0, width() - 8.0, 20.0),
                         Qt::AlignCenter, "Wire height profile");
        painter.save();
        painter.translate(12.0, plot.center().y());
        painter.rotate(-90.0);
        painter.drawText(QRectF(-plot.height() / 2.0, -9.0,
                                plot.height(), 18.0),
                         Qt::AlignCenter, "Height above ground (m)");
        painter.restore();
        if (!wire) {
            painter.drawText(plot, Qt::AlignCenter, "Select a wire to view height");
            return;
        }

        const double maximumHeight =
            std::max(wire->fromHeightMeters(), wire->toHeightMeters());
        const double heightScale = maximumHeight > 0.0 ? maximumHeight : 1.0;
        const auto pointFor = [&plot, heightScale](double distanceRatio,
                                                    double height) {
            return QPointF(
                plot.left() + distanceRatio * plot.width(),
                plot.bottom() - height / heightScale * plot.height());
        };
        painter.setPen(QPen(QColor(165, 72, 65), 1.0, Qt::DashLine));
        if (maximumHeight >= 5.0) {
            const double clearanceY = pointFor(0.0, 5.0).y();
            painter.drawLine(QPointF(plot.left(), clearanceY),
                             QPointF(plot.right(), clearanceY));
            painter.drawText(QRectF(0.0, clearanceY - 9.0, 44.0, 18.0),
                             Qt::AlignRight | Qt::AlignVCenter, "5 m");
        }

        QPainterPath profile;
        for (int sample = 0; sample <= 80; ++sample) {
            const double t = sample / 80.0;
            const double height =
                wire->fromHeightMeters() +
                (wire->toHeightMeters() - wire->fromHeightMeters()) * t -
                4.0 * wire->sagMeters() * t * (1.0 - t);
            const QPointF point = pointFor(t, height);
            if (sample == 0) {
                profile.moveTo(point);
            } else {
                profile.lineTo(point);
            }
        }
        painter.setPen(QPen(wire->isLowClearance() ? QColor(220, 35, 35) :
                                                    QColor(54, 112, 166), 2.5));
        painter.drawPath(profile);
        painter.setPen(QColor(55, 67, 80));
        painter.drawText(QRectF(0.0, plot.top(), 44.0, 20.0), Qt::AlignRight,
                         QString("%1 m").arg(maximumHeight, 0, 'f', 1));
        painter.drawText(QRectF(0.0, plot.bottom() - 18.0, 44.0, 18.0),
                         Qt::AlignRight, "0");
        painter.drawText(QRectF(plot.left(), plot.bottom() + 5.0,
                                plot.width(), 18.0),
                         Qt::AlignCenter,
                         QString("Distance along wire: %1 m")
                             .arg(wire->length(), 0, 'f', 1));
    }

private:
    const Wire* wire = nullptr;
};

QString objectKindKey(ObjectKind kind)
{
    switch (kind) {
    case ObjectKind::House:
        return "house";
    case ObjectKind::Apartment:
        return "apartment";
    case ObjectKind::UtilityPole:
        return "pole";
    case ObjectKind::GroundWireConnection:
        return "ground";
    case ObjectKind::WireSeparator:
        return "separator";
    case ObjectKind::PowerGenerator:
        return "generator";
    }
    return {};
}

class GridScene : public QGraphicsScene {
public:
    using ClickHandler =
        std::function<void(const QPointF&, Qt::MouseButton, int, int)>;
    std::function<void(const QPointF&)> moveHandler;

    explicit GridScene(QObject* parent = nullptr) : QGraphicsScene(parent)
    {
        setSceneRect(-32000.0, -32000.0, 64000.0, 64000.0);
    }

    ClickHandler clickHandler;

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override
    {
        painter->fillRect(rect, QColor(245, 247, 249));
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        int objectId = 0;
        int terminal = -1;
        int wireId = 0;
        for (QGraphicsItem* item : items(event->scenePos())) {
            bool validId = false;
            const int candidate = item->data(0).toInt(&validId);
            if (validId && candidate > 0) {
                if (item->data(2).toString() == "wire") {
                    if (wireId == 0) {
                        wireId = candidate;
                    }
                } else {
                    objectId = candidate;
                    bool validTerminal = false;
                    const int candidateTerminal = item->data(1).toInt(&validTerminal);
                    if (validTerminal && candidateTerminal >= 0) {
                        terminal = candidateTerminal;
                    }
                }
                if (objectId > 0) {
                    break;
                }
            }
        }
        if (objectId == 0 && wireId > 0) {
            objectId = -wireId;
        }
        if (clickHandler) {
            clickHandler(event->scenePos(), event->button(), objectId, terminal);
        }
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (moveHandler) {
            moveHandler(event->scenePos());
        }
        QGraphicsScene::mouseMoveEvent(event);
    }
};

class GridView : public QGraphicsView {
public:
    explicit GridView(QGraphicsScene* scene, QWidget* parent = nullptr)
        : QGraphicsView(scene, parent)
    {
        setRenderHint(QPainter::Antialiasing);
        setRenderHint(QPainter::SmoothPixmapTransform);
        setDragMode(QGraphicsView::NoDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorViewCenter);
        setBackgroundBrush(QColor(245, 247, 249));
        setMouseTracking(true);
        viewport()->setMouseTracking(true);
    }

    std::function<void()> leaveHandler;
    std::function<void()> rightClickHandler;
    std::function<void(const QPointF&)> coordinateHandler;
    std::function<void(double)> zoomHandler;

    void setCellLabel(QLabel* label)
    {
        cellLabel = label;
        cellLabel->setParent(viewport());
        cellLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        cellLabel->setStyleSheet(
            "QLabel { color: #263442; background: rgba(255, 255, 255, 220); "
            "border: 1px solid #8c9baa; padding: 2px 5px; }");
        cellLabel->adjustSize();
        positionCellLabel();
        cellLabel->show();
        cellLabel->raise();
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QGraphicsView::resizeEvent(event);
        positionCellLabel();
    }

    void leaveEvent(QEvent* event) override
    {
        if (leaveHandler) {
            leaveHandler();
        }
        QGraphicsView::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::MiddleButton ||
            event->button() == Qt::RightButton) {
            setFocus(Qt::MouseFocusReason);
            panning = true;
            panButton = event->button();
            rightClickDragged = false;
            lastPanPosition = event->position().toPoint();
            pressPosition = lastPanPosition;
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (coordinateHandler) {
            coordinateHandler(mapToScene(event->position().toPoint()));
        }
        if (panning && (event->buttons() & panButton)) {
            const QPoint currentPosition = event->position().toPoint();
            if (panButton == Qt::RightButton && !rightClickDragged) {
                if ((currentPosition - pressPosition).manhattanLength() < 4) {
                    lastPanPosition = currentPosition;
                    event->accept();
                    return;
                }
                rightClickDragged = true;
            }
            const QPoint delta = currentPosition - lastPanPosition;
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
            lastPanPosition = currentPosition;
            event->accept();
            return;
        }
        QGraphicsView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if ((event->button() == Qt::MiddleButton ||
             event->button() == Qt::RightButton) &&
            panning && event->button() == panButton) {
            if (event->button() == Qt::RightButton && !rightClickDragged &&
                rightClickHandler) {
                rightClickHandler();
            }
            panning = false;
            panButton = Qt::NoButton;
            unsetCursor();
            event->accept();
            return;
        }
        QGraphicsView::mouseReleaseEvent(event);
    }

    void wheelEvent(QWheelEvent* event) override
    {
        const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        if ((zoom * factor) < 0.025 || (zoom * factor) > 4.0) {
            event->accept();
            return;
        }
        zoom *= factor;
        scale(factor, factor);
        if (zoomHandler) {
            zoomHandler(zoom);
        }
        event->accept();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        const int step = 48;
        switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_A:
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - step);
            break;
        case Qt::Key_Right:
        case Qt::Key_D:
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + step);
            break;
        case Qt::Key_Up:
        case Qt::Key_W:
            verticalScrollBar()->setValue(verticalScrollBar()->value() - step);
            break;
        case Qt::Key_Down:
        case Qt::Key_S:
            verticalScrollBar()->setValue(verticalScrollBar()->value() + step);
            break;
        default:
            QGraphicsView::keyPressEvent(event);
            return;
        }
        event->accept();
    }

private:
    void positionCellLabel()
    {
        if (cellLabel) {
            cellLabel->adjustSize();
            cellLabel->move(viewport()->width() - cellLabel->width() - 10, 10);
            cellLabel->raise();
        }
    }

    double zoom = 1.0;
    bool panning = false;
    bool rightClickDragged = false;
    Qt::MouseButton panButton = Qt::NoButton;
    QPoint lastPanPosition;
    QPoint pressPosition;
    QLabel* cellLabel = nullptr;
};
}

MainSimulationWindow::MainSimulationWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Electrical Grid Simulator");
    resize(1280, 820);

    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    viewScene = new GridScene(this);
    auto* gridLines = new GridLinesItem;
    gridLines->setZValue(-1000.0);
    gridLines->setAcceptedMouseButtons(Qt::NoButton);
    viewScene->addItem(gridLines);
    view = new GridView(viewScene, centralWidget);
    layout->addWidget(view);
    setCentralWidget(centralWidget);

    auto* paletteDock = new QDockWidget("Grid components", this);
    palette = new QListWidget(paletteDock);
    const auto addTool = [this](const QString& label, const QString& tool) {
        auto* item = new QListWidgetItem(label, palette);
        item->setData(Qt::UserRole, tool);
    };
    addTool("None", "none");
    addTool("House", "house");
    addTool("Apartment", "apartment");
    addTool("Utility pole", "pole");
    addTool("Ground wire connection", "ground");
    addTool("Wire separator", "separator");
    addTool("Power generator", "generator");
    addTool("Connect wires", "wire");
    paletteDock->setWidget(palette);
    addDockWidget(Qt::LeftDockWidgetArea, paletteDock);

    auto* propertiesDock = new QDockWidget("Properties", this);
    auto* properties = new QWidget(propertiesDock);
    propertyForm = new QFormLayout(properties);
    infoLabel = new QLabel("Select a component to inspect it.");
    simulationLabel = new QLabel("Simulation running");
    simulationLabel->setWordWrap(true);
    propertyForm->addRow(infoLabel);
    propertyForm->addRow("State", simulationLabel);

    const auto makeSpinBox = [properties](double minimum, double maximum,
                                           double step, const QString& suffix) {
        auto* spin = new QDoubleSpinBox(properties);
        spin->setRange(minimum, maximum);
        spin->setSingleStep(step);
        spin->setDecimals(2);
        spin->setSuffix(suffix);
        spin->setKeyboardTracking(false);
        return spin;
    };
    generatorVoltage = makeSpinBox(1.0, 100000.0, 10.0, " V");
    generatorCurrent = makeSpinBox(0.1, 10000.0, 1.0, " A");
    houseDemand = makeSpinBox(0.1, 10000.0, 0.5, " A");
    houseVoltage = makeSpinBox(1.0, 100000.0, 10.0, " V");
    connectionHeight = makeSpinBox(0.0, 1000.0, 1.0, " m");
    separatorClosed = new QCheckBox("Conducting", properties);
    propertyForm->addRow("Generator voltage", generatorVoltage);
    propertyForm->addRow("Generator current limit", generatorCurrent);
    propertyForm->addRow("House required current", houseDemand);
    propertyForm->addRow("House rated voltage", houseVoltage);
    propertyForm->addRow("Connection height", connectionHeight);
    propertyForm->addRow("Wire separator", separatorClosed);
    wireReadout = new QLabel;
    wireReadout->setWordWrap(true);
    wireMaterial = new QComboBox(properties);
    wireMaterial->addItem("Copper", static_cast<int>(WireMaterial::Copper));
    wireMaterial->addItem("Aluminum", static_cast<int>(WireMaterial::Aluminum));
    wireMaterial->addItem("Steel", static_cast<int>(WireMaterial::Steel));
    wireResistivity = new QDoubleSpinBox(properties);
    wireResistivity->setRange(1.0e-10, 1.0e-4);
    wireResistivity->setDecimals(10);
    wireResistivity->setSingleStep(1.0e-9);
    wireResistivity->setSuffix(" ohm m");
    wireResistivity->setKeyboardTracking(false);
    sagGraph = new SagGraph(properties);
    propertyForm->addRow("Wire values", wireReadout);
    propertyForm->addRow("Wire material", wireMaterial);
    propertyForm->addRow("Resistivity", wireResistivity);
    propertyForm->addRow(sagGraph);
    propertiesDock->setWidget(properties);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock);

    auto* fileMenu = menuBar()->addMenu("&File");
    auto* saveAction = fileMenu->addAction("&Save...");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered,
            this, &MainSimulationWindow::saveSimulation);
    auto* loadAction = fileMenu->addAction("&Load...");
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered,
            this, &MainSimulationWindow::loadSimulation);
    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto* editMenu = menuBar()->addMenu("&Edit");
    auto* deleteAction = editMenu->addAction("Delete selected object");
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setShortcutContext(Qt::WindowShortcut);
    connect(deleteAction, &QAction::triggered, this,
            &MainSimulationWindow::deleteSelectedObject);
    auto* rotateAction = editMenu->addAction("Rotate clockwise");
    rotateAction->setShortcut(QKeySequence(Qt::Key_R));
    rotateAction->setShortcutContext(Qt::WindowShortcut);
    connect(rotateAction, &QAction::triggered, this,
            &MainSimulationWindow::rotateCurrent);

    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::information(
            this, "About Electrical Grid Simulator",
            "Place grid components, connect them with wires, and observe electrical "
            "and thermal behavior as the network runs.");
    });

    auto* toolbar = addToolBar("Simulation");
    auto* informationButton = new QPushButton("Display all information", toolbar);
    toolbar->addWidget(informationButton);
    connect(informationButton, &QPushButton::clicked,
            this, &MainSimulationWindow::displayAllInformation);
    simulationButton = new QPushButton("Run simulation", toolbar);
    simulationButton->setCheckable(true);
    toolbar->addWidget(simulationButton);
    auto* resetButton = new QPushButton("Reset simulation", toolbar);
    toolbar->addWidget(resetButton);
    connect(resetButton, &QPushButton::clicked,
            this, &MainSimulationWindow::resetSimulation);
    auto* speedMultiplier = new QDoubleSpinBox(toolbar);
    speedMultiplier->setRange(0.1, 1000.0);
    speedMultiplier->setDecimals(1);
    speedMultiplier->setSingleStep(1.0);
    speedMultiplier->setValue(10.0);
    speedMultiplier->setSuffix("x");
    speedMultiplier->setPrefix("Speed: ");
    speedMultiplier->setKeyboardTracking(false);
    toolbar->addWidget(speedMultiplier);
    connect(speedMultiplier, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double multiplier) {
                simulationSpeedMultiplier = multiplier;
            });
    connect(simulationButton, &QPushButton::clicked, this, [this] {
        setSimulationRunning(!simulationRunning);
    });

    statusBar()->showMessage(
        "Select None to inspect objects. Press R to rotate; Delete removes the selected object.", 8000);

    connect(palette, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem* item) {
                if (item) {
                    selectTool(item->data(Qt::UserRole).toString());
                }
            });
    connect(palette, &QListWidget::itemClicked,
            this, [this](QListWidgetItem* item) {
                if (item->data(Qt::UserRole).toString() == activeTool &&
                    selectedObjectId != -1) {
                    selectTool(activeTool);
                }
            });
    palette->setCurrentRow(0);
    auto* scene = static_cast<GridScene*>(viewScene);
    auto* gridView = static_cast<GridView*>(view);
    gridView->zoomHandler = [gridLines](double zoom) {
        const bool showGrid = zoom >= 0.12;
        if (gridLines->isVisible() != showGrid) {
            gridLines->setVisible(showGrid);
        }
    };
    gridView->rightClickHandler = [this] {
        selectTool("none");
        palette->setCurrentRow(0);
    };
    gridView->leaveHandler = [this] {
        pointerOverGrid = false;
        if (mouseCellLabel) {
            mouseCellLabel->setText("Cell (row, col): --");
            mouseCellLabel->adjustSize();
        }
        if (ghostItem) {
            ghostItem->hide();
        }
        if (ghostFrame) {
            ghostFrame->hide();
        }
        if (ghostWireItem) {
            ghostWireItem->hide();
        }
        if (ghostWireItem) {
            ghostWireItem->hide();
        }
    };
    mouseCellLabel = new QLabel(view->viewport());
    mouseCellLabel->setText("Cell (row, col): (0, 0)");
    QFont coordinateFont = mouseCellLabel->font();
    coordinateFont.setPointSize(8);
    mouseCellLabel->setFont(coordinateFont);
    gridView->setCellLabel(mouseCellLabel);
    gridView->coordinateHandler = [this](const QPointF& position) {
        if (!mouseCellLabel) {
            return;
        }
        const int column = static_cast<int>(std::floor(position.x() / gridSize));
        const int row = static_cast<int>(std::floor(position.y() / gridSize));
        mouseCellLabel->setText(
            QString("Cell (row, col): (%1, %2)").arg(row).arg(column));
        mouseCellLabel->adjustSize();
        mouseCellLabel->move(
            view->viewport()->width() - mouseCellLabel->width() - 10, 10);
        mouseCellLabel->raise();
        updateWireGhost(position);
    };
    scene->moveHandler = [this](const QPointF& position) {
        pointerOverGrid = true;
        updateGhost(position);
        updateWireGhost(position);
    };

    connect(generatorVoltage, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                for (const auto& object : objects) {
                    if (object->id() == selectedObjectId) {
                        if (auto* generator = dynamic_cast<PowerGenerator*>(object.get())) {
                            generator->setOutputVoltage(value);
                            runSimulation();
                        }
                        break;
                    }
                }
            });
    connect(generatorCurrent, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                for (const auto& object : objects) {
                    if (object->id() == selectedObjectId) {
                        if (auto* generator = dynamic_cast<PowerGenerator*>(object.get())) {
                            generator->setCurrentLimit(value);
                            runSimulation();
                        }
                        break;
                    }
                }
            });
    connect(houseDemand, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                for (const auto& object : objects) {
                    if (object->id() == selectedObjectId) {
                        if (auto* house = dynamic_cast<House*>(object.get())) {
                            house->setRequiredCurrent(value);
                            runSimulation();
                        }
                        break;
                    }
                }
            });
    connect(houseVoltage, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                for (const auto& object : objects) {
                    if (object->id() == selectedObjectId) {
                        if (auto* house = dynamic_cast<House*>(object.get())) {
                            house->setRatedVoltage(value);
                            runSimulation();
                        }
                        break;
                    }
                }
            });
    connect(connectionHeight, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
                for (const auto& object : objects) {
                    if (object->id() == selectedObjectId) {
                        object->setConnectionHeightMeters(value);
                        runSimulation();
                        break;
                    }
                }
            });
    connect(separatorClosed, &QCheckBox::toggled, this, [this](bool closed) {
        for (const auto& object : objects) {
            if (object->id() == selectedObjectId) {
                if (auto* separator = dynamic_cast<WireSeparator*>(object.get())) {
                    separator->setClosed(closed);
                    runSimulation();
                }
                break;
            }
        }
    });
    connect(wireMaterial, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                for (const auto& wire : connections.wires()) {
                    if (wire->id() != selectedWireId) {
                        continue;
                    }
                    wire->setMaterial(
                        static_cast<WireMaterial>(wireMaterial->itemData(index).toInt()));
                    const QSignalBlocker blockResistivity(wireResistivity);
                    wireResistivity->setValue(wire->get_resistivity());
                    runSimulation();
                    break;
                }
            });
    connect(wireResistivity, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double resistivity) {
                for (const auto& wire : connections.wires()) {
                    if (wire->id() == selectedWireId) {
                        wire->setResistivity(resistivity);
                        runSimulation();
                        break;
                    }
                }
            });

    scene->clickHandler = [this](const QPointF& position, Qt::MouseButton button,
                                 int id, int terminal) {
        pointerOverGrid = true;
        handleSceneClick(position, button, id, terminal);
        if (button != Qt::LeftButton || activeTool != "wire" || id <= 0) {
            return;
        }
        const ParentOfObjects* clickedObject = nullptr;
        for (const auto& object : objects) {
            if (object->id() == id) {
                clickedObject = object.get();
                break;
            }
        }
        if (!clickedObject) {
            statusBar()->showMessage("Could not find a selected component.");
            return;
        }
        if (terminal < 0) {
            double nearestDistance = std::numeric_limits<double>::max();
            for (int port = 0; port < clickedObject->terminalCount(); ++port) {
                const QPointF portPosition =
                    terminalScenePosition(*clickedObject, port, gridSize);
                const double dx = position.x() - portPosition.x();
                const double dy = position.y() - portPosition.y();
                const double distance = dx * dx + dy * dy;
                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    terminal = port;
                }
            }
        }
        if (clickedObject->kind() == ObjectKind::UtilityPole) {
            int leastUsedConnections = std::numeric_limits<int>::max();
            for (int port = 0; port < clickedObject->terminalCount(); ++port) {
                const int portConnections =
                    connections.terminalConnectionCount(id, port);
                if (portConnections < leastUsedConnections) {
                    leastUsedConnections = portConnections;
                    terminal = port;
                }
            }
        }
        if (pendingWireSourceId == -1) {
            pendingWireSourceId = id;
            pendingWireSourceTerminal = terminal;
            selectObject(id);
            statusBar()->showMessage("Select a second terminal to connect the wire.");
            updateWireGhost(position);
            return;
        }

        const int firstId = pendingWireSourceId;
        const int firstTerminal = pendingWireSourceTerminal;
        pendingWireSourceId = -1;
        pendingWireSourceTerminal = -1;
        if (ghostWireItem) {
            ghostWireItem->hide();
        }
        const ParentOfObjects* first = nullptr;
        for (const auto& object : objects) {
            if (object->id() == firstId) {
                first = object.get();
            }
        }
        if (!first) {
            statusBar()->showMessage("Could not find a selected component.");
            return;
        }
        const QPointF firstPosition =
            terminalScenePosition(*first, firstTerminal, gridSize);
        const QPointF secondPosition =
            terminalScenePosition(*clickedObject, terminal, gridSize);
        const double dx = firstPosition.x() - secondPosition.x();
        const double dy = firstPosition.y() - secondPosition.y();
        const double lengthMeters = std::hypot(dx, dy) * 10.0 / gridSize;
        const bool shortCircuit =
            firstId == id && firstTerminal != terminal &&
            dynamic_cast<const PowerGenerator*>(first) != nullptr &&
            (firstTerminal < 4) != (terminal < 4);
        QString error;
        if (!connections.addWire(nextWireId, *first, firstTerminal,
                                *clickedObject, terminal, lengthMeters,
                                shortCircuit, &error)) {
            statusBar()->showMessage(error, 5000);
            return;
        }
        ++nextWireId;
        refreshWireGraphics();
        if (ghostWireItem) {
            ghostWireItem->hide();
        }
        runSimulation();
        statusBar()->showMessage(
            shortCircuit ? "Generator terminals shorted (wire shown in red)." :
            connections.wires().back()->isGroundWire()
                ? "Ground wire connected to the ground reference."
                : "Wire connected.",
            5000);
    };

    connect(&simulationTimer, &QTimer::timeout, this, &MainSimulationWindow::runSimulation);
    simulationTimer.setInterval(250);
    ghostItem = viewScene->addPixmap(QPixmap());
    ghostItem->setOffset(0.0, 0.0);
    ghostItem->setZValue(5.0);
    ghostItem->setOpacity(0.45);
    ghostItem->setAcceptedMouseButtons(Qt::NoButton);
    ghostFrame = viewScene->addRect(QRectF());
    ghostFrame->setBrush(QBrush(Qt::transparent));
    ghostFrame->setZValue(6.0);
    ghostFrame->setAcceptedMouseButtons(Qt::NoButton);
    ghostItem->hide();
    ghostFrame->hide();
    ghostWireItem = new QGraphicsPathItem;
    ghostWireItem->setPen(QPen(QColor(30, 125, 205, 190), 3.0,
                               Qt::DashLine, Qt::RoundCap));
    ghostWireItem->setZValue(30.0);
    ghostWireItem->setAcceptedMouseButtons(Qt::NoButton);
    viewScene->addItem(ghostWireItem);
    ghostWireItem->hide();
    refreshInspector();
    elapsedTimer.start();
    runSimulation();
    view->setFocus();
}

void MainSimulationWindow::selectTool(const QString& tool)
{
    activeTool = tool;
    selectedWireId = -1;
    pendingWireSourceId = -1;
    pendingWireSourceTerminal = -1;
    ghostObject.reset();
    if (tool != "none" && tool != "wire" && selectedObjectId != -1) {
        selectedObjectId = -1;
        refreshInspector();
        refreshObjectGraphics();
    }
    if (wireReadout) {
        refreshInspector();
    }
    if (!wireItems.empty()) {
        refreshWireGraphics();
    }
    if (ghostItem) {
        ghostItem->hide();
    }
    if (ghostFrame) {
        ghostFrame->hide();
    }
    if (tool == "wire" || tool == "none") {
        if (tool == "none") {
            statusBar()->showMessage("No placement tool selected.");
        } else {
            statusBar()->showMessage(
                "Connect terminal ports on components. Bridging both generator ports creates a short.");
        }
        return;
    }
    ghostObject = createObject(tool, -1, lastGhostGridX, lastGhostGridY);
    if (!ghostObject || ghostObject->image().isNull()) {
        ghostObject.reset();
        statusBar()->showMessage("Could not load the selected component icon.", 7000);
        return;
    }
    const int baseWidth = ghostObject->rotation() % 2 == 0
        ? ghostObject->width() : ghostObject->height();
    const int baseHeight = ghostObject->rotation() % 2 == 0
        ? ghostObject->height() : ghostObject->width();
    QPixmap pixmap = QPixmap::fromImage(ghostObject->image()).scaled(
        baseWidth * static_cast<int>(gridSize),
        baseHeight * static_cast<int>(gridSize),
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (ghostObject->rotation() != 0) {
        pixmap = pixmap.transformed(QTransform().rotate(ghostObject->rotation() * 90),
                                    Qt::SmoothTransformation);
    }
    ghostItem->setPixmap(pixmap);
    ghostItem->setOffset(-ghostObject->width() * gridSize / 2.0,
                         -ghostObject->height() * gridSize / 2.0);
    ghostItem->setRotation(0.0);
    ghostItem->show();
    ghostFrame->show();
    updateGhost(QPointF((lastGhostGridX + 0.5) * gridSize,
                        (lastGhostGridY + 0.5) * gridSize));
    const QString toolName = tool == "house" ? "house" :
        tool == "apartment" ? "apartment" :
        tool == "pole" ? "utility pole" :
    tool == "ground" ? "ground wire connection" :
    tool == "separator" ? "wire separator" : "power generator";
    statusBar()->showMessage("Click an empty grid area to place a " + toolName + ".");
}

std::unique_ptr<ParentOfObjects> MainSimulationWindow::createObject(
    const QString& tool, int id, int gridX, int gridY) const
{
    if (tool == "house") {
        return std::make_unique<House>(id, gridX, gridY);
    }
    if (tool == "apartment") {
        return std::make_unique<Apartment>(id, gridX, gridY);
    }
    if (tool == "pole") {
        return std::make_unique<UtilityPole>(id, gridX, gridY);
    }
    if (tool == "ground") {
        return std::make_unique<GroundWireConnection>(id, gridX, gridY);
    }
    if (tool == "separator") {
        return std::make_unique<WireSeparator>(id, gridX, gridY);
    }
    if (tool == "generator") {
        return std::make_unique<PowerGenerator>(id, gridX, gridY);
    }
    return nullptr;
}

void MainSimulationWindow::updateGhost(const QPointF& position)
{
    if (!ghostObject || activeTool == "none" || activeTool == "wire" ||
        !pointerOverGrid) {
        if (ghostItem) {
            ghostItem->hide();
        }
        if (ghostFrame) {
            ghostFrame->hide();
        }
        return;
    }
    lastGhostGridX = static_cast<int>(std::floor(position.x() / gridSize));
    lastGhostGridY = static_cast<int>(std::floor(position.y() / gridSize));
    ghostObject->setPosition(lastGhostGridX, lastGhostGridY);
    const QPointF center(
        (lastGhostGridX + ghostObject->width() / 2.0) * gridSize,
        (lastGhostGridY + ghostObject->height() / 2.0) * gridSize);
    ghostItem->setPos(center);
    ghostFrame->setPos(center);
    ghostFrame->setRect(-ghostObject->width() * gridSize / 2.0 - 2.0,
                        -ghostObject->height() * gridSize / 2.0 - 2.0,
                        ghostObject->width() * gridSize + 4.0,
                        ghostObject->height() * gridSize + 4.0);
    ghostFrame->setPen(QPen(canPlace(*ghostObject) ? QColor(30, 150, 75) :
                                                     QColor(220, 35, 35), 3.0));
    ghostItem->show();
    ghostFrame->show();
}

void MainSimulationWindow::rotateCurrent()
{
    for (const auto& object : objects) {
        if (object->id() != selectedObjectId) {
            continue;
        }
        object->rotateClockwise();
        if (!canPlace(*object, object->id())) {
            for (int turn = 0; turn < 3; ++turn) {
                object->rotateClockwise();
            }
            statusBar()->showMessage(
                "Cannot rotate this component because it would overlap another object.", 5000);
            return;
        }
        refreshObjectGraphics();
        refreshWireGraphics();
        runSimulation();
        return;
    }

    if (ghostObject && activeTool != "none" && activeTool != "wire") {
        ghostObject->rotateClockwise();
        const int baseWidth = ghostObject->rotation() % 2 == 0
            ? ghostObject->width() : ghostObject->height();
        const int baseHeight = ghostObject->rotation() % 2 == 0
            ? ghostObject->height() : ghostObject->width();
        QPixmap pixmap = QPixmap::fromImage(ghostObject->image()).scaled(
            baseWidth * static_cast<int>(gridSize),
            baseHeight * static_cast<int>(gridSize),
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        pixmap = pixmap.transformed(
            QTransform().rotate(ghostObject->rotation() * 90),
            Qt::SmoothTransformation);
        ghostItem->setPixmap(pixmap);
        ghostItem->setOffset(-ghostObject->width() * gridSize / 2.0,
                             -ghostObject->height() * gridSize / 2.0);
        updateGhost(QPointF((lastGhostGridX + 0.5) * gridSize,
                            (lastGhostGridY + 0.5) * gridSize));
        return;
    }
}

void MainSimulationWindow::deleteSelectedObject()
{
    if (selectedWireId != -1) {
        const auto& wires = connections.wires();
        const auto found = std::find_if(
            wires.begin(), wires.end(),
            [this](const std::unique_ptr<Wire>& wire) {
                return wire->id() == selectedWireId;
            });
        if (found != wires.end()) {
            const std::size_t wireIndex =
                static_cast<std::size_t>(std::distance(wires.begin(), found));
            delete wireItems[wireIndex];
            wireItems.erase(wireItems.begin() +
                            static_cast<std::ptrdiff_t>(wireIndex));
            connections.removeWire(selectedWireId);
            selectedWireId = -1;
            refreshInspector();
            runSimulation();
            statusBar()->showMessage("Wire deleted.", 3000);
            return;
        }
    }

    for (std::size_t index = 0; index < objects.size(); ++index) {
        if (objects[index]->id() != selectedObjectId) {
            continue;
        }
        const int removedId = selectedObjectId;
        connections.removeObject(removedId);
        delete objectItems[index];
        delete objectFrames[index];
        for (QGraphicsEllipseItem* terminal : objectTerminals[index]) {
            delete terminal;
        }
        objectItems.erase(objectItems.begin() + static_cast<std::ptrdiff_t>(index));
        objectFrames.erase(objectFrames.begin() + static_cast<std::ptrdiff_t>(index));
        objectImageRotations.erase(
            objectImageRotations.begin() + static_cast<std::ptrdiff_t>(index));
        objectUnderpowered.erase(
            objectUnderpowered.begin() + static_cast<std::ptrdiff_t>(index));
        objectTerminals.erase(
            objectTerminals.begin() + static_cast<std::ptrdiff_t>(index));
        objects.erase(objects.begin() + static_cast<std::ptrdiff_t>(index));
        selectedObjectId = -1;
        pendingWireSourceId = -1;
        pendingWireSourceTerminal = -1;
        for (QGraphicsPathItem* wireItem : wireItems) {
            delete wireItem;
        }
        wireItems.clear();
        refreshWireGraphics();
        refreshObjectGraphics();
        refreshInspector();
        runSimulation();
        statusBar()->showMessage("Component and its connected wires deleted.", 4000);
        return;
    }
    statusBar()->showMessage("Select a component or wire before deleting it.", 3000);
}

void MainSimulationWindow::handleSceneClick(const QPointF& position,
                                            Qt::MouseButton button,
                                            int objectId, int)
{
    view->setFocus();
    if (button == Qt::RightButton) {
        pendingWireSourceId = -1;
        pendingWireSourceTerminal = -1;
        statusBar()->showMessage("Wire connection cancelled.");
        return;
    }

    if (button != Qt::LeftButton) {
        return;
    }
    if (activeTool == "wire") {
        return;
    }
    if (objectId < 0) {
        selectWire(-objectId);
        return;
    }
    if (objectId > 0) {
        selectObject(objectId);
        return;
    }
    if (!activeTool.isEmpty() && activeTool != "none") {
        placeObject(position);
    }
}

void MainSimulationWindow::placeObject(const QPointF& position)
{
    const int gridX = static_cast<int>(std::floor(position.x() / gridSize));
    const int gridY = static_cast<int>(std::floor(position.y() / gridSize));
    std::unique_ptr<ParentOfObjects> object =
        createObject(activeTool, nextObjectId, gridX, gridY);
    if (!object) {
        return;
    }
    if (ghostObject) {
        for (int turn = 0; turn < ghostObject->rotation(); ++turn) {
            object->rotateClockwise();
        }
    }

    if (object->image().isNull()) {
        statusBar()->showMessage(
            "Could not load the icon file: " + object->name() + ". Check the icons folder.",
            7000);
        return;
    }
    if (!canPlace(*object)) {
        statusBar()->showMessage("Components cannot overlap; choose an empty area.", 4000);
        return;
    }

    ++nextObjectId;
    objects.push_back(std::move(object));
    refreshObjectGraphics();
    selectObject(nextObjectId - 1);
    updateGhost(position);
    runSimulation();
}

bool MainSimulationWindow::canPlace(const ParentOfObjects& candidate,
                                    int ignoredObjectId) const
{
    const double candidateLeft = candidate.x();
    const double candidateTop = candidate.y();
    const double candidateRight = candidateLeft + candidate.width();
    const double candidateBottom = candidateTop + candidate.height();
    for (const auto& object : objects) {
        if (object->id() == ignoredObjectId) {
            continue;
        }
        const double left = object->x();
        const double top = object->y();
        const double right = left + object->width();
        const double bottom = top + object->height();
        if (candidateLeft < right && candidateRight > left &&
            candidateTop < bottom && candidateBottom > top) {
            return false;
        }
    }
    return true;
}

void MainSimulationWindow::selectObject(int id)
{
    selectedWireId = -1;
    selectedObjectId = id;
    refreshInspector();
    refreshObjectGraphics();
    refreshWireGraphics();
}

void MainSimulationWindow::selectWire(int id)
{
    selectedWireId = id;
    selectedObjectId = -1;
    refreshObjectGraphics();
    refreshInspector();
    refreshWireGraphics();
}

void MainSimulationWindow::refreshObjectGraphics()
{
    while (objectItems.size() > objects.size()) {
        delete objectItems.back();
        objectItems.pop_back();
        delete objectFrames.back();
        objectFrames.pop_back();
        objectImageRotations.pop_back();
        objectUnderpowered.pop_back();
        for (QGraphicsEllipseItem* terminal : objectTerminals.back()) {
            delete terminal;
        }
        objectTerminals.pop_back();
    }
    while (objectItems.size() < objects.size()) {
        objectItems.push_back(viewScene->addPixmap(QPixmap()));
        objectFrames.push_back(viewScene->addRect(QRectF()));
        objectImageRotations.push_back(-1);
        objectUnderpowered.push_back(false);
        std::vector<QGraphicsEllipseItem*> terminals;
        const ParentOfObjects& object = *objects[objectItems.size() - 1];
        for (int terminal = 0; terminal < object.terminalCount(); ++terminal) {
            auto* marker = viewScene->addEllipse(QRectF(-6.0, -6.0, 12.0, 12.0));
            marker->setZValue(25.0);
            terminals.push_back(marker);
        }
        objectTerminals.push_back(std::move(terminals));
    }

    for (std::size_t index = 0; index < objects.size(); ++index) {
        const ParentOfObjects& object = *objects[index];
        const double width = object.width() * gridSize;
        const double height = object.height() * gridSize;
        const QPointF center((object.x() + object.width() / 2.0) * gridSize,
                             (object.y() + object.height() / 2.0) * gridSize);
        QGraphicsPixmapItem* item = objectItems[index];
        if (objectImageRotations[index] != object.rotation() ||
            item->pixmap().isNull()) {
            const int baseWidth = object.rotation() % 2 == 0
                ? object.width() : object.height();
            const int baseHeight = object.rotation() % 2 == 0
                ? object.height() : object.width();
            QPixmap pixmap = QPixmap::fromImage(object.image()).scaled(
                baseWidth * static_cast<int>(gridSize),
                baseHeight * static_cast<int>(gridSize),
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (object.rotation() != 0) {
                pixmap = pixmap.transformed(
                    QTransform().rotate(object.rotation() * 90),
                    Qt::SmoothTransformation);
            }
            item->setPixmap(pixmap);
            item->setOffset(-width / 2.0, -height / 2.0);
            objectImageRotations[index] = object.rotation();
        }
        item->setPos(center);
        item->setData(0, object.id());
        item->setData(1, -1);
        item->setZValue(2.0);
        item->setToolTip(object.name());

        QGraphicsRectItem* frame = objectFrames[index];
        frame->setRect(-width / 2.0 - 2.0, -height / 2.0 - 2.0,
                       width + 4.0, height + 4.0);
        frame->setPos(center);
        frame->setBrush(QBrush(Qt::transparent));
        frame->setData(0, object.id());
        frame->setData(1, -1);
        frame->setZValue(1.0);
        for (int terminal = 0; terminal < object.terminalCount(); ++terminal) {
            QGraphicsEllipseItem* marker =
                objectTerminals[index][static_cast<std::size_t>(terminal)];
            marker->setPos(terminalScenePosition(object, terminal, gridSize));
            marker->setData(0, object.id());
            marker->setData(1, terminal);
            QColor terminalColor;
            QString terminalName;
            if (object.terminalCount() == 1) {
                if (object.kind() == ObjectKind::GroundWireConnection) {
                    terminalColor = QColor(109, 76, 161);
                    terminalName = "Ground wire terminal";
                } else {
                    terminalColor = QColor(235, 145, 35);
                    terminalName = "Junction terminal";
                }
            } else if (object.kind() == ObjectKind::WireSeparator) {
                terminalColor = terminal == 0 ? QColor(35, 155, 75) :
                                                QColor(45, 105, 220);
                terminalName = terminal == 0 ? "Switch input" :
                    QString("Switch output %1").arg(terminal);
            } else if (object.kind() == ObjectKind::PowerGenerator) {
                terminalColor = terminal < 4 ? QColor(35, 155, 75) :
                                               QColor(45, 105, 220);
                terminalName = terminal < 4
                    ? QString("Generator output %1").arg(terminal + 1)
                    : QString("Generator input %1").arg(terminal - 3);
            } else {
                terminalColor = terminal == 0 ? QColor(35, 155, 75) :
                                                QColor(45, 105, 220);
                terminalName = terminal == 0 ? "Positive terminal" :
                                               "Return terminal";
            }
            marker->setBrush(terminalColor);
            marker->setPen(QPen(Qt::white, 1.5));
            marker->setToolTip(terminalName);
        }
        const auto* house = dynamic_cast<const House*>(&object);
        const bool underpowered = house && !house->isPowered();
        if (underpowered != objectUnderpowered[index]) {
            if (underpowered) {
                auto* tint = new QGraphicsColorizeEffect;
                tint->setColor(QColor(255, 193, 7));
                tint->setStrength(0.7);
                item->setGraphicsEffect(tint);
            } else {
                item->setGraphicsEffect(nullptr);
            }
            objectUnderpowered[index] = underpowered;
        }
        if (underpowered) {
            frame->setPen(QPen(QColor(255, 193, 7), 4.0));
        } else if (object.id() == selectedObjectId) {
            frame->setPen(QPen(QColor(30, 120, 210), 3.0));
        } else {
            frame->setPen(Qt::NoPen);
        }
    }
}

void MainSimulationWindow::refreshWireGraphics()
{
    const auto& wires = connections.wires();
    while (wireItems.size() > wires.size()) {
        delete wireItems.back();
        wireItems.pop_back();
    }
    while (wireItems.size() < wires.size()) {
        auto* item = new QGraphicsPathItem;
        item->setZValue(10.0);
        item->setAcceptedMouseButtons(Qt::NoButton);
        viewScene->addItem(item);
        wireItems.push_back(item);
    }

    const auto findObject = [this](int id) -> const ParentOfObjects* {
        for (const auto& object : objects) {
            if (object->id() == id) {
                return object.get();
            }
        }
        return nullptr;
    };
    for (std::size_t index = 0; index < wires.size(); ++index) {
        const Wire& wire = *wires[index];
        const ParentOfObjects* from = findObject(wire.fromObject());
        const ParentOfObjects* to = findObject(wire.toObject());
        if (!from || !to) {
            continue;
        }

        const QPointF start =
            terminalScenePosition(*from, wire.fromTerminal(), gridSize);
        const QPointF end =
            terminalScenePosition(*to, wire.toTerminal(), gridSize);
        QPainterPath path(start);
        if (wire.isGroundWire()) {
            path.lineTo(end);
        } else {
            const QPointF delta = end - start;
            const double sag = std::max(5.0, std::hypot(delta.x(), delta.y()) *
                                                wire.sagRatio());
            path.cubicTo(start + delta / 3.0 + QPointF(0.0, sag),
                         start + delta * (2.0 / 3.0) + QPointF(0.0, sag),
                         end);
        }
        QGraphicsPathItem* item = wireItems[index];
        item->setPath(path);
        item->setData(0, wire.id());
        item->setData(1, -1);
        item->setData(2, "wire");
        item->setZValue(wire.isGroundWire() ? 26.0 : 20.0);
        const QColor wireColor = wire.isRed() ? QColor(220, 35, 35) :
            wire.isGroundWire() ? QColor(109, 76, 161) :
            wire.id() == selectedWireId ? QColor(25, 125, 220) :
                                          QColor(75, 91, 105);
        item->setPen(QPen(wireColor, wire.id() == selectedWireId ? 6.0 : 4.0,
                          Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        item->setToolTip(QString("Current: %1 A\nVoltage drop: %2 V\n"
                                 "Resistance: %3 ohm\nTemperature: %4 C\n"
                                 "Temperature change: %5 C\nLength: %6 m\n"
                                 "Sag: %7 m\nMinimum height: %8 m")
                             .arg(std::abs(wire.current()), 0, 'f', 2)
                             .arg(std::abs(wire.voltageDrop()), 0, 'f', 2)
                             .arg(wire.resistance(), 0, 'f', 4)
                             .arg(wire.temperature(), 0, 'f', 1)
                             .arg(wire.temperatureChange(), 0, 'f', 3)
                             .arg(wire.expandedLength(), 0, 'f', 2)
                             .arg(wire.sagMeters(), 0, 'f', 2)
                             .arg(wire.minimumHeightMeters(), 0, 'f', 2));
    }
}

void MainSimulationWindow::refreshInspector()
{
    const QSignalBlocker blockGeneratorVoltage(generatorVoltage);
    const QSignalBlocker blockGeneratorCurrent(generatorCurrent);
    const QSignalBlocker blockHouseDemand(houseDemand);
    const QSignalBlocker blockHouseVoltage(houseVoltage);
    const QSignalBlocker blockConnectionHeight(connectionHeight);
    const QSignalBlocker blockSeparatorClosed(separatorClosed);
    const QSignalBlocker blockWireMaterial(wireMaterial);
    const QSignalBlocker blockWireResistivity(wireResistivity);
    const auto setPropertyVisible = [this](QWidget* field, bool visible) {
        if (QWidget* label = propertyForm->labelForField(field)) {
            label->setVisible(visible);
        }
        field->setVisible(visible);
    };
    setPropertyVisible(generatorVoltage, false);
    setPropertyVisible(generatorCurrent, false);
    setPropertyVisible(houseDemand, false);
    setPropertyVisible(houseVoltage, false);
    setPropertyVisible(connectionHeight, false);
    setPropertyVisible(separatorClosed, false);
    setPropertyVisible(wireReadout, false);
    setPropertyVisible(wireMaterial, false);
    setPropertyVisible(wireResistivity, false);
    setPropertyVisible(sagGraph, false);
    setPropertyVisible(simulationLabel, false);
    const ParentOfObjects* selected = nullptr;
    for (const auto& object : objects) {
        if (object->id() == selectedObjectId) {
            selected = object.get();
            break;
        }
    }
    if (!selected) {
        if (selectedWireId == -1) {
            infoLabel->setText("Select a component or wire to inspect it.");
            return;
        }
    }

    const Wire* selectedWire = nullptr;
    for (const auto& wire : connections.wires()) {
        if (wire->id() == selectedWireId) {
            selectedWire = wire.get();
            break;
        }
    }
    if (selectedWire) {
        infoLabel->setText("Wire");
        wireReadout->setText(
            QString("Voltage drop: %1 V\nCurrent: %2 A\nResistance: %3 ohm\n"
                    "Temperature: %4 C\nChange in temperature: %5 C\n"
                    "Sag: %6 m\nMinimum height above ground: %7 m%8\n"
                    "Length: %9 m (base %10 m)\nMass: %11 kg\n"
                    "Thermal expansion: %12 um/(m C)")
                .arg(std::abs(selectedWire->voltageDrop()), 0, 'f', 3)
                .arg(std::abs(selectedWire->current()), 0, 'f', 3)
                .arg(selectedWire->resistance(), 0, 'g', 5)
                .arg(selectedWire->temperature(), 0, 'f', 1)
                .arg(selectedWire->temperatureChange(), 0, 'f', 3)
                .arg(selectedWire->sagMeters(), 0, 'f', 3)
                .arg(selectedWire->minimumHeightMeters(), 0, 'f', 2)
                .arg(selectedWire->isLowClearance()
                         ? QStringLiteral(" (below 5 m; wire is red)")
                         : QString())
                .arg(selectedWire->expandedLength(), 0, 'f', 2)
                .arg(selectedWire->length(), 0, 'f', 2)
                .arg(selectedWire->mass(), 0, 'f', 3)
                .arg(selectedWire->expansionCoefficient() * 1.0e6, 0, 'f', 2));
        const int materialIndex = wireMaterial->findData(
            static_cast<int>(selectedWire->material()));
        if (!wireMaterial->hasFocus()) {
            wireMaterial->setCurrentIndex(materialIndex);
        }
        if (!wireResistivity->hasFocus()) {
            wireResistivity->setValue(selectedWire->get_resistivity());
        }
        setPropertyVisible(wireReadout, true);
        setPropertyVisible(wireMaterial, true);
        setPropertyVisible(wireResistivity, true);
        setPropertyVisible(sagGraph, true);
        static_cast<SagGraph*>(sagGraph)->setWire(selectedWire);
        return;
    }
    static_cast<SagGraph*>(sagGraph)->setWire(nullptr);

    if (!selected) {
        return;
    }
    const bool hasEditableConnectionHeight =
        selected->kind() != ObjectKind::GroundWireConnection;
    setPropertyVisible(connectionHeight, hasEditableConnectionHeight);
    if (const auto* generator = dynamic_cast<const PowerGenerator*>(selected)) {
        infoLabel->setText(QString("%1\nUID: %2\nOutput: %3 V, %4 A")
                               .arg(generator->name())
                               .arg(generator->uid())
                               .arg(selected->voltage(), 0, 'f', 1)
                               .arg(selected->current(), 0, 'f', 2));
        setPropertyVisible(generatorVoltage, true);
        setPropertyVisible(generatorCurrent, true);
        if (!generatorVoltage->hasFocus()) {
            generatorVoltage->setValue(generator->outputVoltage());
        }
        if (!generatorCurrent->hasFocus()) {
            generatorCurrent->setValue(generator->currentLimit());
        }
    } else if (const auto* house = dynamic_cast<const House*>(selected)) {
        const bool powered = house->isPowered();
        infoLabel->setText(QString("%1\nUID: %2\nNode voltage: %3 V\nLoad current: %4 / %5 A")
                               .arg(house->name())
                               .arg(house->uid())
                               .arg(selected->voltage(), 0, 'f', 1)
                               .arg(selected->current(), 0, 'f', 2)
                               .arg(house->requiredCurrent(), 0, 'f', 2));
        simulationLabel->setText(powered ? "Electrical demand met" : "Underpowered");
        setPropertyVisible(houseDemand, true);
        setPropertyVisible(houseVoltage, true);
        setPropertyVisible(simulationLabel, true);
        if (!houseDemand->hasFocus()) {
            houseDemand->setValue(house->requiredCurrent());
        }
        if (!houseVoltage->hasFocus()) {
            houseVoltage->setValue(house->ratedVoltage());
        }
    } else if (const auto* separator =
                   dynamic_cast<const WireSeparator*>(selected)) {
        infoLabel->setText(QString("%1\nUID: %2\nSwitch current: %3 A")
                               .arg(separator->name())
                               .arg(separator->uid())
                               .arg(selected->current(), 0, 'f', 2));
        simulationLabel->setText(separator->isClosed() ? "Separator closed" :
                                                          "Separator open");
        setPropertyVisible(separatorClosed, true);
        setPropertyVisible(simulationLabel, true);
        if (!separatorClosed->hasFocus()) {
            separatorClosed->setChecked(separator->isClosed());
        }
    } else {
        infoLabel->setText(QString("%1\nUID: %2\nNode voltage: %3 V")
                               .arg(selected->name())
                               .arg(selected->uid())
                               .arg(selected->voltage(), 0, 'f', 1));
    }
    if (hasEditableConnectionHeight && !connectionHeight->hasFocus()) {
        connectionHeight->setValue(selected->connectionHeightMeters());
    }
}

void MainSimulationWindow::runSimulation()
{
    for (const auto& wire : connections.wires()) {
        const ParentOfObjects* fromObject = nullptr;
        const ParentOfObjects* toObject = nullptr;
        for (const auto& object : objects) {
            if (object->id() == wire->fromObject()) {
                fromObject = object.get();
            } else if (object->id() == wire->toObject()) {
                toObject = object.get();
            }
        }
        if (fromObject && toObject) {
            wire->setEndpointHeights(fromObject->connectionHeightMeters(),
                                     toObject->connectionHeightMeters());
        }
    }

    const double elapsedSeconds = elapsedTimer.isValid()
        ? static_cast<double>(elapsedTimer.restart()) / 1000.0 : 0.0;
    QString error;
    if (!connections.simulate(objects,
                              elapsedSeconds * simulationSpeedMultiplier, &error,
                              simulationRunning)) {
        if (simulationRunning) {
            simulationRunning = false;
            simulationTimer.stop();
            const QSignalBlocker blocker(simulationButton);
            simulationButton->setChecked(false);
            simulationButton->setText("Run simulation");
        }
        statusBar()->showMessage("Simulation error: " + error, 7000);
        return;
    }

    refreshWireGraphics();
    refreshObjectGraphics();
    refreshInspector();
}

void MainSimulationWindow::setSimulationRunning(bool running)
{
    simulationRunning = running;
    {
        const QSignalBlocker blocker(simulationButton);
        simulationButton->setChecked(running);
    }
    simulationButton->setText(running ? "Stop simulation" : "Run simulation");
    if (running) {
        elapsedTimer.restart();
        simulationTimer.start();
        statusBar()->showMessage(
            "Simulation running: Joule and solar heating are balanced by natural and wind convection.");
    } else {
        simulationTimer.stop();
        elapsedTimer.restart();
        statusBar()->showMessage(
            "Simulation stopped. Demand estimate assumes 20 C wires and 0 mph wind; thermal updates are paused.");
    }
    runSimulation();
}

void MainSimulationWindow::resetSimulation()
{
    setSimulationRunning(false);
    for (auto& wire : connections.wires()) {
        wire->resetThermalState();
    }
    runSimulation();
    statusBar()->showMessage(
        "Simulation reset to the base grid: wire temperatures are 20 C; layout and settings were preserved.",
        6000);
}

void MainSimulationWindow::updateWireGhost(const QPointF& position)
{
    if (!ghostWireItem || activeTool != "wire" || pendingWireSourceId == -1 ||
        !pointerOverGrid) {
        if (ghostWireItem) {
            ghostWireItem->hide();
        }
        return;
    }

    const ParentOfObjects* source = nullptr;
    for (const auto& object : objects) {
        if (object->id() == pendingWireSourceId) {
            source = object.get();
            break;
        }
    }
    if (!source) {
        ghostWireItem->hide();
        return;
    }

    QPainterPath path(terminalScenePosition(
        *source, pendingWireSourceTerminal, gridSize));
    const QPointF start = path.currentPosition();
    const QPointF delta = position - start;
    const double sag = std::max(5.0, std::hypot(delta.x(), delta.y()) * 0.02);
    path.cubicTo(start + delta / 3.0 + QPointF(0.0, sag),
                 start + delta * (2.0 / 3.0) + QPointF(0.0, sag),
                 position);
    ghostWireItem->setPath(path);
    ghostWireItem->show();
}

void MainSimulationWindow::displayAllInformation()
{
    if (informationDialog) {
        refreshInformationTable();
        informationDialog->show();
        informationDialog->raise();
        informationDialog->activateWindow();
        return;
    }

    informationDialog = new QDialog(this);
    informationDialog->setAttribute(Qt::WA_DeleteOnClose);
    informationDialog->setWindowTitle("All simulator information");
    informationDialog->resize(1200, 520);
    auto* layout = new QVBoxLayout(informationDialog);
    informationTable = new QTableWidget(informationDialog);
    informationTable->setColumnCount(12);
    informationTable->setHorizontalHeaderLabels(
        {"UID", "Type", "Position / endpoints", "Height (m)", "Voltage (V)",
         "Current (A)", "Required V / A", "Resistance (ohm)",
         "Resistivity (ohm m)", "Temperature (C)", "Sag / minimum height (m)",
         "Connections / status"});
    informationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    informationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    informationTable->setSelectionMode(QAbstractItemView::SingleSelection);
    informationTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    informationTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(informationTable);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close,
                                         Qt::Horizontal, informationDialog);
    auto* refreshButton = buttons->addButton("Refresh", QDialogButtonBox::ActionRole);
    connect(refreshButton, &QPushButton::clicked,
            this, &MainSimulationWindow::refreshInformationTable);
    connect(buttons, &QDialogButtonBox::rejected,
            informationDialog, &QDialog::close);
    layout->addWidget(buttons);

    connect(informationTable, &QTableWidget::cellClicked,
            this, [this](int row, int) {
                QTableWidgetItem* item = informationTable->item(row, 0);
                if (!item) {
                    return;
                }
                const int id = item->data(Qt::UserRole + 1).toInt();
                if (item->data(Qt::UserRole).toString() == "wire") {
                    selectWire(id);
                } else {
                    selectObject(id);
                    for (std::size_t index = 0; index < objects.size(); ++index) {
                        if (objects[index]->id() == id) {
                            view->centerOn(objectItems[index]);
                            break;
                        }
                    }
                }
            });
    connect(informationDialog, &QDialog::finished, this, [this] {
        informationDialog = nullptr;
        informationTable = nullptr;
    });
    refreshInformationTable();
    informationDialog->show();
}

void MainSimulationWindow::refreshInformationTable()
{
    if (!informationTable) {
        return;
    }
    informationTable->setRowCount(0);
    const auto setCell = [this](int row, int column, const QString& value) {
        auto* item = new QTableWidgetItem(value);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        informationTable->setItem(row, column, item);
    };
    const auto findObject = [this](int id) -> const ParentOfObjects* {
        for (const auto& object : objects) {
            if (object->id() == id) {
                return object.get();
            }
        }
        return nullptr;
    };

    for (const auto& object : objects) {
        const int row = informationTable->rowCount();
        informationTable->insertRow(row);
        setCell(row, 0, object->uid());
        informationTable->item(row, 0)->setData(Qt::UserRole, "object");
        informationTable->item(row, 0)->setData(Qt::UserRole + 1, object->id());
        setCell(row, 1, object->name());
        setCell(row, 2, QString("(%1, %2)").arg(object->y()).arg(object->x()));
        setCell(row, 3, QString::number(object->connectionHeightMeters(), 'f', 2));
        setCell(row, 4, QString::number(object->voltage(), 'f', 2));
        setCell(row, 5, QString::number(object->current(), 'f', 3));
        QString requirements = "—";
        if (const auto* house = dynamic_cast<const House*>(object.get())) {
            requirements = QString("%1 V / %2 A")
                               .arg(house->ratedVoltage(), 0, 'f', 1)
                               .arg(house->requiredCurrent(), 0, 'f', 2);
        } else if (const auto* generator =
                       dynamic_cast<const PowerGenerator*>(object.get())) {
            requirements = QString("%1 V / max %2 A")
                               .arg(generator->outputVoltage(), 0, 'f', 1)
                               .arg(generator->currentLimit(), 0, 'f', 2);
        }
        setCell(row, 6, requirements);
        setCell(row, 7, "—");
        setCell(row, 8, "—");
        setCell(row, 9, "—");
        setCell(row, 10, "—");
        QStringList attached;
        for (const auto& wire : connections.wires()) {
            if (wire->fromObject() == object->id()) {
                const ParentOfObjects* remote = findObject(wire->toObject());
                attached << QString("T%1 → %2 (%3 A)")
                                .arg(wire->fromTerminal())
                                .arg(remote ? remote->uid() : "?")
                                .arg(std::abs(wire->current()), 0, 'f', 2);
            } else if (wire->toObject() == object->id()) {
                const ParentOfObjects* remote = findObject(wire->fromObject());
                attached << QString("T%1 ← %2 (%3 A)")
                                .arg(wire->toTerminal())
                                .arg(remote ? remote->uid() : "?")
                                .arg(std::abs(wire->current()), 0, 'f', 2);
            }
        }
        setCell(row, 11, attached.isEmpty() ? "No wires" : attached.join("; "));
    }

    for (const auto& wire : connections.wires()) {
        const ParentOfObjects* from = findObject(wire->fromObject());
        const ParentOfObjects* to = findObject(wire->toObject());
        const int row = informationTable->rowCount();
        informationTable->insertRow(row);
        setCell(row, 0, wire->uid());
        informationTable->item(row, 0)->setData(Qt::UserRole, "wire");
        informationTable->item(row, 0)->setData(Qt::UserRole + 1, wire->id());
        setCell(row, 1, wire->isGroundWire() ? "Ground wire" : "Wire");
        setCell(row, 2, QString("%1:T%2 → %3:T%4")
                            .arg(from ? from->uid() : "?")
                            .arg(wire->fromTerminal())
                            .arg(to ? to->uid() : "?")
                            .arg(wire->toTerminal()));
        setCell(row, 3, QString("%1 → %2")
                            .arg(wire->fromHeightMeters(), 0, 'f', 2)
                            .arg(wire->toHeightMeters(), 0, 'f', 2));
        setCell(row, 4, QString::number(std::abs(wire->voltageDrop()), 'f', 3));
        setCell(row, 5, QString::number(std::abs(wire->current()), 'f', 3));
        setCell(row, 6, "—");
        setCell(row, 7, QString::number(wire->resistance(), 'g', 6));
        setCell(row, 8, QString::number(wire->get_resistivity(), 'g', 6));
        setCell(row, 9, QString::number(wire->temperature(), 'f', 2));
        setCell(row, 10, QString("%1 / %2")
                             .arg(wire->sagMeters(), 0, 'f', 2)
                             .arg(wire->minimumHeightMeters(), 0, 'f', 2));
        setCell(row, 11,
                QString("Length %1 m (base %2 m); %3")
                    .arg(wire->expandedLength(), 0, 'f', 2)
                    .arg(wire->length(), 0, 'f', 2)
                    .arg(wire->isRed()
                             ? "RED / clearance or electrical warning"
                             : "OK"));
    }
}

void MainSimulationWindow::saveSimulation()
{
    QJsonObject root;
    root.insert("formatVersion", 1);
    root.insert("speedMultiplier", simulationSpeedMultiplier);
    QJsonArray objectArray;
    for (const auto& object : objects) {
        QJsonObject item;
        item.insert("id", object->id());
        item.insert("uid", object->uid());
        item.insert("type", objectKindKey(object->kind()));
        item.insert("x", object->x());
        item.insert("y", object->y());
        item.insert("rotation", object->rotation());
        item.insert("connectionHeightMeters", object->connectionHeightMeters());
        if (const auto* house = dynamic_cast<const House*>(object.get())) {
            item.insert("ratedVoltage", house->ratedVoltage());
            item.insert("requiredCurrent", house->requiredCurrent());
        }
        if (const auto* generator =
                dynamic_cast<const PowerGenerator*>(object.get())) {
            item.insert("outputVoltage", generator->outputVoltage());
            item.insert("currentLimit", generator->currentLimit());
        }
        if (const auto* separator =
                dynamic_cast<const WireSeparator*>(object.get())) {
            item.insert("closed", separator->isClosed());
        }
        objectArray.append(item);
    }
    root.insert("objects", objectArray);

    QJsonArray wireArray;
    for (const auto& wire : connections.wires()) {
        QJsonObject item;
        item.insert("id", wire->id());
        item.insert("uid", wire->uid());
        item.insert("fromObject", wire->fromObject());
        item.insert("fromTerminal", wire->fromTerminal());
        item.insert("toObject", wire->toObject());
        item.insert("toTerminal", wire->toTerminal());
        item.insert("lengthMeters", wire->length());
        item.insert("shortCircuit", wire->isShortCircuit());
        item.insert("material", static_cast<int>(wire->material()));
        item.insert("resistivity", wire->get_resistivity());
        item.insert("temperature", 20.0);
        wireArray.append(item);
    }
    root.insert("wires", wireArray);

    const QString fileName = QFileDialog::getSaveFileName(
        this, "Save simulator", QDir("save").filePath("city.json"),
        "Grid simulator JSON (*.json)");
    if (fileName.isEmpty()) {
        return;
    }
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save failed", file.errorString());
        return;
    }
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size() || !file.commit()) {
        QMessageBox::warning(this, "Save failed", file.errorString());
        return;
    }
    statusBar()->showMessage("Simulation saved to " + fileName, 5000);
}

void MainSimulationWindow::loadSimulation()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this, "Load simulator", "save", "Grid simulator JSON (*.json)");
    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Load failed", file.errorString());
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, "Load failed",
                             "Invalid simulator JSON: " + parseError.errorString());
        return;
    }

    const QJsonObject root = document.object();
    if (root.value("formatVersion").toInt() != 1 ||
        !root.value("objects").isArray() || !root.value("wires").isArray()) {
        QMessageBox::warning(this, "Load failed",
                             "Unsupported or incomplete simulator save file.");
        return;
    }
    std::vector<std::unique_ptr<ParentOfObjects>> loadedObjects;
    Connections loadedConnections;
    QSet<QString> loadedUids;
    QSet<int> loadedIds;
    QSet<int> loadedWireIds;
    int maxObjectId = 0;
    int maxWireId = 0;
    QString error;
    const auto number = [](const QJsonObject& item, const char* key,
                           double fallback) {
        const QJsonValue value = item.value(QLatin1String(key));
        return value.isDouble() ? value.toDouble() : fallback;
    };
    for (const QJsonValue& value : root.value("objects").toArray()) {
        if (!value.isObject()) {
            error = "Object entries must be JSON objects.";
            break;
        }
        const QJsonObject item = value.toObject();
        const int id = item.value("id").toInt(-1);
        const QString uid = item.value("uid").toString();
        const QString type = item.value("type").toString();
        const int x = item.value("x").toInt();
        const int y = item.value("y").toInt();
        const int rotation = item.value("rotation").toInt(-1);
        if (id <= 0 || loadedIds.contains(id) || uid.size() != 8 ||
            loadedUids.contains(uid) || rotation < 0 || rotation > 3) {
            error = "Object IDs, 8-character UIDs, or rotations are invalid or duplicated.";
            break;
        }
        std::unique_ptr<ParentOfObjects> object;
        if (type == "house") {
            object = std::make_unique<House>(id, x, y);
        } else if (type == "apartment") {
            object = std::make_unique<Apartment>(id, x, y);
        } else if (type == "pole") {
            object = std::make_unique<UtilityPole>(id, x, y);
        } else if (type == "ground") {
            object = std::make_unique<GroundWireConnection>(id, x, y);
        } else if (type == "separator") {
            object = std::make_unique<WireSeparator>(id, x, y);
        } else if (type == "generator") {
            object = std::make_unique<PowerGenerator>(id, x, y);
        } else {
            error = "Unknown object type: " + type;
            break;
        }
        if (!object->setUid(uid, true)) {
            error = "Object UIDs must be eight ASCII letters or digits.";
            break;
        }
        object->setConnectionHeightMeters(
            number(item, "connectionHeightMeters",
                   object->connectionHeightMeters()));
        for (int turn = 0; turn < rotation; ++turn) {
            object->rotateClockwise();
        }
        if (auto* house = dynamic_cast<House*>(object.get())) {
            house->setRatedVoltage(number(item, "ratedVoltage",
                                          house->ratedVoltage()));
            house->setRequiredCurrent(number(item, "requiredCurrent",
                                             house->requiredCurrent()));
        }
        if (auto* generator = dynamic_cast<PowerGenerator*>(object.get())) {
            generator->setOutputVoltage(number(item, "outputVoltage",
                                               generator->outputVoltage()));
            generator->setCurrentLimit(number(item, "currentLimit",
                                              generator->currentLimit()));
        }
        if (auto* separator = dynamic_cast<WireSeparator*>(object.get())) {
            separator->setClosed(item.value("closed").toBool(false));
        }
        for (const auto& existing : loadedObjects) {
            const bool overlaps =
                object->x() < existing->x() + existing->width() &&
                object->x() + object->width() > existing->x() &&
                object->y() < existing->y() + existing->height() &&
                object->y() + object->height() > existing->y();
            if (overlaps) {
                error = "The save contains overlapping components.";
                break;
            }
        }
        if (!error.isEmpty()) {
            break;
        }
        loadedIds.insert(id);
        loadedUids.insert(uid);
        maxObjectId = std::max(maxObjectId, id);
        loadedObjects.push_back(std::move(object));
    }

    for (const QJsonValue& value : root.value("wires").toArray()) {
        if (!error.isEmpty()) {
            break;
        }
        if (!value.isObject()) {
            error = "Wire entries must be JSON objects.";
            break;
        }
        const QJsonObject item = value.toObject();
        const int id = item.value("id").toInt(-1);
        const QString uid = item.value("uid").toString();
        const int fromId = item.value("fromObject").toInt(-1);
        const int toId = item.value("toObject").toInt(-1);
        const int fromTerminal = item.value("fromTerminal").toInt(-1);
        const int toTerminal = item.value("toTerminal").toInt(-1);
        const double length = number(item, "lengthMeters", 0.0);
        const int material = item.value("material").toInt(-1);
        const double resistivity = number(item, "resistivity", 0.0);
        ParentOfObjects* from = nullptr;
        ParentOfObjects* to = nullptr;
        for (const auto& object : loadedObjects) {
            if (object->id() == fromId) from = object.get();
            if (object->id() == toId) to = object.get();
        }
        if (id <= 0 || loadedWireIds.contains(id) ||
            uid.size() != 8 || loadedUids.contains(uid) ||
            !std::isfinite(length) ||
            length <= 0.0 || material < 0 || material > 2 ||
            !std::isfinite(resistivity) || resistivity < 1.0e-10 ||
            resistivity > 1.0e-4) {
            error = "Wire data is invalid.";
            break;
        }
        if (!from || !to ||
            !loadedConnections.addWire(id, *from, fromTerminal, *to,
                                       toTerminal, length,
                                       item.value("shortCircuit").toBool(false),
                                       &error)) {
            if (error.isEmpty()) {
                error = "A wire refers to an object that is not in this save.";
            }
            break;
        }
        Wire& wire = *loadedConnections.wires().back();
        if (!wire.setUid(uid, true)) {
            error = "Wire UIDs must be unique eight-character alphanumeric values.";
            break;
        }
        wire.setMaterial(static_cast<WireMaterial>(material));
        wire.setResistivity(resistivity);
        wire.setTemperatureCelsius(number(item, "temperature", 20.0));
        loadedWireIds.insert(id);
        loadedUids.insert(uid);
        maxWireId = std::max(maxWireId, id);
    }

    if (!error.isEmpty()) {
        QMessageBox::warning(this, "Load failed", error);
        return;
    }

    objects = std::move(loadedObjects);
    connections = std::move(loadedConnections);
    nextObjectId = maxObjectId + 1;
    nextWireId = maxWireId + 1;
    const double speed = number(root, "speedMultiplier", 10.0);
    simulationSpeedMultiplier = std::clamp(speed, 0.1, 1000.0);
    pendingWireSourceId = -1;
    pendingWireSourceTerminal = -1;
    selectedObjectId = -1;
    selectedWireId = -1;
    refreshObjectGraphics();
    refreshWireGraphics();
    refreshInspector();
    runSimulation();
    refreshInformationTable();
    statusBar()->showMessage("Loaded simulation from " + fileName, 5000);
}
