#pragma once

#include <stdint.h>

namespace config {
constexpr uint8_t voltagePin = 35;
constexpr uint8_t currentPin = 34;
constexpr float voltageCalibration = 162.7f;
constexpr float currentCalibration = 1.80f;
constexpr float voltageCutoff = 50.0f;
constexpr float currentCutoff = 0.30f;
constexpr float powerCutoff = 5.0f;
constexpr float costPerKWh = 7.5f;
constexpr uint32_t samplePeriodMs = 2000;
constexpr uint32_t storageCheckpointMs = 60000;
}

