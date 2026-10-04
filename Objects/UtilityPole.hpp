#ifndef UTILITY_POLE_HPP
#define UTILITY_POLE_HPP

#include "ParentOfObjects.hpp"

class UtilityPole : public ParentOfObjects {
public:
    UtilityPole(int id, int gridX, int gridY);
    int terminalCount() const override { return 2; }
    PortSide terminalSide(int terminal) const override
    {
        return terminal == 0 ? PortSide::Left : PortSide::Right;
    }
};

#endif
