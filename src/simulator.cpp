#include "simulator.h"
#include "config.h"

EnergyData EnergySimulator::sample(uint32_t elapsedMs, float accumulatedKWh, float tariff,
                                   SimulationScenario scenario) const {
    EnergyData data;
    if (scenario == SimulationScenario::INVALID_READING) {
        data.voltage = 0.0f;
        data.current = 0.0f;
        data.power = 0.0f;
        data.valid = false;
    } else if (scenario == SimulationScenario::NO_LOAD) {
        data.voltage = 230.0f;
        data.current = 0.0f;
        data.power = 0.0f;
        data.valid = true;
    } else if (scenario == SimulationScenario::OVERCURRENT) {
        data.voltage = 230.0f;
        data.current = config::overcurrentFaultA + 1.0f;
        data.power = data.voltage * data.current;
        data.valid = true;
    } else if (scenario == SimulationScenario::LOW_VOLTAGE) {
        data.voltage = config::voltageWarningLow - 10.0f;
        data.current = 2.0f;
        data.power = data.voltage * data.current;
        data.valid = true;
    } else {
        data.voltage = 230.0f;
        data.current = 2.0f;
        data.power = 460.0f;
        data.valid = true;
    }
    data.energyKWh = accumulatedKWh + integrateKWh(data.power, elapsedMs);
    data.cost = calculateCost(data.energyKWh, tariff);
    return data;
}

