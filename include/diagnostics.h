#pragma once

#include <stddef.h>
#include <stdint.h>
#include "fault_manager.h"
#include "ota_types.h"

struct RuntimeDiagnostics {
    char firmwareVersion[16] = {};
    uint32_t uptimeSeconds = 0;
    size_t freeHeap = 0;
    size_t minimumFreeHeap = 0;
    uint32_t wifiReconnectCount = 0;
    uint32_t networkFailureCount = 0;
    uint32_t mqttReconnectCount = 0;
    uint32_t sensorErrorCount = 0;
    uint32_t faultCount = 0;
    uint32_t modbusRequestCount = 0;
    uint32_t modbusErrorCount = 0;
    uint32_t offlineBufferCount = 0;
    uint32_t telemetryDroppedCount = 0;
    OtaState otaState = OtaState::IDLE;
    OtaError otaError = OtaError::NONE;
    uint32_t otaAttemptCount = 0;
    uint32_t otaSuccessCount = 0;
    uint32_t otaFailureCount = 0;
    uint32_t lastOtaAttemptMs = 0;
    uint32_t energyTaskStackWatermark = 0;
    uint32_t faultTaskStackWatermark = 0;
    uint32_t networkTaskStackWatermark = 0;
    SystemState currentState = SystemState::NORMAL;
    FaultCode activeFault = FaultCode::NONE;
};

class Diagnostics {
public:
    void begin();
    void update(const RuntimeDiagnostics &source);
    RuntimeDiagnostics snapshot() const { return metrics_; }
    void print() const;

private:
    RuntimeDiagnostics metrics_;
};
