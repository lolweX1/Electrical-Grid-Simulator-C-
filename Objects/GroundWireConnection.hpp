#ifndef GROUND_WIRE_CONNECTION_HPP
#define GROUND_WIRE_CONNECTION_HPP

#include "ParentOfObjects.hpp"

class GroundWireConnection : public ParentOfObjects {
public:
    GroundWireConnection(int id, int gridX, int gridY);
    int terminalCount() const override { return 1; }
    PortSide terminalSide(int) const override { return PortSide::Bottom; }
};

#endif
