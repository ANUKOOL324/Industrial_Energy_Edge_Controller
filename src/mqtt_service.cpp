#include "mqtt_service.h"
#include "config.h"
#include <cstdio>

#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

class MqttService::WiFiClientHolder {
public:
    WiFiClient wifi;
    PubSubClient mqtt;
    WiFiClientHolder() : mqtt(wifi) {}
};
#endif

namespace {
const char *stateName(SystemState state) {
    switch (state) {
        case SystemState::WARNING: return "WARNING";
        case SystemState::FAULT: return "FAULT";
        case SystemState::RECOVERY: return "RECOVERY";
        default: return "NORMAL";
    }
}

const char *faultName(FaultCode fault) {
    switch (fault) {
        case FaultCode::INVALID_SENSOR_READING: return "INVALID_SENSOR_READING";
        case FaultCode::SENSOR_TIMEOUT: return "SENSOR_TIMEOUT";
        case FaultCode::OVERCURRENT: return "OVERCURRENT";
        case FaultCode::ABNORMAL_VOLTAGE: return "ABNORMAL_VOLTAGE";
        case FaultCode::NETWORK_DISCONNECTED: return "NETWORK_DISCONNECTED";
        case FaultCode::LOW_MEMORY: return "LOW_MEMORY";
        default: return "NONE";
    }
}

void topic(char *destination, size_t capacity, const char *suffix) {
    snprintf(destination, capacity, "industrial-energy/%s/%s", config::mqttDeviceId, suffix);
}
}

MqttService::MqttService()
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    : clientHolder_(new WiFiClientHolder())
#endif
{}

void MqttService::begin() {
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    clientHolder_->mqtt.setServer(config::mqttBroker, config::mqttPort);
#endif
}

void MqttService::tick(uint32_t nowMs) {
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    if (WiFi.status() != WL_CONNECTED) return;
    if (!clientHolder_->mqtt.connected()) {
        if (nowMs - lastAttemptMs_ < config::mqttReconnectIntervalMs) return;
        lastAttemptMs_ = nowMs;
        ++reconnectCount_;
        char statusTopic[128];
        topic(statusTopic, sizeof(statusTopic), "status");
        if (!clientHolder_->mqtt.connect(config::mqttDeviceId, MQTT_USERNAME, MQTT_PASSWORD,
                                         statusTopic, 0, true, "offline")) return;
        clientHolder_->mqtt.publish(statusTopic, "online", true);
    } else {
        clientHolder_->mqtt.loop();
    }

    for (uint8_t sent = 0; sent < config::mqttDrainPerLoop; ++sent) {
        TelemetryRecord record;
        if (!buffer_.peek(record) || !publishTelemetryRecord(record)) break;
        buffer_.discardOldest();
    }
#else
    (void)nowMs;
#endif
}

bool MqttService::connected() const {
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    return clientHolder_->mqtt.connected();
#else
    return false;
#endif
}

bool MqttService::submitTelemetry(const TelemetryRecord &record) {
    if (connected() && publishTelemetryRecord(record)) return true;
    return buffer_.push(record);
}

bool MqttService::publishTelemetryRecord(const TelemetryRecord &record) {
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    if (!clientHolder_->mqtt.connected()) return false;
    char destination[128];
    char payload[config::mqttPayloadBufferSize];
    topic(destination, sizeof(destination), "telemetry");
    const int length = snprintf(payload, sizeof(payload),
        "{\"voltage\":%.2f,\"current\":%.3f,\"power\":%.2f,\"energy_kwh\":%.5f,\"cost\":%.2f,\"state\":\"%s\",\"uptime_s\":%lu}",
        record.energy.voltage, record.energy.current, record.energy.power,
        record.energy.energyKWh, record.energy.cost, stateName(record.state),
        static_cast<unsigned long>(record.timestampMs / 1000U));
    return length > 0 && static_cast<size_t>(length) < sizeof(payload) &&
           clientHolder_->mqtt.publish(destination, payload);
#else
    (void)record;
    return false;
#endif
}

