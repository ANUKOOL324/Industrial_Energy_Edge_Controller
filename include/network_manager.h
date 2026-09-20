#pragma once

#include <stdint.h>
#include "energy_logic.h"

struct NetworkStatus {
    bool connected = false;
    uint32_t reconnectCount = 0;
    uint32_t failureCount = 0;
    uint32_t lastAttemptMs = 0;
};

class NetworkManager {
public:
    void begin();
    void tick(uint32_t nowMs);
    void publish(const EnergyData &data);
    NetworkStatus status() const { return status_; }

private:
    NetworkStatus status_;
};
