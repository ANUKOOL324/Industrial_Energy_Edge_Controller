#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <cstring>
#include "config.h"
#include "diagnostics.h"
#include "energy_meter.h"
#include "energy_store.h"
#include "fault_manager.h"
#include "modbus_server.h"
#include "mqtt_service.h"
#include "network_manager.h"
#include "ota_manager.h"
#include "simulator.h"
#include "version.h"

namespace {
EnergyMeter meter;
EnergyStore store;
EnergySimulator simulator;
FaultManager faultManager;
Diagnostics diagnostics;
NetworkManager network;
ModbusServer modbus;
MqttService mqtt;
OtaManager ota;
float tariff = config::costPerKWh;
bool storageReady = false;

QueueHandle_t faultQueue = nullptr;
QueueHandle_t storageQueue = nullptr;
QueueHandle_t networkQueue = nullptr;
SemaphoreHandle_t snapshotMutex = nullptr;
TaskHandle_t energyTaskHandle = nullptr;
TaskHandle_t faultTaskHandle = nullptr;
TaskHandle_t networkTaskHandle = nullptr;

struct SharedSnapshot {
    EnergyData latest;
    FaultEvent fault;
    RuntimeDiagnostics health;
};

SharedSnapshot snapshot;

bool copySnapshot(SharedSnapshot &target) {
    if (snapshotMutex == nullptr || xSemaphoreTake(snapshotMutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;
    target = snapshot;
    xSemaphoreGive(snapshotMutex);
    return true;
}

void updateEnergySnapshot(const EnergyData &data) {
    if (xSemaphoreTake(snapshotMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        snapshot.latest = data;
        xSemaphoreGive(snapshotMutex);
    }
}

void updateFaultSnapshot(const FaultEvent &event) {
    if (xSemaphoreTake(snapshotMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        snapshot.fault = event;
        snapshot.health.currentState = event.state;
        snapshot.health.activeFault = event.code;
        snapshot.health.faultCount = faultManager.faultCount();
        if (event.code == FaultCode::INVALID_SENSOR_READING) ++snapshot.health.sensorErrorCount;
        xSemaphoreGive(snapshotMutex);
    }
}

void energyTask(void *) {
    uint32_t previousMs = millis();
    float energyKWh = snapshot.latest.energyKWh;
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        const uint32_t now = millis();
        const uint32_t elapsed = now - previousMs;
        previousMs = now;
        EnergyData data;
        if (config::inputMode == config::InputMode::SIMULATION) {
            data = simulator.sample(elapsed, energyKWh, tariff,
                                    SimulationScenario::NORMAL_LOAD);
        } else {
            data = meter.read(elapsed, energyKWh, tariff);
        }
        energyKWh = data.energyKWh;
        updateEnergySnapshot(data);
        xQueueOverwrite(faultQueue, &data);
        xQueueOverwrite(storageQueue, &data);
        xQueueOverwrite(networkQueue, &data);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(config::energySamplePeriodMs));
    }
}

void faultTask(void *) {
    EnergyData data;
    for (;;) {
        if (xQueueReceive(faultQueue, &data, portMAX_DELAY) == pdTRUE) {
            updateFaultSnapshot(faultManager.evaluate(data, millis()));
        }
    }
}

void storageTask(void *) {
    EnergyData data = snapshot.latest;
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        xQueueReceive(storageQueue, &data, 0);
        store.save(data, tariff);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(config::storagePeriodMs));
    }
}

void networkTask(void *) {
    network.begin();
    modbus.begin();
    mqtt.begin();
    EnergyData data;
    FaultEvent lastFault;
    bool haveLastFault = false;
    OtaState lastOtaState = OtaState::IDLE;
    OtaError lastOtaError = OtaError::NONE;
    uint32_t lastTelemetryMs = 0;
    uint32_t lastDiagnosticsMs = 0;
    for (;;) {
        const uint32_t now = millis();
        TelemetryRecord telemetry;
        FaultEvent fault;
        bool haveTelemetry = false;
        if (xQueueReceive(networkQueue, &data, pdMS_TO_TICKS(50)) == pdTRUE) {
            SharedSnapshot current;
            if (copySnapshot(current)) {
                modbus.update(data, current.health);
                telemetry = TelemetryRecord{now, data, current.fault.state, current.fault.code};
                fault = current.fault;
                haveTelemetry = true;
            }
            network.publish(data);
        }
        if (haveTelemetry) {
            if (now - lastTelemetryMs >= config::mqttPublishPeriodMs) {
                mqtt.submitTelemetry(telemetry);
                lastTelemetryMs = now;
            }
            const bool faultChanged = !haveLastFault || fault.code != lastFault.code ||
                                      fault.state != lastFault.state;
            if (faultChanged && (haveLastFault || fault.code != FaultCode::NONE)) {
                mqtt.publishFault(fault);
            }
            lastFault = fault;
            haveLastFault = true;
        }
        network.tick(now);
        mqtt.tick(now);
        SharedSnapshot healthSnapshot;
        if (copySnapshot(healthSnapshot)) {
            ota.reportHealth(energyTaskHandle != nullptr, storageReady,
                             healthSnapshot.fault.state == SystemState::FAULT,
                             ESP.getFreeHeap());
        }
        ota.process(now);
        if (ota.state() != lastOtaState || ota.error() != lastOtaError) {
            mqtt.publishOtaStatus(ota.state(), ota.error(), firmwareVersionString(), ota.targetVersion());
            lastOtaState = ota.state();
            lastOtaError = ota.error();
        }
        modbus.tick();
        RuntimeDiagnostics diagnosticsToPublish;
        bool publishDiagnosticsNow = false;
        if (xSemaphoreTake(snapshotMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            snapshot.health.wifiReconnectCount = network.status().reconnectCount;
            snapshot.health.networkFailureCount = network.status().failureCount;
            snapshot.health.mqttReconnectCount = mqtt.reconnectCount();
            snapshot.health.modbusRequestCount = modbus.requestCount();
            snapshot.health.modbusErrorCount = modbus.errorCount();
            snapshot.health.offlineBufferCount = mqtt.offlineBufferCount();
            snapshot.health.telemetryDroppedCount = mqtt.telemetryDroppedCount();
            strncpy(snapshot.health.firmwareVersion, firmwareVersionString(), sizeof(snapshot.health.firmwareVersion) - 1);
            snapshot.health.otaState = ota.state();
            snapshot.health.otaError = ota.error();
            snapshot.health.otaAttemptCount = ota.attemptCount();
            snapshot.health.otaSuccessCount = ota.successCount();
            snapshot.health.otaFailureCount = ota.failureCount();
            snapshot.health.lastOtaAttemptMs = ota.lastAttemptMs();
            if (now - lastDiagnosticsMs >= config::diagnosticsPeriodMs) {
                snapshot.health.uptimeSeconds = now / 1000U;
                diagnosticsToPublish = snapshot.health;
                publishDiagnosticsNow = true;
                lastDiagnosticsMs = now;
            }
            xSemaphoreGive(snapshotMutex);
        }
        if (publishDiagnosticsNow) mqtt.publishDiagnostics(diagnosticsToPublish);
        if (ota.rebootRequired()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP.restart();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void diagnosticsTask(void *) {
    diagnostics.begin();
    for (;;) {
        RuntimeDiagnostics health;
        SharedSnapshot current;
        if (copySnapshot(current)) {
            health = current.health;
            health.energyTaskStackWatermark = uxTaskGetStackHighWaterMark(energyTaskHandle);
            health.faultTaskStackWatermark = uxTaskGetStackHighWaterMark(faultTaskHandle);
            health.networkTaskStackWatermark = uxTaskGetStackHighWaterMark(networkTaskHandle);
            diagnostics.update(health);
            diagnostics.print();
        }
        vTaskDelay(pdMS_TO_TICKS(config::diagnosticsPeriodMs));
    }
}

template <typename Function>
bool createTask(Function function, const char *name, uint16_t stackWords, uint8_t priority,
                TaskHandle_t *handle) {
    return xTaskCreate(function, name, stackWords, nullptr, priority, handle) == pdPASS;
}
}

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.printf("Industrial Energy Edge Controller firmware %s\n", firmwareVersionString());
    storageReady = store.begin();
    store.load(snapshot.latest.energyKWh, snapshot.latest.cost, tariff);
    meter.begin();
    ota.begin();
    strncpy(snapshot.health.firmwareVersion, firmwareVersionString(), sizeof(snapshot.health.firmwareVersion) - 1);

    faultQueue = xQueueCreate(1, sizeof(EnergyData));
    storageQueue = xQueueCreate(1, sizeof(EnergyData));
    networkQueue = xQueueCreate(1, sizeof(EnergyData));
    snapshotMutex = xSemaphoreCreateMutex();
    if (faultQueue == nullptr || storageQueue == nullptr || networkQueue == nullptr || snapshotMutex == nullptr) {
        Serial.println("FATAL: RTOS object creation failed");
        return;
    }

    bool tasksReady = true;
    tasksReady &= createTask(energyTask, "EnergyTask", config::energyTaskStackWords, config::energyTaskPriority, &energyTaskHandle);
    tasksReady &= createTask(faultTask, "FaultTask", config::faultTaskStackWords, config::faultTaskPriority, &faultTaskHandle);
    tasksReady &= createTask(storageTask, "StorageTask", config::storageTaskStackWords, config::storageTaskPriority, nullptr);
    tasksReady &= createTask(networkTask, "NetworkTask", config::networkTaskStackWords, config::networkTaskPriority, &networkTaskHandle);
    tasksReady &= createTask(diagnosticsTask, "DiagnosticsTask", config::diagnosticsTaskStackWords, config::diagnosticsTaskPriority, nullptr);
    if (!tasksReady) Serial.println("FATAL: RTOS task creation failed");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
