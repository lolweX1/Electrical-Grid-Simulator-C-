#include "Connections.hpp"

#include "../Objects/House.hpp"
#include "../Objects/PowerGenerator.hpp"
#include "../Objects/UtilityPole.hpp"
#include "../Objects/WireSeparator.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
constexpr double separatorConductanceSiemens = 100.0;

enum class GeneratorMode {
    VoltageSource,
    CurrentLimited,
    Off
};

void addConductance(std::vector<std::vector<double>>& matrix,
                    std::size_t positive, std::size_t negative, double conductance)
{
    matrix[positive][positive] += conductance;
    matrix[negative][negative] += conductance;
    matrix[positive][negative] -= conductance;
    matrix[negative][positive] -= conductance;
}

bool solveDenseSystem(std::vector<std::vector<double>>& matrix,
                      std::vector<double>& values)
{
    const std::size_t size = values.size();
    for (std::size_t column = 0; column < size; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < size; ++row) {
            if (std::abs(matrix[row][column]) > std::abs(matrix[pivot][column])) {
                pivot = row;
            }
        }

        if (std::abs(matrix[pivot][column]) < 1.0e-18) {
            return false;
        }
        std::swap(matrix[column], matrix[pivot]);
        std::swap(values[column], values[pivot]);

        const double divisor = matrix[column][column];
        for (std::size_t entry = column; entry < size; ++entry) {
            matrix[column][entry] /= divisor;
        }
        values[column] /= divisor;

        for (std::size_t row = 0; row < size; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = matrix[row][column];
            for (std::size_t entry = column; entry < size; ++entry) {
                matrix[row][entry] -= factor * matrix[column][entry];
            }
            values[row] -= factor * values[column];
        }
    }
    return true;
}

bool solveLinearSystem(const std::vector<std::vector<double>>& matrix,
                       std::vector<double>& values,
                       const std::vector<std::size_t>& referenceNodes)
{
    const std::size_t size = values.size();
    std::vector<bool> visited(size, false);

    for (std::size_t root = 0; root < size; ++root) {
        if (visited[root]) {
            continue;
        }

        std::vector<std::size_t> component;
        component.push_back(root);
        visited[root] = true;
        for (std::size_t cursor = 0; cursor < component.size(); ++cursor) {
            const std::size_t row = component[cursor];
            for (std::size_t column = 0; column < size; ++column) {
                if (!visited[column] && row != column &&
                    matrix[row][column] != 0.0) {
                    visited[column] = true;
                    component.push_back(column);
                }
            }
        }

        std::vector<std::vector<double>> localMatrix(
            component.size(), std::vector<double>(component.size(), 0.0));
        std::vector<double> localValues(component.size(), 0.0);
        for (std::size_t localRow = 0; localRow < component.size(); ++localRow) {
            const std::size_t globalRow = component[localRow];
            localValues[localRow] = values[globalRow];
            for (std::size_t localColumn = 0;
                 localColumn < component.size(); ++localColumn) {
                localMatrix[localRow][localColumn] =
                    matrix[globalRow][component[localColumn]];
            }
        }

        std::size_t referenceLocal = component.size();
        for (std::size_t local = 0; local < component.size(); ++local) {
            if (std::find(referenceNodes.begin(), referenceNodes.end(),
                          component[local]) != referenceNodes.end()) {
                referenceLocal = local;
                break;
            }
        }
        if (referenceLocal < component.size()) {
            for (std::size_t local = 0; local < component.size(); ++local) {
                localMatrix[local][referenceLocal] = 0.0;
                localMatrix[referenceLocal][local] = 0.0;
            }
            localMatrix[referenceLocal][referenceLocal] = 1.0;
            localValues[referenceLocal] = 0.0;
        }

        if (!solveDenseSystem(localMatrix, localValues)) {
            return false;
        }
        for (std::size_t local = 0; local < component.size(); ++local) {
            values[component[local]] = localValues[local];
        }
    }
    return true;
}
}

