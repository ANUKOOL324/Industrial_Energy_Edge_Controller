#pragma once

#include <stdint.h>
#include <stddef.h>

namespace config {
enum class InputMode : uint8_t { REAL_SENSOR, SIMULATION };

constexpr uint8_t voltagePin = 35;
constexpr uint8_t currentPin = 34;
constexpr float voltageCalibration = 162.7f;
constexpr float currentCalibration = 1.80f;
constexpr float voltageCutoff = 50.0f;
constexpr float currentCutoff = 0.30f;
constexpr float powerCutoff = 5.0f;
constexpr float costPerKWh = 7.5f;
constexpr InputMode inputMode = InputMode::REAL_SENSOR;
constexpr uint32_t energySamplePeriodMs = 2000;
constexpr uint32_t storagePeriodMs = 60000;
constexpr uint32_t diagnosticsPeriodMs = 30000;
constexpr uint32_t wifiRetryIntervalMs = 10000;
constexpr char mqttBroker[] = "192.168.1.10";
constexpr uint16_t mqttPort = 1883;
constexpr char mqttDeviceId[] = "meter01";
constexpr uint32_t mqttPublishPeriodMs = 2000;
constexpr uint32_t mqttReconnectIntervalMs = 10000;
constexpr uint16_t telemetryBufferCapacity = 64;
constexpr uint8_t mqttDrainPerLoop = 2;
constexpr size_t mqttPayloadBufferSize = 512;
constexpr char otaManifestUrl[] = ""; // Configure locally only after an authenticated update service exists.
constexpr uint32_t otaCheckPeriodMs = 3600000;
constexpr uint32_t otaHealthyPeriodMs = 30000;
constexpr size_t otaMinimumFreeHeap = 20000;
constexpr uint16_t modbusPort = 502;
constexpr uint16_t modbusMaxRegisters = 16;
constexpr float overcurrentWarningA = 8.0f; // Demonstration threshold; validate for the installed CT.
constexpr float overcurrentFaultA = 12.0f; // Demonstration threshold; validate for the installed CT.
constexpr float voltageWarningLow = 200.0f; // Demonstration threshold for a nominal 230 V installation.
constexpr float voltageWarningHigh = 250.0f;
constexpr uint32_t faultRecoveryTimeMs = 10000;
constexpr uint32_t faultDebounceMs = 2000;
constexpr char nvsNamespace[] = "energy";
constexpr uint16_t energyTaskStackWords = 4096;
constexpr uint16_t faultTaskStackWords = 3072;
constexpr uint16_t storageTaskStackWords = 3072;
constexpr uint16_t networkTaskStackWords = 6144;
constexpr uint16_t diagnosticsTaskStackWords = 4096;
constexpr uint8_t energyTaskPriority = 3;
constexpr uint8_t faultTaskPriority = 2;
constexpr uint8_t storageTaskPriority = 1;
constexpr uint8_t networkTaskPriority = 2;
constexpr uint8_t diagnosticsTaskPriority = 1;
}
