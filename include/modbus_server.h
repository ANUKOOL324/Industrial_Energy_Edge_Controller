#pragma once

#include "diagnostics.h"
#include "energy_logic.h"

#ifndef UNIT_TEST
#include <WiFi.h>
#endif

class ModbusServer {
public:
    ModbusServer();
    bool begin();
    void update(const EnergyData &data, const RuntimeDiagnostics &diagnostics);
    void tick();
    uint32_t requestCount() const { return requestCount_; }
    uint32_t errorCount() const { return errorCount_; }

private:
    EnergyData data_;
    RuntimeDiagnostics diagnostics_;
    uint32_t requestCount_ = 0;
    uint32_t errorCount_ = 0;
#ifndef UNIT_TEST
    WiFiServer server_;
    WiFiClient client_;
#endif
};
