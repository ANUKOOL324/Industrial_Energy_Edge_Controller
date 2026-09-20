#pragma once

#include <stdint.h>

struct EnergySample {
    float voltage = 0.0f;
    float current = 0.0f;
    float power = 0.0f;
    float energyKWh = 0.0f;
    float cost = 0.0f;
    bool valid = false;
};

float integrateKWh(float powerWatts, uint32_t elapsedMs);
float calculateCost(float energyKWh, float tariff);
EnergySample sanitizeSample(EnergySample sample, float voltageCutoff, float currentCutoff, float powerCutoff);

