#include "energy_logic.h"
#include <cmath>

float integrateKWh(float powerWatts, uint32_t elapsedMs) {
    if (!std::isfinite(powerWatts) || powerWatts <= 0.0f || elapsedMs == 0) return 0.0f;
    return (powerWatts * (static_cast<float>(elapsedMs) / 3600000.0f)) / 1000.0f;
}

float calculateCost(float energyKWh, float tariff) {
    if (!std::isfinite(energyKWh) || !std::isfinite(tariff) || energyKWh < 0.0f || tariff < 0.0f) return 0.0f;
    return energyKWh * tariff;
}

EnergySample sanitizeSample(EnergySample sample, float voltageCutoff, float currentCutoff, float powerCutoff) {
    sample.voltage = std::isfinite(sample.voltage) && sample.voltage > voltageCutoff ? sample.voltage : 0.0f;
    sample.current = std::isfinite(sample.current) && sample.current > currentCutoff ? sample.current : 0.0f;
    sample.power = std::isfinite(sample.power) && sample.power > powerCutoff ? std::fabs(sample.power) : 0.0f;
    sample.valid = sample.voltage > 0.0f || sample.current > 0.0f || sample.power > 0.0f;
    return sample;
}