bool MqttService::publishFault(const FaultEvent &event) {
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    if (!clientHolder_->mqtt.connected()) return false;
    char destination[128];
    char payload[config::mqttPayloadBufferSize];
    topic(destination, sizeof(destination), "fault");
    const int length = snprintf(payload, sizeof(payload),
        "{\"fault\":\"%s\",\"state\":\"%s\",\"voltage\":%.2f,\"current\":%.3f,\"power\":%.2f,\"timestamp_ms\":%lu}",
        faultName(event.code), stateName(event.state), event.voltage, event.current,
        event.realPower, static_cast<unsigned long>(event.timestampMs));
    return length > 0 && static_cast<size_t>(length) < sizeof(payload) &&
           clientHolder_->mqtt.publish(destination, payload);
#else
    (void)event;
    return false;
#endif
}

bool MqttService::publishDiagnostics(const RuntimeDiagnostics &diagnostics) {
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    if (!clientHolder_->mqtt.connected()) return false;
    char destination[128];
    char payload[config::mqttPayloadBufferSize];
    topic(destination, sizeof(destination), "diagnostics");
    const int length = snprintf(payload, sizeof(payload),
        "{\"firmware_version\":\"%s\",\"ota_state\":\"%s\",\"ota_error\":\"%s\",\"ota_attempts\":%lu,\"ota_successes\":%lu,\"ota_failures\":%lu,\"uptime_s\":%lu,\"free_heap\":%u,\"min_free_heap\":%u,\"wifi_reconnects\":%lu,\"mqtt_reconnects\":%lu,\"modbus_requests\":%lu,\"fault_count\":%lu,\"offline_buffer_count\":%u,\"telemetry_dropped\":%lu}",
        diagnostics.firmwareVersion, otaStateName(diagnostics.otaState), otaErrorName(diagnostics.otaError),
        static_cast<unsigned long>(diagnostics.otaAttemptCount), static_cast<unsigned long>(diagnostics.otaSuccessCount),
        static_cast<unsigned long>(diagnostics.otaFailureCount), static_cast<unsigned long>(diagnostics.uptimeSeconds),
        static_cast<unsigned>(diagnostics.freeHeap),
        static_cast<unsigned>(diagnostics.minimumFreeHeap), static_cast<unsigned long>(diagnostics.wifiReconnectCount),
        static_cast<unsigned long>(diagnostics.mqttReconnectCount), static_cast<unsigned long>(diagnostics.modbusRequestCount),
        static_cast<unsigned long>(diagnostics.faultCount), static_cast<unsigned>(diagnostics.offlineBufferCount),
        static_cast<unsigned long>(diagnostics.telemetryDroppedCount));
    return length > 0 && static_cast<size_t>(length) < sizeof(payload) &&
           clientHolder_->mqtt.publish(destination, payload);
#else
    (void)diagnostics;
    return false;
#endif
}

bool MqttService::publishOtaStatus(OtaState state, OtaError error, const char *currentVersion, const char *targetVersion) {
#if !defined(UNIT_TEST) && IEEC_ENABLE_MQTT
    if (!clientHolder_->mqtt.connected()) return false;
    char destination[128];
    char payload[config::mqttPayloadBufferSize];
    topic(destination, sizeof(destination), "ota");
    const int length = snprintf(payload, sizeof(payload),
        "{\"state\":\"%s\",\"error\":\"%s\",\"current_version\":\"%s\",\"target_version\":\"%s\"}",
        otaStateName(state), otaErrorName(error), currentVersion != nullptr ? currentVersion : "",
        targetVersion != nullptr ? targetVersion : "");
    return length > 0 && static_cast<size_t>(length) < sizeof(payload) &&
           clientHolder_->mqtt.publish(destination, payload);
#else
    (void)state;
    (void)error;
    (void)currentVersion;
    (void)targetVersion;
    return false;
#endif
}
