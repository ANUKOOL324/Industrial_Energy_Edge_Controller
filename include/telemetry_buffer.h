#pragma once

#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "energy_logic.h"
#include "fault_manager.h"

struct TelemetryRecord {
    uint32_t timestampMs = 0;
    EnergyData energy;
    SystemState state = SystemState::NORMAL;
    FaultCode fault = FaultCode::NONE;
};

class TelemetryBuffer {
public:
    bool push(const TelemetryRecord &record);
    bool peek(TelemetryRecord &record) const;
    bool pop(TelemetryRecord &record);
    bool discardOldest();
    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == config::telemetryBufferCapacity; }
    uint32_t droppedCount() const { return droppedCount_; }

private:
    TelemetryRecord records_[config::telemetryBufferCapacity] = {};
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t count_ = 0;
    uint32_t droppedCount_ = 0;
};

