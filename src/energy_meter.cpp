#include "energy_meter.h"
#include "config.h"

void EnergyMeter::begin() {
#ifndef UNIT_TEST
    analogReadResolution(12);
    emon_.voltage(config::voltagePin, config::voltageCalibration, 1.7f);
    emon_.current(config::currentPin, config::currentCalibration);
#endif
}

EnergySample EnergyMeter::read(uint32_t elapsedMs, float energyKWh, float tariff) {
    EnergySample sample;
#ifdef UNIT_TEST
    (void)elapsedMs;
    sample.energyKWh = energyKWh;
    sample.cost = calculateCost(energyKWh, tariff);
    return sample;
#else
    emon_.calcVI(20, 2000);
    sample.voltage = emon_.Vrms;
    sample.current = emon_.Irms;
    sample.power = emon_.realPower;
    sample = sanitizeSample(sample, config::voltageCutoff, config::currentCutoff, config::powerCutoff);
    sample.energyKWh = energyKWh + integrateKWh(sample.power, elapsedMs);
    sample.cost = calculateCost(sample.energyKWh, tariff);
    return sample;
#endif
}

