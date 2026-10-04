#include "GroundWireConnection.hpp"

GroundWireConnection::GroundWireConnection(int id, int gridX, int gridY)
    : ParentOfObjects(id, "icons/ground_wire_connection.png", 1, 1,
                      gridX, gridY, "Ground wire connection",
                      ObjectKind::GroundWireConnection, 0.0)
{
}
