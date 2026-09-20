#pragma once

#include <stdint.h>
#include "diagnostics.h"
#include "telemetry_buffer.h"
#include "ota_types.h"

class MqttService {
public:
    MqttService();
    void begin();
    void tick(uint32_t nowMs);
    bool submitTelemetry(const TelemetryRecord &record);
    bool publishFault(const FaultEvent &event);
    bool publishDiagnostics(const RuntimeDiagnostics &diagnostics);
    bool publishOtaStatus(OtaState state, OtaError error, const char *currentVersion, const char *targetVersion);
    bool connected() const;
    uint32_t reconnectCount() const { return reconnectCount_; }
    size_t offlineBufferCount() const { return buffer_.size(); }
    uint32_t telemetryDroppedCount() const { return buffer_.droppedCount(); }

private:
    bool publishTelemetryRecord(const TelemetryRecord &record);
    TelemetryBuffer buffer_;
    uint32_t reconnectCount_ = 0;
    uint32_t lastAttemptMs_ = 0;
#ifndef UNIT_TEST
    class WiFiClientHolder;
    WiFiClientHolder *clientHolder_ = nullptr;
#endif
};
