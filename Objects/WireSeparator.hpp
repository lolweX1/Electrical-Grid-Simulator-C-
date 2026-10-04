#ifndef WIRE_SEPARATOR_HPP
#define WIRE_SEPARATOR_HPP

#include "ParentOfObjects.hpp"

#include <array>
#include <cstddef>

class WireSeparator : public ParentOfObjects {
public:
    WireSeparator(int id, int gridX, int gridY);
    int terminalCount() const override { return 4; }
    PortSide terminalSide(int terminal) const override;
    bool isClosed() const { return closed; }
    void setClosed(bool value) { closed = value; }
    double outputVoltage(int terminal) const
    {
        return outputVoltages[static_cast<std::size_t>(terminal - 1)];
    }
    double outputCurrent(int terminal) const
    {
        return outputCurrents[static_cast<std::size_t>(terminal - 1)];
    }
    void setOutputElectricalState(int terminal, double voltage, double current)
    {
        const std::size_t index = static_cast<std::size_t>(terminal - 1);
        outputVoltages[index] = voltage;
        outputCurrents[index] = current;
    }

private:
    bool closed = true;
    std::array<double, 3> outputVoltages{};
    std::array<double, 3> outputCurrents{};
};

#endif
