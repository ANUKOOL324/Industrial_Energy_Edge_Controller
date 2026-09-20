#include "energy_store.h"
#include "config.h"

#ifndef UNIT_TEST
#include <Preferences.h>
static Preferences preferences;
#endif

bool EnergyStore::begin() {
#ifndef UNIT_TEST
    return preferences.begin("energy", false);
#else
    return true;
#endif
}

void EnergyStore::load(float &energyKWh, float &cost, float &tariff) {
#ifndef UNIT_TEST
    energyKWh = preferences.getFloat("kwh", 0.0f);
    cost = preferences.getFloat("cost", 0.0f);
    tariff = preferences.getFloat("tariff", config::costPerKWh);
#else
    energyKWh = 0.0f;
    cost = 0.0f;
    tariff = config::costPerKWh;
#endif
}

bool EnergyStore::save(const EnergySample &sample, float tariff) {
#ifndef UNIT_TEST
    return preferences.putFloat("kwh", sample.energyKWh) > 0 &&
           preferences.putFloat("cost", sample.cost) > 0 &&
           preferences.putFloat("tariff", tariff) > 0;
#else
    (void)sample;
    (void)tariff;
    return true;
#endif
}

