#ifndef WIRE_HPP
#define WIRE_HPP

#include <algorithm>
#include <cmath>
#include <QString>

enum class WireMaterial {
    Copper,
    Aluminum,
    Steel
};

class Wire {
public:
    Wire(int id, int fromObjectId, int fromTerminal, int toObjectId, int toTerminal,
         double lengthMeters,
         WireMaterial material = WireMaterial::Aluminum, bool shortCircuit = false);

    int id() const { return wireId; }
    const QString& uid() const { return uniqueId; }
    bool setUid(const QString& value, bool allowRegistered = false);
    int fromObject() const { return firstObjectId; }
    int fromTerminal() const { return firstTerminal; }
    int toObject() const { return secondObjectId; }
    int toTerminal() const { return secondTerminal; }
    double length() const { return baseLengthMeters; }
    double expandedLength() const;
    double temperature() const { return wireTemperature; }
    double temperatureChange() const { return temperatureChangeCelsius; }
    double current() const { return wireCurrent; }
    double voltageDrop() const { return wireVoltageDrop; }
    double resistance() const;
    double resistanceAtTemperature(double temperatureCelsius) const;
    double sagMeters() const;
    double sagRatio() const;
    double fromHeightMeters() const { return firstHeightMeters; }
    double toHeightMeters() const { return secondHeightMeters; }
    double minimumHeightMeters() const;
    void setEndpointHeights(double fromHeightMeters, double toHeightMeters);
    bool isGroundWire() const { return groundWire; }
    void setGroundWire(bool value) { groundWire = value; }
    double get_resistivity() const;
    double baseResistivity() const;
    double density() const;
    double specificHeatCapacity() const;
    double expansionCoefficient() const;
    double mass() const;
    WireMaterial material() const { return wireMaterial; }
    void setMaterial(WireMaterial material);
    void setResistivity(double resistivity);
    bool isShortCircuit() const { return shortCircuit; }
    bool isOverloaded() const { return std::abs(wireCurrent) > ampacity; }
    bool isOverheated() const
    {
        return wireTemperature >= 90.0 ||
               (!groundWire && sagRatio() > 0.08);
    }
    bool isLowClearance() const
    {
        return !groundWire && minimumHeightMeters() < 5.0;
    }
    bool isRed() const
    {
        return shortCircuit || isOverloaded() || isOverheated() || isLowClearance();
    }

    void setElectricalState(double current, double voltageDrop);
    void setTemperatureCelsius(double value)
    {
        wireTemperature = std::max(20.0, value);
    }
    void resetThermalState()
    {
        wireTemperature = 20.0;
        temperatureChangeCelsius = 0.0;
    }
    void updateTemperature(double elapsedSeconds, double solarIrradiance = 800.0,
                           double windSpeedMph = 0.0);

private:
    int wireId;
    QString uniqueId;
    int firstObjectId;
    int firstTerminal;
    int secondObjectId;
    int secondTerminal;
    double baseLengthMeters;
    double wireTemperature = 20.0;
    double wireCurrent = 0.0;
    double wireVoltageDrop = 0.0;
    double crossSectionArea = 25.0e-6;
    double ampacity = 80.0;
    double shortResistance = 0.05;
    WireMaterial wireMaterial;
    double resistivityOverride = 0.0;
    double temperatureChangeCelsius = 0.0;
    double firstHeightMeters = 5.0;
    double secondHeightMeters = 5.0;
    bool shortCircuit;
    bool groundWire = false;
};

#endif
