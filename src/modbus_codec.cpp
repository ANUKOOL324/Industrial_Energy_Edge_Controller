#include "modbus_codec.h"
#include <cmath>
#include <limits>

namespace {
uint16_t scaled(float value, float scale) {
    if (!std::isfinite(value) || value <= 0.0f) return 0;
    const float result = value * scale;
    if (result >= static_cast<float>(std::numeric_limits<uint16_t>::max())) return std::numeric_limits<uint16_t>::max();
    return static_cast<uint16_t>(result + 0.5f);
}

void put16(uint8_t *out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value >> 8);
    out[1] = static_cast<uint8_t>(value & 0xff);
}

uint8_t exceptionCode(ModbusError error) {
    if (error == ModbusError::ILLEGAL_FUNCTION) return 0x01;
    if (error == ModbusError::ILLEGAL_ADDRESS) return 0x02;
    return 0x03;
}
}

bool parseReadRequest(const uint8_t *frame, size_t length, uint16_t maxRegisters,
                      ModbusReadRequest &request, ModbusError &error) {
    error = ModbusError::NONE;
    if (frame == nullptr || length < 12 || frame[2] != 0 || frame[3] != 0) {
        error = ModbusError::MALFORMED_FRAME;
        return false;
    }
    const uint16_t mbapLength = static_cast<uint16_t>((frame[4] << 8) | frame[5]);
    if (mbapLength != 6 || length != static_cast<size_t>(6 + mbapLength) || frame[6] == 0) {
        error = ModbusError::MALFORMED_FRAME;
        return false;
    }
    request.transactionId = static_cast<uint16_t>((frame[0] << 8) | frame[1]);
    request.unitId = frame[6];
    request.function = frame[7];
    request.startAddress = static_cast<uint16_t>((frame[8] << 8) | frame[9]);
    request.quantity = static_cast<uint16_t>((frame[10] << 8) | frame[11]);
    if (request.function != 0x03 && request.function != 0x04) {
        error = ModbusError::ILLEGAL_FUNCTION;
        return false;
    }
    if (request.quantity == 0 || request.quantity > maxRegisters ||
        static_cast<uint32_t>(request.startAddress) + request.quantity > 16) {
        error = request.quantity == 0 || request.quantity > maxRegisters
                    ? ModbusError::ILLEGAL_VALUE
                    : ModbusError::ILLEGAL_ADDRESS;
        return false;
    }
    return true;
}

uint16_t encodeVoltage(float volts) { return scaled(volts, 100.0f); }
uint16_t encodeCurrent(float amps) { return scaled(amps, 1000.0f); }
uint16_t encodePower(float watts) { return scaled(watts, 10.0f); }

void splitUint32(uint32_t value, uint16_t &highWord, uint16_t &lowWord) {
    highWord = static_cast<uint16_t>(value >> 16);
    lowWord = static_cast<uint16_t>(value & 0xffffU);
}

size_t buildReadResponse(const ModbusReadRequest &request, const uint16_t *values,
                         uint16_t valueCount, uint8_t *response, size_t capacity) {
    const size_t size = 9U + static_cast<size_t>(valueCount) * 2U;
    if (response == nullptr || values == nullptr || valueCount == 0 || capacity < size) return 0;
    put16(response, request.transactionId);
    put16(response + 2, 0);
    put16(response + 4, static_cast<uint16_t>(3U + valueCount * 2U));
    response[6] = request.unitId;
    response[7] = request.function;
    response[8] = static_cast<uint8_t>(valueCount * 2U);
    for (uint16_t i = 0; i < valueCount; ++i) put16(response + 9 + i * 2, values[i]);
    return size;
}

size_t buildExceptionResponse(const ModbusReadRequest &request, ModbusError error,
                              uint8_t *response, size_t capacity) {
    if (response == nullptr || capacity < 9) return 0;
    put16(response, request.transactionId);
    put16(response + 2, 0);
    put16(response + 4, 3);
    response[6] = request.unitId;
    response[7] = static_cast<uint8_t>(request.function | 0x80U);
    response[8] = exceptionCode(error);
    return 9;
}