bool Connections::addWire(int id, const ParentOfObjects& fromObject,
                          int fromTerminal, const ParentOfObjects& toObject,
                          int toTerminal, double lengthMeters,
                          bool shortCircuit, QString* errorMessage)
{
    const int fromObjectId = fromObject.id();
    const int toObjectId = toObject.id();
    if (fromTerminal < 0 || toTerminal < 0 ||
        fromTerminal >= fromObject.terminalCount() ||
        toTerminal >= toObject.terminalCount()) {
        if (errorMessage) {
            *errorMessage = "Connect wires to the visible component terminals.";
        }
        return false;
    }
    if (fromObjectId == toObjectId && fromTerminal == toTerminal) {
        if (errorMessage) {
            *errorMessage = "A wire must connect two different terminals.";
        }
        return false;
    }
    const auto* shortedGenerator =
        dynamic_cast<const PowerGenerator*>(&fromObject);
    if (shortCircuit &&
        (fromObjectId != toObjectId || !shortedGenerator ||
         shortedGenerator->isOutputTerminal(fromTerminal) ==
             shortedGenerator->isOutputTerminal(toTerminal))) {
        if (errorMessage) {
            *errorMessage = "A short circuit must bridge the two terminals of one generator.";
        }
        return false;
    }
    for (const auto& wire : connections) {
        if (wire->id() == id) {
            if (errorMessage) {
                *errorMessage = "Wire IDs must be unique.";
            }
            return false;
        }
        const bool sameDirection =
            wire->fromObject() == fromObjectId &&
            wire->fromTerminal() == fromTerminal &&
            wire->toObject() == toObjectId &&
            wire->toTerminal() == toTerminal;
        const bool reverseDirection =
            wire->fromObject() == toObjectId &&
            wire->fromTerminal() == toTerminal &&
            wire->toObject() == fromObjectId &&
            wire->toTerminal() == fromTerminal;
        if (sameDirection || reverseDirection) {
            if (errorMessage) {
                *errorMessage = "Those terminals are already connected by a wire.";
            }
            return false;
        }
    }

    const auto isTerminalAtCapacity = [this](const ParentOfObjects& object,
                                             int terminal) {
        return terminalConnectionCount(object.id(), terminal) >= 1;
    };
    if (isTerminalAtCapacity(fromObject, fromTerminal) ||
        isTerminalAtCapacity(toObject, toTerminal)) {
        if (errorMessage) {
            *errorMessage = "Each component terminal supports one wire.";
        }
        return false;
    }

    connections.push_back(std::make_unique<Wire>(
        id, fromObjectId, fromTerminal, toObjectId, toTerminal, lengthMeters,
        WireMaterial::Aluminum, shortCircuit));
    connections.back()->setEndpointHeights(
        fromObject.connectionHeightMeters(), toObject.connectionHeightMeters());
    return true;
}

void Connections::removeObject(int objectId)
{
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
                       [objectId](const std::unique_ptr<Wire>& wire) {
                           return wire->fromObject() == objectId ||
                                  wire->toObject() == objectId;
                       }),
        connections.end());
}

void Connections::removeWire(int wireId)
{
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
                       [wireId](const std::unique_ptr<Wire>& wire) {
                           return wire->id() == wireId;
                       }),
        connections.end());
}

int Connections::terminalConnectionCount(int objectId, int terminal) const
{
    int count = 0;
    for (const auto& wire : connections) {
        if ((wire->fromObject() == objectId && wire->fromTerminal() == terminal) ||
            (wire->toObject() == objectId && wire->toTerminal() == terminal)) {
            ++count;
        }
    }
    return count;
}

bool Connections::simulate(
    const std::vector<std::unique_ptr<ParentOfObjects>>& objects,
    double elapsedSeconds, QString* errorMessage, bool simulationRunning)
{
    std::vector<ParentOfObjects*> nodes;
    nodes.reserve(objects.size());
    for (const auto& object : objects) {
        nodes.push_back(object.get());
    }

    if (!solve(nodes, errorMessage, !simulationRunning)) {
        return false;
    }

    const double interval =
        simulationRunning ? std::max(0.0, elapsedSeconds) : 0.0;
    for (auto& wire : connections) {
        wire->updateTemperature(interval);
    }
    return true;
}

