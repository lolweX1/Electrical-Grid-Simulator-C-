#include "PowerGenerator.hpp"

#include <algorithm>

PowerGenerator::PowerGenerator(int id, int gridX, int gridY)
    : ParentOfObjects(id, "icons/power_generator.png", 4, 4, gridX, gridY,
                      "Power generator", ObjectKind::PowerGenerator, 8.0)
{
}

void PowerGenerator::setOutputVoltage(double value)
{
    sourceVoltage = std::max(1.0, value);
}

void PowerGenerator::setCurrentLimit(double value)
{
    maximumCurrent = std::max(0.1, value);
}
