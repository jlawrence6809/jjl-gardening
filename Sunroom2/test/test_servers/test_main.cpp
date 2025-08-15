#include <Arduino.h>
#include <unity.h>

#include "definitions.h"

// Lightweight tests to ensure map shaping logic for relays is consistent

void setUp()
{
    // Ensure a predictable runtime state
    RUNTIME_RELAY_COUNT = 0;
}

void test_labels_rules_values_arrays_are_sized_by_max()
{
    // Access a high index within bounds of MAX_RELAYS-1 to ensure arrays exist
    int last = MAX_RELAYS - 1;
    // Just touch elements; Unity has no direct assert for address validity
    RELAY_VALUES[last] = FORCE_OFF_AUTO_X;
    RELAY_RULES[last] = "[\"NOP\"]";
    RELAY_LABELS[last] = "Test";
    TEST_ASSERT_EQUAL_INT(FORCE_OFF_AUTO_X, RELAY_VALUES[last]);
}

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_labels_rules_values_arrays_are_sized_by_max);
    UNITY_END();
}

void loop()
{
}


