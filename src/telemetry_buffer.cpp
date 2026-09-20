#include "telemetry_buffer.h"

bool TelemetryBuffer::push(const TelemetryRecord &record) {
    if (full()) {
        head_ = (head_ + 1U) % config::telemetryBufferCapacity;
        --count_;
        ++droppedCount_;
    }
    records_[tail_] = record;
    tail_ = (tail_ + 1U) % config::telemetryBufferCapacity;
    ++count_;
    return true;
}

bool TelemetryBuffer::peek(TelemetryRecord &record) const {
    if (empty()) return false;
    record = records_[head_];
    return true;
}

bool TelemetryBuffer::pop(TelemetryRecord &record) {
    if (!peek(record)) return false;
    return discardOldest();
}

bool TelemetryBuffer::discardOldest() {
    if (empty()) return false;
    head_ = (head_ + 1U) % config::telemetryBufferCapacity;
    --count_;
    return true;
}

