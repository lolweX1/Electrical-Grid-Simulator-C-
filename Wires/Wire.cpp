#include "Wire.hpp"

#include "../Objects/ParentOfObjects.hpp"

Wire::Wire(int id, int fromObjectId, int fromTerminal, int toObjectId,
           int toTerminal, double lengthMeters,
           WireMaterial material, bool shortCircuit)
    : wireId(id),
      uniqueId(ParentOfObjects::generateUniqueUid()),
      firstObjectId(fromObjectId),
      firstTerminal(fromTerminal),
      secondObjectId(toObjectId),
      secondTerminal(toTerminal),
      baseLengthMeters(std::max(0.1, lengthMeters)),
      wireMaterial(material),
      shortCircuit(shortCircuit)
{
}

bool Wire::setUid(const QString& value, bool allowRegistered)
{
    if (!ParentOfObjects::registerUid(value, allowRegistered || value == uniqueId)) {
        return false;
    }
    uniqueId = value;
    return true;
}

double Wire::get_resistivity() const
{
    return resistivityOverride > 0.0 ? resistivityOverride : baseResistivity();
}

double Wire::baseResistivity() const
{
    switch (wireMaterial) {
    case WireMaterial::Copper:
        return 1.68e-8;
    case WireMaterial::Aluminum:
        return 2.82e-8;
    case WireMaterial::Steel:
        return 1.43e-7;
    }
    return 2.82e-8;
}

double Wire::density() const
{
    switch (wireMaterial) {
    case WireMaterial::Copper:
        return 8960.0;
    case WireMaterial::Aluminum:
        return 2700.0;
    case WireMaterial::Steel:
        return 7850.0;
    }
    return 2700.0;
}

double Wire::specificHeatCapacity() const
{
    switch (wireMaterial) {
    case WireMaterial::Copper:
        return 385.0;
    case WireMaterial::Aluminum:
        return 900.0;
    case WireMaterial::Steel:
        return 490.0;
    }
    return 900.0;
}

double Wire::expansionCoefficient() const
{
    switch (wireMaterial) {
    case WireMaterial::Copper:
        return 17.0e-6;
    case WireMaterial::Aluminum:
        return 23.0e-6;
    case WireMaterial::Steel:
        return 12.0e-6;
    }
    return 23.0e-6;
}

double Wire::mass() const
{
    return density() * crossSectionArea * baseLengthMeters;
}

double Wire::expandedLength() const
{
    const double thermalExpansion =
        1.0 + expansionCoefficient() * (wireTemperature - 20.0);
    return baseLengthMeters * std::max(0.1, thermalExpansion);
}

void Wire::setMaterial(WireMaterial material)
{
    const double resistivityScale = resistivityOverride > 0.0
        ? resistivityOverride / baseResistivity() : 0.0;
    wireMaterial = material;
    if (resistivityOverride > 0.0) {
        resistivityOverride = resistivityScale * baseResistivity();
    }
}

void Wire::setResistivity(double resistivity)
{
    resistivityOverride = std::max(1.0e-10, resistivity);
}

double Wire::resistance() const
{
    return resistanceAtTemperature(wireTemperature);
}

double Wire::resistanceAtTemperature(double temperatureCelsius) const
{
    if (shortCircuit) {
        return shortResistance;
    }

    const double temperatureDifference = temperatureCelsius - 20.0;
    double temperatureCoefficient = 0.00403;
    if (wireMaterial == WireMaterial::Copper) {
        temperatureCoefficient = 0.00393;
    } else if (wireMaterial == WireMaterial::Steel) {
        temperatureCoefficient = 0.006;
    }
    const double resistivity =
        get_resistivity() * std::max(0.1, 1.0 + temperatureCoefficient * temperatureDifference);
    const double thermallyExpandedLength =
        baseLengthMeters * std::max(
            0.1, 1.0 + expansionCoefficient() * temperatureDifference);
    return std::max(1.0e-6, resistivity * thermallyExpandedLength / crossSectionArea);
}

double Wire::sagRatio() const
{
    const double thermalStrain =
        expansionCoefficient() * std::max(0.0, wireTemperature - 20.0);
    return 0.02 + std::sqrt(3.0 * thermalStrain);
}

double Wire::sagMeters() const
{
    return expandedLength() * sagRatio();
}

void Wire::setEndpointHeights(double fromHeight, double toHeight)
{
    firstHeightMeters = std::max(0.0, fromHeight);
    secondHeightMeters = std::max(0.0, toHeight);
}

double Wire::minimumHeightMeters() const
{
    const double sag = sagMeters();
    const double slope = secondHeightMeters - firstHeightMeters;
    const double quadratic = 4.0 * sag;
    if (quadratic <= 0.0) {
        return std::min(firstHeightMeters, secondHeightMeters);
    }
    const double minimumPosition =
        std::clamp((quadratic - slope) / (2.0 * quadratic), 0.0, 1.0);
    return firstHeightMeters + slope * minimumPosition -
           quadratic * minimumPosition * (1.0 - minimumPosition);
}

void Wire::setElectricalState(double current, double voltageDrop)
{
    wireCurrent = current;
    wireVoltageDrop = voltageDrop;
}

void Wire::updateTemperature(double elapsedSeconds, double solarIrradiance,
                             double windSpeedMph)
{
    if (elapsedSeconds <= 0.0) {
        temperatureChangeCelsius = 0.0;
        return;
    }

    const double wireDiameter = 2.0 * std::sqrt(crossSectionArea / 3.141592653589793);
    const double heatCapacity = std::max(1.0e-6, mass() * specificHeatCapacity());
    const double oldTemperature = wireTemperature;
    constexpr double ambientTemperatureCelsius = 20.0;
    constexpr double milesPerHourToMetersPerSecond = 0.44704;
    const double windSpeedMetersPerSecond =
        std::max(0.0, windSpeedMph) * milesPerHourToMetersPerSecond;
    double remaining = elapsedSeconds;
    while (remaining > 0.0) {
        const double interval = std::min(0.1, remaining);
        const double heatGenerated = wireCurrent * wireCurrent * resistance();
        const double solarHeat = std::max(0.0, solarIrradiance) * wireDiameter *
                                 baseLengthMeters * 0.5;
        const double convectionCoefficient =
            10.0 + 5.7 * std::sqrt(windSpeedMetersPerSecond);
        const double convection = convectionCoefficient *
                                  3.141592653589793 * wireDiameter *
                                  baseLengthMeters;
        const double heatLost =
            convection * (wireTemperature - ambientTemperatureCelsius);
        wireTemperature = std::max(
            ambientTemperatureCelsius, wireTemperature +
                      (heatGenerated + solarHeat - heatLost) * interval /
                          heatCapacity);
        remaining -= interval;
    }
    temperatureChangeCelsius = wireTemperature - oldTemperature;
}
