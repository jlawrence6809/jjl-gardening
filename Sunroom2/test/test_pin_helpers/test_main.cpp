#include <Arduino.h>
#include <unity.h>

#include "pin_helpers.h"

void test_pin_helpers_valid_gpio_true()
{
    // For ESP32-S3 env, GPIO 4 is in CFG_VALID_GPIO_PINS
    TEST_ASSERT_TRUE(boardPinIsOutputAllowed(4));
    TEST_ASSERT_TRUE(boardPinIsInputAllowed(4));
}

void test_pin_helpers_invalid_gpio_false()
{
    // For ESP32-S3 env, GPIO 19 is NOT in CFG_VALID_GPIO_PINS in our config
    TEST_ASSERT_FALSE(boardPinIsOutputAllowed(19));
    TEST_ASSERT_FALSE(boardPinIsInputAllowed(19));
}

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_pin_helpers_valid_gpio_true);
    RUN_TEST(test_pin_helpers_invalid_gpio_false);
    UNITY_END();
}

void loop()
{
}


