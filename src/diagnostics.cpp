#include "diagnostics.h"
#include "version.h"
#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

void Diagnostics::begin() {
    strncpy(metrics_.firmwareVersion, firmwareVersionString(), sizeof(metrics_.firmwareVersion) - 1);
#ifndef UNIT_TEST
    metrics_.freeHeap = ESP.getFreeHeap();
    metrics_.minimumFreeHeap = ESP.getMinFreeHeap();
#endif
}

void Diagnostics::update(const RuntimeDiagnostics &source) {
    metrics_ = source;
#ifndef UNIT_TEST
    metrics_.uptimeSeconds = millis() / 1000U;
    metrics_.freeHeap = ESP.getFreeHeap();
    metrics_.minimumFreeHeap = ESP.getMinFreeHeap();
#endif
}

void Diagnostics::print() const {
#ifndef UNIT_TEST
    Serial.println("===== SYSTEM HEALTH =====");
    Serial.printf("Uptime: %lu s\nFree Heap: %u bytes\nMinimum Free Heap: %u bytes\n",
                  static_cast<unsigned long>(metrics_.uptimeSeconds),
                  static_cast<unsigned>(metrics_.freeHeap),
                  static_cast<unsigned>(metrics_.minimumFreeHeap));
    Serial.printf("WiFi Reconnects: %lu\nNetwork Failures: %lu\n",
                  static_cast<unsigned long>(metrics_.wifiReconnectCount),
                  static_cast<unsigned long>(metrics_.networkFailureCount));
    Serial.printf("Modbus Requests: %lu\nModbus Errors: %lu\nSensor Errors: %lu\nFault Count: %lu\n",
                  static_cast<unsigned long>(metrics_.modbusRequestCount),
                  static_cast<unsigned long>(metrics_.modbusErrorCount),
                  static_cast<unsigned long>(metrics_.sensorErrorCount),
                  static_cast<unsigned long>(metrics_.faultCount));
    Serial.printf("Firmware: %s\nState: %u Fault: %u\nTask Stack HWM (words): Energy=%lu Fault=%lu Network=%lu\n",
                  metrics_.firmwareVersion,
                  static_cast<unsigned>(metrics_.currentState),
                  static_cast<unsigned>(metrics_.activeFault),
                  static_cast<unsigned long>(metrics_.energyTaskStackWatermark),
                  static_cast<unsigned long>(metrics_.faultTaskStackWatermark),
                  static_cast<unsigned long>(metrics_.networkTaskStackWatermark));
    Serial.printf("OTA: %s Error: %s Attempts=%lu Successes=%lu Failures=%lu\n",
                  otaStateName(metrics_.otaState), otaErrorName(metrics_.otaError),
                  static_cast<unsigned long>(metrics_.otaAttemptCount),
                  static_cast<unsigned long>(metrics_.otaSuccessCount),
                  static_cast<unsigned long>(metrics_.otaFailureCount));
#endif
}
