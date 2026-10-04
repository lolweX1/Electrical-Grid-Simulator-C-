#ifndef POWER_GENERATOR_HPP
#define POWER_GENERATOR_HPP

#include "ParentOfObjects.hpp"

class PowerGenerator : public ParentOfObjects {
public:
    PowerGenerator(int id, int gridX, int gridY);
    int terminalCount() const override { return 8; }
    PortSide terminalSide(int terminal) const override
    {
        return terminal < 4 ? PortSide::Left : PortSide::Right;
    }
    bool isOutputTerminal(int terminal) const { return terminal >= 0 && terminal < 4; }

    double outputVoltage() const { return sourceVoltage; }
    double currentLimit() const { return maximumCurrent; }
    double internalResistance() const { return sourceResistance; }
    void setOutputVoltage(double value);
    void setCurrentLimit(double value);

private:
    double sourceVoltage = 480.0;
    double maximumCurrent = 40.0;
    double sourceResistance = 0.05;
};

#endif
