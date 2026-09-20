#pragma once

#include "energy_logic.h"

#ifndef UNIT_TEST
#include <EmonLib.h>
#endif

class EnergyMeter {
public:
    void begin();
    EnergySample read(uint32_t elapsedMs, float energyKWh, float tariff);

private:
#ifndef UNIT_TEST
    EnergyMonitor emon_;
#endif
};

