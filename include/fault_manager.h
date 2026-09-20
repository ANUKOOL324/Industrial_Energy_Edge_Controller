#pragma once

#include <stdint.h>
#include "energy_logic.h"

enum class SystemState : uint8_t { NORMAL, WARNING, FAULT, RECOVERY };
enum class FaultCode : uint8_t {
    NONE = 0,
    INVALID_SENSOR_READING,
    SENSOR_TIMEOUT,
    OVERCURRENT,
    ABNORMAL_VOLTAGE,
    NETWORK_DISCONNECTED,
    LOW_MEMORY
};

struct FaultEvent {
    FaultCode code = FaultCode::NONE;
    SystemState state = SystemState::NORMAL;
    uint32_t timestampMs = 0;
    float voltage = 0.0f;
    float current = 0.0f;
    float realPower = 0.0f;
};

class FaultManager {
public:
    FaultEvent evaluate(const EnergyData &data, uint32_t nowMs);
    SystemState state() const { return state_; }
    FaultCode activeFault() const { return activeFault_; }
    uint32_t faultCount() const { return faultCount_; }

private:
    SystemState state_ = SystemState::NORMAL;
    FaultCode activeFault_ = FaultCode::NONE;
    uint32_t faultCount_ = 0;
    uint32_t abnormalSinceMs_ = 0;
    uint32_t healthySinceMs_ = 0;
    bool abnormalActive_ = false;
    bool healthyActive_ = false;
};
