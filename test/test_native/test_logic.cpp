#include <unity.h>
#include <cstring>
#include "energy_logic.h"
#include "fault_manager.h"
#include "modbus_codec.h"
#include "telemetry_buffer.h"
#include "version.h"
#include "ota_manager.h"

void test_energy_integrates_watts_over_time() {
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.5f, integrateKWh(1000.0f, 1800000));
}

void test_zero_or_negative_power_does_not_decrease_total() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, integrateKWh(-20.0f, 3600000));
}

void test_cost_uses_tariff() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, calculateCost(2.0f, 7.5f));
}

void test_cutoffs_remove_noise() {
    EnergySample sample;
    sample.voltage = 48.0f;
    sample.current = 0.1f;
    sample.power = 2.0f;
    sample = sanitizeSample(sample, 50.0f, 0.30f, 5.0f);
    TEST_ASSERT_FALSE(sample.valid);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, sample.power);
}

void test_fault_state_machine_debounces_and_recovers() {
    FaultManager manager;
    EnergyData overload;
    overload.voltage = 230.0f;
    overload.current = 13.0f;
    overload.power = 2990.0f;
    overload.valid = true;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SystemState::WARNING),
                            static_cast<uint8_t>(manager.evaluate(overload, 0).state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SystemState::WARNING),
                            static_cast<uint8_t>(manager.evaluate(overload, 1999).state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SystemState::FAULT),
                            static_cast<uint8_t>(manager.evaluate(overload, 2000).state));

    EnergyData healthy = overload;
    healthy.current = 1.0f;
    healthy.power = 230.0f;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SystemState::RECOVERY),
                            static_cast<uint8_t>(manager.evaluate(healthy, 3000).state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SystemState::NORMAL),
                            static_cast<uint8_t>(manager.evaluate(healthy, 13000).state));
}

void test_modbus_scaling_and_word_split() {
    TEST_ASSERT_EQUAL_UINT16(22954, encodeVoltage(229.54f));
    TEST_ASSERT_EQUAL_UINT16(1250, encodeCurrent(1.25f));
    TEST_ASSERT_EQUAL_UINT16(65535, encodePower(100000.0f));
    uint16_t high = 0;
    uint16_t low = 0;
    splitUint32(0x12345678UL, high, low);
    TEST_ASSERT_EQUAL_UINT16(0x1234, high);
    TEST_ASSERT_EQUAL_UINT16(0x5678, low);
}

void test_modbus_rejects_malformed_and_unsupported_frames() {
    uint8_t frame[12] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x06, 0x00, 0x00, 0x00, 0x01};
    ModbusReadRequest request;
    ModbusError error;
    TEST_ASSERT_FALSE(parseReadRequest(frame, sizeof(frame), 16, request, error));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ModbusError::ILLEGAL_FUNCTION), static_cast<uint8_t>(error));
    TEST_ASSERT_FALSE(parseReadRequest(frame, 4, 16, request, error));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ModbusError::MALFORMED_FRAME), static_cast<uint8_t>(error));
}

void test_telemetry_buffer_is_fifo_and_drops_oldest() {
    TelemetryBuffer buffer;
    TelemetryRecord record;
    for (uint32_t i = 0; i < config::telemetryBufferCapacity; ++i) {
        TelemetryRecord item;
        item.timestampMs = i;
        TEST_ASSERT_TRUE(buffer.push(item));
    }
    TelemetryRecord newest;
    newest.timestampMs = 1000;
    TEST_ASSERT_TRUE(buffer.push(newest));
    TEST_ASSERT_EQUAL_UINT32(1, buffer.droppedCount());
    TEST_ASSERT_EQUAL_UINT32(config::telemetryBufferCapacity, buffer.size());
    TEST_ASSERT_TRUE(buffer.pop(record));
    TEST_ASSERT_EQUAL_UINT32(1, record.timestampMs);
    TEST_ASSERT_TRUE(buffer.pop(record));
    TEST_ASSERT_EQUAL_UINT32(2, record.timestampMs);

    TelemetryRecord wrap;
    wrap.timestampMs = 2000;
    TEST_ASSERT_TRUE(buffer.push(wrap));
    TEST_ASSERT_TRUE(buffer.pop(record));
    TEST_ASSERT_EQUAL_UINT32(3, record.timestampMs);
}

void test_telemetry_buffer_empty_pop_is_safe() {
    TelemetryBuffer buffer;
    TelemetryRecord record;
    TEST_ASSERT_FALSE(buffer.pop(record));
    TEST_ASSERT_FALSE(buffer.peek(record));
    TEST_ASSERT_EQUAL_UINT32(0, buffer.droppedCount());
}

void test_semantic_versions_compare_numerically() {
    SemanticVersion a;
    SemanticVersion b;
    TEST_ASSERT_TRUE(parseSemanticVersion("1.0.10", a));
    TEST_ASSERT_TRUE(parseSemanticVersion("1.0.9", b));
    TEST_ASSERT_TRUE(compareSemanticVersions(a, b) > 0);
    TEST_ASSERT_TRUE(parseSemanticVersion("2.0.0", a));
    TEST_ASSERT_TRUE(parseSemanticVersion("1.9.9", b));
    TEST_ASSERT_TRUE(compareSemanticVersions(a, b) > 0);
    TEST_ASSERT_TRUE(parseSemanticVersion("1.2.3", b));
    TEST_ASSERT_EQUAL_INT(0, compareSemanticVersions(b, b));
    TEST_ASSERT_FALSE(parseSemanticVersion("1.2", a));
    TEST_ASSERT_FALSE(parseSemanticVersion("1.2.65536", a));
}

void test_manifest_parser_validates_required_fields() {
    const char *json = "{\"version\":\"1.1.0\",\"firmware_url\":\"https://example.test/fw.bin\",\"sha256\":\"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\",\"size_bytes\":1234}";
    FirmwareManifest manifest;
    OtaError error;
    TEST_ASSERT_TRUE(parseFirmwareManifest(json, strlen(json), manifest, error));
    TEST_ASSERT_EQUAL_UINT32(1234, manifest.sizeBytes);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OtaError::NONE), static_cast<uint8_t>(error));
    const char *invalid = "{\"version\":\"1.1.0\",\"firmware_url\":\"ftp://example.test/fw.bin\",\"sha256\":\"bad\"}";
    TEST_ASSERT_FALSE(parseFirmwareManifest(invalid, strlen(invalid), manifest, error));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_energy_integrates_watts_over_time);
    RUN_TEST(test_zero_or_negative_power_does_not_decrease_total);
    RUN_TEST(test_cost_uses_tariff);
    RUN_TEST(test_cutoffs_remove_noise);
    RUN_TEST(test_fault_state_machine_debounces_and_recovers);
    RUN_TEST(test_modbus_scaling_and_word_split);
    RUN_TEST(test_modbus_rejects_malformed_and_unsupported_frames);
    RUN_TEST(test_telemetry_buffer_is_fifo_and_drops_oldest);
    RUN_TEST(test_telemetry_buffer_empty_pop_is_safe);
    RUN_TEST(test_semantic_versions_compare_numerically);
    RUN_TEST(test_manifest_parser_validates_required_fields);
    return UNITY_END();
}
