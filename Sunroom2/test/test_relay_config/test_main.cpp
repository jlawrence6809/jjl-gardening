#include <Arduino.h>
#include <unity.h>

#include "definitions.h"

void setUpRelayRuntimeZero()
{
    RUNTIME_RELAY_COUNT = 0;
}

void test_runtime_relays_start_zero()
{
    setUpRelayRuntimeZero();
    TEST_ASSERT_EQUAL_INT(0, RUNTIME_RELAY_COUNT);
}

void test_add_runtime_relay_and_bounds()
{
    setUpRelayRuntimeZero();
    // Simulate adding one relay into slot 0
    RUNTIME_RELAY_PINS[0] = 4;
    RUNTIME_RELAY_IS_INVERTED[0] = true;
    RELAY_VALUES[0] = FORCE_OFF_AUTO_X;
    RELAY_RULES[0] = "[\"NOP\"]";
    RELAY_LABELS[0] = "Relay 0";
    RUNTIME_RELAY_COUNT = 1;

    TEST_ASSERT_EQUAL_INT(1, RUNTIME_RELAY_COUNT);
    TEST_ASSERT_EQUAL_INT(4, RUNTIME_RELAY_PINS[0]);
    TEST_ASSERT_TRUE(RUNTIME_RELAY_IS_INVERTED[0]);
}

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_runtime_relays_start_zero);
    RUN_TEST(test_add_runtime_relay_and_bounds);
    UNITY_END();
}

void loop()
{
}


