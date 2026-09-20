#pragma once

#include <stddef.h>
#include <stdint.h>

struct ModbusReadRequest {
    uint16_t transactionId = 0;
    uint8_t unitId = 0;
    uint8_t function = 0;
    uint16_t startAddress = 0;
    uint16_t quantity = 0;
};

enum class ModbusError : uint8_t {
    NONE = 0,
    MALFORMED_FRAME,
    ILLEGAL_FUNCTION,
    ILLEGAL_ADDRESS,
    ILLEGAL_VALUE
};

bool parseReadRequest(const uint8_t *frame, size_t length, uint16_t maxRegisters,
                      ModbusReadRequest &request, ModbusError &error);
uint16_t encodeVoltage(float volts);
uint16_t encodeCurrent(float amps);
uint16_t encodePower(float watts);
void splitUint32(uint32_t value, uint16_t &highWord, uint16_t &lowWord);
size_t buildReadResponse(const ModbusReadRequest &request, const uint16_t *values,
                         uint16_t valueCount, uint8_t *response, size_t capacity);
size_t buildExceptionResponse(const ModbusReadRequest &request, ModbusError error,
                              uint8_t *response, size_t capacity);

