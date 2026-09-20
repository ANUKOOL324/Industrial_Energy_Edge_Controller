#pragma once

#include "energy_logic.h"

class EnergyStore {
public:
    bool begin();
    void load(float &energyKWh, float &cost, float &tariff);
    bool save(const EnergySample &sample, float tariff);
};

