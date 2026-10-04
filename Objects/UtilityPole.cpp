#include "UtilityPole.hpp"

UtilityPole::UtilityPole(int id, int gridX, int gridY)
    : ParentOfObjects(id, "icons/power_node.png", 1, 1, gridX, gridY,
                      "Utility pole", ObjectKind::UtilityPole, 10.0)
{
}
