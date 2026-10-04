#include "WireSeparator.hpp"

WireSeparator::WireSeparator(int id, int gridX, int gridY)
    : ParentOfObjects(id, "icons/wire_seperator.png", 1, 1, gridX, gridY,
                      "Wire separator", ObjectKind::WireSeparator, 8.0)
{
}

PortSide WireSeparator::terminalSide(int terminal) const
{
    switch (terminal) {
    case 0:
        return PortSide::Bottom;
    case 1:
        return PortSide::Left;
    case 2:
        return PortSide::Top;
    default:
        return PortSide::Right;
    }
}