bool Connections::solve(const std::vector<ParentOfObjects*>& objects,
                        QString* errorMessage, bool standardConditions)
{
    std::size_t nodeCount = 0;
    std::unordered_map<int, std::vector<std::size_t>> terminalIndices;
    terminalIndices.reserve(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        ParentOfObjects* object = objects[i];
        std::vector<std::size_t> indices;
        indices.reserve(static_cast<std::size_t>(object->terminalCount()));
        for (int terminal = 0; terminal < object->terminalCount(); ++terminal) {
            indices.push_back(nodeCount++);
        }
        terminalIndices.emplace(object->id(), std::move(indices));
        object->setVoltage(0.0);
        object->setCurrent(0.0);
    }

    if (nodeCount == 0 && !connections.empty()) {
        if (errorMessage) {
            *errorMessage = "Wires cannot be simulated without connected components.";
        }
        return false;
    }
    if (nodeCount == 0) {
        return true;
    }

    const auto findTerminal = [&terminalIndices](int objectId, int terminal,
                                                  std::size_t* index) {
        const auto objectTerminals = terminalIndices.find(objectId);
        if (objectTerminals == terminalIndices.end() ||
            terminal < 0 ||
            static_cast<std::size_t>(terminal) >= objectTerminals->second.size()) {
            return false;
        }
        *index = objectTerminals->second[static_cast<std::size_t>(terminal)];
        return true;
    };

    for (const auto& wire : connections) {
        std::size_t from = 0;
        std::size_t to = 0;
        if (!findTerminal(wire->fromObject(), wire->fromTerminal(), &from) ||
            !findTerminal(wire->toObject(), wire->toTerminal(), &to)) {
            if (errorMessage) {
                *errorMessage = "A wire refers to a terminal that no longer exists.";
            }
            return false;
        }
    }

    std::vector<std::size_t> generatorPositive;
    std::vector<std::size_t> generatorNegative;
    std::vector<std::size_t> referenceNodes;
    std::vector<const PowerGenerator*> generators;
    std::vector<GeneratorMode> modes;
    for (const ParentOfObjects* object : objects) {
        if (const auto* generator = dynamic_cast<const PowerGenerator*>(object)) {
            std::size_t positive = 0;
            std::size_t negative = 0;
            findTerminal(generator->id(), 0, &positive);
            findTerminal(generator->id(), 4, &negative);
            generatorPositive.push_back(positive);
            generatorNegative.push_back(negative);
            referenceNodes.push_back(negative);
            generators.push_back(generator);
            modes.push_back(GeneratorMode::VoltageSource);
        }
    }

    std::vector<double> voltages(nodeCount, 0.0);
    for (ParentOfObjects* object : objects) {
        if (auto* separator = dynamic_cast<WireSeparator*>(object)) {
            for (int terminal = 1; terminal < separator->terminalCount();
                 ++terminal) {
                separator->setOutputElectricalState(terminal, 0.0, 0.0);
            }
        }
    }
    const std::size_t maximumIterations =
        std::max<std::size_t>(8, generators.size() * 4 + 4);
    bool converged = false;

    for (std::size_t iteration = 0; iteration < maximumIterations; ++iteration) {
        std::vector<std::vector<double>> matrix(
            nodeCount, std::vector<double>(nodeCount, 0.0));
        std::vector<double> injections(nodeCount, 0.0);

        for (std::size_t i = 0; i < nodeCount; ++i) {
            matrix[i][i] += 1.0e-12;
        }

        for (const ParentOfObjects* object : objects) {
            if (const auto* generator =
                    dynamic_cast<const PowerGenerator*>(object)) {
                std::size_t positiveBus = 0;
                std::size_t negativeBus = 0;
                findTerminal(generator->id(), 0, &positiveBus);
                findTerminal(generator->id(), 4, &negativeBus);
                for (int terminal = 1; terminal < 4; ++terminal) {
                    std::size_t output = 0;
                    findTerminal(generator->id(), terminal, &output);
                    addConductance(matrix, positiveBus, output, 10000.0);
                }
                for (int terminal = 5; terminal < 8; ++terminal) {
                    std::size_t input = 0;
                    findTerminal(generator->id(), terminal, &input);
                    addConductance(matrix, negativeBus, input, 10000.0);
                }
            } else if (const auto* house = dynamic_cast<const House*>(object)) {
                std::size_t positive = 0;
                std::size_t negative = 0;
                findTerminal(house->id(), 0, &positive);
                findTerminal(house->id(), 1, &negative);
                addConductance(matrix, positive, negative, house->loadConductance());
            } else if (const auto* separator =
                           dynamic_cast<const WireSeparator*>(object)) {
                if (separator->isClosed()) {
                    std::size_t input = 0;
                    findTerminal(separator->id(), 0, &input);
                    for (int terminal = 1; terminal < separator->terminalCount(); ++terminal) {
                        std::size_t output = 0;
                        findTerminal(separator->id(), terminal, &output);
                        addConductance(matrix, input, output,
                                       separatorConductanceSiemens);
                    }
                }
            } else if (dynamic_cast<const UtilityPole*>(object)) {
                std::size_t firstTerminal = 0;
                std::size_t secondTerminal = 0;
                findTerminal(object->id(), 0, &firstTerminal);
                findTerminal(object->id(), 1, &secondTerminal);
                addConductance(matrix, firstTerminal, secondTerminal, 10000.0);
            }
        }

        for (const auto& wire : connections) {
            std::size_t from = 0;
            std::size_t to = 0;
            findTerminal(wire->fromObject(), wire->fromTerminal(), &from);
            findTerminal(wire->toObject(), wire->toTerminal(), &to);
            const double resistance = standardConditions
                ? wire->resistanceAtTemperature(20.0) : wire->resistance();
            addConductance(matrix, from, to, 1.0 / resistance);
        }

        for (std::size_t i = 0; i < generators.size(); ++i) {
            const std::size_t positive = generatorPositive[i];
            const std::size_t negative = generatorNegative[i];
            if (modes[i] == GeneratorMode::VoltageSource) {
                const double conductance = 1.0 / generators[i]->internalResistance();
                addConductance(matrix, positive, negative, conductance);
                injections[positive] += generators[i]->outputVoltage() * conductance;
                injections[negative] -= generators[i]->outputVoltage() * conductance;
            } else if (modes[i] == GeneratorMode::CurrentLimited) {
                injections[positive] += generators[i]->currentLimit();
                injections[negative] -= generators[i]->currentLimit();
            }
        }

        if (!solveLinearSystem(matrix, injections, referenceNodes)) {
            if (errorMessage) {
                *errorMessage = "The electrical network could not be solved.";
            }
            return false;
        }
        voltages = std::move(injections);

        bool modesChanged = false;
        for (std::size_t i = 0; i < generators.size(); ++i) {
            const double terminalVoltage =
                voltages[generatorPositive[i]] - voltages[generatorNegative[i]];
            const double sourceCurrent =
                (generators[i]->outputVoltage() - terminalVoltage) /
                generators[i]->internalResistance();
            GeneratorMode nextMode = modes[i];
            if (sourceCurrent < -1.0e-8) {
                nextMode = GeneratorMode::Off;
            } else if (sourceCurrent > generators[i]->currentLimit() + 1.0e-8) {
                nextMode = GeneratorMode::CurrentLimited;
            } else if (modes[i] != GeneratorMode::VoltageSource &&
                       sourceCurrent >= -1.0e-8 &&
                       sourceCurrent <= generators[i]->currentLimit() + 1.0e-8) {
                nextMode = GeneratorMode::VoltageSource;
            }
            if (nextMode != modes[i]) {
                modes[i] = nextMode;
                modesChanged = true;
            }
        }
        if (!modesChanged) {
            converged = true;
            break;
        }
    }

    if (!converged) {
        if (errorMessage) {
            *errorMessage = "The generator current limits did not converge.";
        }
        return false;
    }

    for (ParentOfObjects* object : objects) {
        std::size_t positive = 0;
        findTerminal(object->id(), 0, &positive);
        double objectVoltage = voltages[positive];
        if (dynamic_cast<const PowerGenerator*>(object)) {
            std::size_t negative = 0;
            findTerminal(object->id(), 4, &negative);
            objectVoltage -= voltages[negative];
        } else if (const auto* house = dynamic_cast<const House*>(object)) {
            std::size_t negative = 0;
            findTerminal(house->id(), 1, &negative);
            objectVoltage -= voltages[negative];
        } else if (object->terminalCount() == 2) {
            std::size_t negative = 0;
            findTerminal(object->id(), 1, &negative);
            objectVoltage -= voltages[negative];
        }
        object->setVoltage(objectVoltage);

        if (const auto* house = dynamic_cast<const House*>(object)) {
            object->setCurrent(objectVoltage * house->loadConductance());
        } else if (const auto* generator = dynamic_cast<const PowerGenerator*>(object)) {
            const double sourceCurrent =
                (generator->outputVoltage() - objectVoltage) /
                generator->internalResistance();
            object->setCurrent(
                std::clamp(sourceCurrent, 0.0, generator->currentLimit()));
        } else if (auto* separator = dynamic_cast<WireSeparator*>(object)) {
            double switchCurrent = 0.0;
            if (separator->isClosed()) {
                std::size_t input = 0;
                findTerminal(separator->id(), 0, &input);
                for (int terminal = 1; terminal < separator->terminalCount(); ++terminal) {
                    std::size_t output = 0;
                    findTerminal(separator->id(), terminal, &output);
                    const double outputCurrent =
                        (voltages[input] - voltages[output]) *
                        separatorConductanceSiemens;
                    switchCurrent += outputCurrent;
                    separator->setOutputElectricalState(
                        terminal, voltages[output], outputCurrent);
                }
            }
            object->setCurrent(switchCurrent);
        }
    }

    for (auto& wire : connections) {
        std::size_t from = 0;
        std::size_t to = 0;
        findTerminal(wire->fromObject(), wire->fromTerminal(), &from);
        findTerminal(wire->toObject(), wire->toTerminal(), &to);
        const double voltageDrop = voltages[from] - voltages[to];
        const double resistance = standardConditions
            ? wire->resistanceAtTemperature(20.0) : wire->resistance();
        wire->setElectricalState(voltageDrop / resistance, voltageDrop);
    }
    return true;
}
