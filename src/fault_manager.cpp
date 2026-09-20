#include "fault_manager.h"
#include "config.h"

namespace {
FaultCode detectFault(const EnergyData &data) {
    if (!data.valid) return FaultCode::INVALID_SENSOR_READING;
    if (data.current >= config::overcurrentWarningA) return FaultCode::OVERCURRENT;
    if (data.voltage < config::voltageCutoff || data.voltage < config::voltageWarningLow ||
        data.voltage > config::voltageWarningHigh) return FaultCode::ABNORMAL_VOLTAGE;
    return FaultCode::NONE;
}
}

FaultEvent FaultManager::evaluate(const EnergyData &data, uint32_t nowMs) {
    const FaultCode detected = detectFault(data);
    const bool serious = detected != FaultCode::NONE &&
                         (detected == FaultCode::OVERCURRENT || !data.valid ||
                          (data.current >= config::overcurrentFaultA));

    if (detected != FaultCode::NONE) {
        healthyActive_ = false;
        healthySinceMs_ = 0;
        if (!abnormalActive_) {
            abnormalSinceMs_ = nowMs;
            abnormalActive_ = true;
        }
        if (serious && nowMs - abnormalSinceMs_ >= config::faultDebounceMs) {
            if (state_ != SystemState::FAULT) ++faultCount_;
            state_ = SystemState::FAULT;
            activeFault_ = detected;
        } else if (state_ == SystemState::NORMAL || state_ == SystemState::RECOVERY) {
            state_ = SystemState::WARNING;
            activeFault_ = detected;
        }
    } else {
        abnormalActive_ = false;
        abnormalSinceMs_ = 0;
        if (state_ == SystemState::FAULT || state_ == SystemState::WARNING) {
            state_ = SystemState::RECOVERY;
            activeFault_ = FaultCode::NONE;
            healthySinceMs_ = nowMs;
            healthyActive_ = true;
        } else if (state_ == SystemState::RECOVERY &&
                   healthyActive_ && nowMs - healthySinceMs_ >= config::faultRecoveryTimeMs) {
            state_ = SystemState::NORMAL;
            healthyActive_ = false;
        }
    }

    FaultEvent event;
    event.code = activeFault_;
    event.state = state_;
    event.timestampMs = nowMs;
    event.voltage = data.voltage;
    event.current = data.current;
    event.realPower = data.power;
    return event;
}
