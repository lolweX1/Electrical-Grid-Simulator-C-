#include "House.hpp"

#include <algorithm>

House::House(int id, int gridX, int gridY)
    : House(id, gridX, gridY, "icons/house_8_x_4.png", "House",
            ObjectKind::House, 480.0, 10.0, 10.0)
{
}

House::House(int id, int gridX, int gridY, const QString& imagePath,
             const QString& displayName, ObjectKind kind, double defaultVoltage,
             double defaultCurrent, double defaultConnectionHeightMeters)
    : ParentOfObjects(id, imagePath, 8, 4, gridX, gridY, displayName, kind,
                      defaultConnectionHeightMeters),
      demandCurrent(defaultCurrent),
      nominalVoltage(defaultVoltage)
{
}

void House::setRequiredCurrent(double value)
{
    demandCurrent = std::max(0.1, value);
}

void House::setRatedVoltage(double value)
{
    nominalVoltage = std::max(1.0, value);
}

double House::loadConductance() const
{
    return demandCurrent / nominalVoltage;
}

bool House::isPowered() const
{
    return current() + std::max(0.02, demandCurrent * 0.01) >= demandCurrent;
}
