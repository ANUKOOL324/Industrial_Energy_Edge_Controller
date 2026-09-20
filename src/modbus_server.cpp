#include "modbus_server.h"
#include "config.h"
#include "modbus_codec.h"
#include <cmath>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <WiFi.h>
#endif

namespace {
uint16_t stateValue(SystemState state) { return static_cast<uint16_t>(state); }
uint32_t energyWh(float kWh) {
    if (!std::isfinite(kWh) || kWh <= 0.0f) return 0;
    const float wh = kWh * 1000.0f;
    return wh >= 4294967295.0f ? 4294967295UL : static_cast<uint32_t>(wh + 0.5f);
}
}

ModbusServer::ModbusServer()
#ifndef UNIT_TEST
    : server_(config::modbusPort)
#endif
{}

bool ModbusServer::begin() {
#ifndef UNIT_TEST
    server_.begin();
#endif
    return true;
}

void ModbusServer::update(const EnergyData &data, const RuntimeDiagnostics &diagnostics) {
    data_ = data;
    diagnostics_ = diagnostics;
}

void ModbusServer::tick() {
#ifndef UNIT_TEST
    if (!client_ || !client_.connected()) {
        WiFiClient incoming = server_.available();
        if (incoming) client_ = incoming;
    }
    if (!client_ || client_.available() < 12) return;

    uint8_t frame[12];
    const size_t received = client_.read(frame, sizeof(frame));
    ModbusReadRequest request;
    ModbusError error = ModbusError::NONE;
    ++requestCount_;
    uint8_t response[64];
    size_t responseSize = 0;
    if (!parseReadRequest(frame, received, config::modbusMaxRegisters, request, error)) {
        ++errorCount_;
        responseSize = buildExceptionResponse(request, error, response, sizeof(response));
    } else {
        uint16_t registers[16] = {};
        registers[0] = encodeVoltage(data_.voltage);
        registers[1] = encodeCurrent(data_.current);
        registers[2] = encodePower(data_.power);
        splitUint32(energyWh(data_.energyKWh), registers[3], registers[4]);
        registers[5] = stateValue(diagnostics_.currentState);
        registers[6] = static_cast<uint16_t>(diagnostics_.activeFault);
        registers[7] = static_cast<uint16_t>(diagnostics_.freeHeap / 1024U);
        splitUint32(diagnostics_.uptimeSeconds, registers[8], registers[9]);
        splitUint32(data_.cost <= 0.0f ? 0U : static_cast<uint32_t>(data_.cost * 100.0f), registers[10], registers[11]);
        responseSize = buildReadResponse(request, registers + request.startAddress, request.quantity,
                                         response, sizeof(response));
        if (responseSize == 0) ++errorCount_;
    }
    if (responseSize > 0) client_.write(response, responseSize);
#endif
}
