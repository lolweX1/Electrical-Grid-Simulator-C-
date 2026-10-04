#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP

#include "Wire.hpp"
#include "../Objects/ParentOfObjects.hpp"

#include <memory>
#include <QString>
#include <vector>

class Connections {
public:
    bool addWire(int id, const ParentOfObjects& fromObject, int fromTerminal,
                 const ParentOfObjects& toObject, int toTerminal,
                 double lengthMeters, bool shortCircuit,
                 QString* errorMessage = nullptr);
    void removeObject(int objectId);
    void removeWire(int wireId);
    int terminalConnectionCount(int objectId, int terminal) const;
    bool simulate(const std::vector<std::unique_ptr<ParentOfObjects>>& objects,
                  double elapsedSeconds, QString* errorMessage = nullptr,
                  bool simulationRunning = true);

    const std::vector<std::unique_ptr<Wire>>& wires() const { return connections; }

private:
    bool solve(const std::vector<ParentOfObjects*>& objects, QString* errorMessage,
               bool standardConditions);
    std::vector<std::unique_ptr<Wire>> connections;
};

#endif
