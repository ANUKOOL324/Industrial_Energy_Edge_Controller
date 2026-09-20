#include <unity.h>
#include "energy_logic.h"

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

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_energy_integrates_watts_over_time);
    RUN_TEST(test_zero_or_negative_power_does_not_decrease_total);
    RUN_TEST(test_cost_uses_tariff);
    RUN_TEST(test_cutoffs_remove_noise);
    return UNITY_END();
}

