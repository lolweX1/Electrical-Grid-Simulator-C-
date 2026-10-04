#include "ParentOfObjects.hpp"

#include <algorithm>
#include <cmath>
#include <QRandomGenerator>

QSet<QString>& ParentOfObjects::registeredUids()
{
    static QSet<QString> uids;
    return uids;
}

QString ParentOfObjects::generateUniqueUid()
{
    static constexpr char alphabet[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    QString candidate;
    do {
        candidate.clear();
        for (int index = 0; index < 8; ++index) {
            candidate.append(alphabet[QRandomGenerator::global()->bounded(
                static_cast<int>(sizeof(alphabet) - 1))]);
        }
    } while (registeredUids().contains(candidate));
    registeredUids().insert(candidate);
    return candidate;
}

ParentOfObjects::ParentOfObjects(int id, const QString& imagePath, int widthTiles,
                                 int heightTiles, int gridX, int gridY,
                                 const QString& displayName, ObjectKind kind,
                                 double connectionHeightMeters)
    : objectId(id),
      label(displayName),
      uniqueId(generateUniqueUid()),
      objectKind(kind),
      icon(imagePath),
      tileWidth(icon.isNull()
                    ? std::max(1, widthTiles)
                    : std::max(1, static_cast<int>(
                          std::ceil(static_cast<double>(icon.width()) / 64.0)))),
      tileHeight(icon.isNull()
                     ? std::max(1, heightTiles)
                     : std::max(1, static_cast<int>(
                           std::ceil(static_cast<double>(icon.height()) / 64.0)))),
      gridX(gridX),
      gridY(gridY),
      connectionHeight(connectionHeightMeters)
{
}

ParentOfObjects::~ParentOfObjects() = default;

bool ParentOfObjects::setUid(const QString& value, bool allowRegistered)
{
    if (!registerUid(value, allowRegistered || value == uniqueId)) {
        return false;
    }
    uniqueId = value;
    return true;
}

bool ParentOfObjects::registerUid(const QString& value, bool allowRegistered)
{
    if (value.size() != 8) {
        return false;
    }
    for (const QChar character : value) {
        if (!character.isLetterOrNumber() || character.unicode() > 127) {
            return false;
        }
    }
    if (registeredUids().contains(value) && !allowRegistered) {
        return false;
    }
    registeredUids().insert(value);
    return true;
}

void ParentOfObjects::setConnectionHeightMeters(double value)
{
    connectionHeight = std::max(0.0, value);
}

PortSide ParentOfObjects::terminalSide(int) const
{
    return PortSide::Left;
}
