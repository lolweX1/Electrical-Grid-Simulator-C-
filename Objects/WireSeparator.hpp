#ifndef WIRE_SEPARATOR_HPP
#define WIRE_SEPARATOR_HPP

#include "ParentOfObjects.hpp"

class WireSeparator : public ParentOfObjects {
public:
    WireSeparator(int id, int gridX, int gridY);
    int terminalCount() const override { return 4; }
    PortSide terminalSide(int terminal) const override;
    bool isClosed() const { return closed; }
    void setClosed(bool value) { closed = value; }

private:
    bool closed = false;
};

#endif
