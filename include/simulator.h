#pragma once

#include "energy_logic.h"

enum class SimulationScenario : uint8_t { NORMAL_LOAD, NO_LOAD, OVERCURRENT, LOW_VOLTAGE, INVALID_READING };

class EnergySimulator {
public:
    EnergyData sample(uint32_t elapsedMs, float accumulatedKWh, float tariff,
                      SimulationScenario scenario) const;
};

