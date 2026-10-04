#ifndef HOUSE_HPP
#define HOUSE_HPP

#include "ParentOfObjects.hpp"

class House : public ParentOfObjects {
public:
    House(int id, int gridX, int gridY);
    int terminalCount() const override { return 2; }
    PortSide terminalSide(int terminal) const override
    {
        return terminal == 0 ? PortSide::Left : PortSide::Right;
    }

    double requiredCurrent() const { return demandCurrent; }
    double ratedVoltage() const { return nominalVoltage; }
    void setRequiredCurrent(double value);
    void setRatedVoltage(double value);
    double loadConductance() const;
    bool isPowered() const;

protected:
    House(int id, int gridX, int gridY, const QString& imagePath,
          const QString& displayName, ObjectKind kind, double defaultVoltage,
          double defaultCurrent, double defaultConnectionHeightMeters);

private:
    double demandCurrent = 10.0;
    double nominalVoltage = 480.0;
};

#endif
