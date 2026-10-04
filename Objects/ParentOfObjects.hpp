#ifndef PARENT_OF_OBJECTS_HPP
#define PARENT_OF_OBJECTS_HPP

#include <QImage>
#include <QSet>
#include <QString>

enum class ObjectKind {
    House,
    Apartment,
    UtilityPole,
    GroundWireConnection,
    WireSeparator,
    PowerGenerator
};

enum class PortSide {
    Left,
    Top,
    Right,
    Bottom
};

class ParentOfObjects {
public:
    ParentOfObjects(const ParentOfObjects&) = delete;
    ParentOfObjects& operator=(const ParentOfObjects&) = delete;
    ParentOfObjects(int id, const QString& imagePath, int widthTiles, int heightTiles,
                    int gridX, int gridY, const QString& displayName, ObjectKind kind,
                    double connectionHeightMeters = 5.0);
    virtual ~ParentOfObjects();

    int id() const { return objectId; }
    int x() const { return gridX; }
    int y() const { return gridY; }
    int width() const { return rotationTurns % 2 == 0 ? tileWidth : tileHeight; }
    int height() const { return rotationTurns % 2 == 0 ? tileHeight : tileWidth; }
    int rotation() const { return rotationTurns; }
    void setPosition(int x, int y) { gridX = x; gridY = y; }
    void rotateClockwise() { rotationTurns = (rotationTurns + 1) % 4; }
    virtual int terminalCount() const { return 1; }
    virtual PortSide terminalSide(int terminal) const;
    const QImage& image() const { return icon; }
    const QString& name() const { return label; }
    const QString& uid() const { return uniqueId; }
    bool setUid(const QString& value, bool allowRegistered = false);
    static QString generateUniqueUid();
    static bool registerUid(const QString& value, bool allowRegistered = false);
    ObjectKind kind() const { return objectKind; }

    double voltage() const { return nodeVoltage; }
    double current() const { return nodeCurrent; }
    double connectionHeightMeters() const { return connectionHeight; }
    void setConnectionHeightMeters(double value);
    void setVoltage(double value) { nodeVoltage = value; }
    void setCurrent(double value) { nodeCurrent = value; }

private:
    static QSet<QString>& registeredUids();

    int objectId;
    QString label;
    QString uniqueId;
    ObjectKind objectKind;
    QImage icon;
    int tileWidth;
    int tileHeight;
    int gridX;
    int gridY;
    int rotationTurns = 0;
    double nodeVoltage = 0.0;
    double nodeCurrent = 0.0;
    double connectionHeight;
};

#endif
