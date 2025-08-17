#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include <ArduinoJson.h>
#include <gtest/gtest.h>
#include "../src/automation_dsl/bridge_functions.h"
#include "../src/automation_dsl/core.h"
#include "../src/automation_dsl/registry_functions.h"
#include "test_mocks.h"

// Test fixture for bridge integration tests
class BridgeIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Set up environment exactly like the new bridge
        env.registerFunctions = [](FunctionRegistry& registry) {
            registerCoreFunctions(registry);
            registerBridgeFunctions(registry);
        };

        env.tryGetActuator = [](const std::string& name, std::function<void(float)>& setter) {
            if (name.rfind("relay_", 0) == 0) {
                int index = atoi(name.c_str() + 6);  // Skip "relay_" prefix
                setter = std::bind(setRelay, index, std::placeholders::_1);
                return true;
            }
            return false;
        };

        // Reset relay states before each test
        for (int i = 0; i < 4; i++) {
            RELAY_VALUES[i] = DONT_CARE;
        }
    }

    void TearDown() override {}

    // Helper function to execute a rule string
    UnifiedValue executeRule(const std::string& rule) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, rule.c_str());
        if (error) {
            return UnifiedValue::createError(UNREC_FUNC_ERROR);
        }
        return processRuleCore(doc.as<JsonVariantConst>(), env);
    }

    // Helper function to simulate processNewRuleSet for a single rule
    void processRule(const std::string& rule, int relayIndex = 0) {
        // Set relay to "don't care" mode before processing (like processNewRuleSet does)
        RELAY_VALUES[relayIndex] = DONT_CARE;

        UnifiedValue result = executeRule(rule);

        // Apply automatic relay control if result is numeric
        if (result.type == UnifiedValue::FLOAT_TYPE || result.type == UnifiedValue::INT_TYPE) {
            RELAY_VALUES[relayIndex] =
                static_cast<RelayValue>(static_cast<int>(result.asFloat()) * 10);
        }
    }

    RuleCoreEnv env;
};

// Test 1: Real bridge sensor functions
TEST_F(BridgeIntegrationTest, RealBridgeSensorFunctions) {
    // Test all bridge sensor functions work correctly
    auto result = executeRule("[\"getTemperature\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), CURRENT_TEMPERATURE);

    result = executeRule("[\"getHumidity\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), CURRENT_HUMIDITY);

    result = executeRule("[\"getPhotoSensor\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), LIGHT_LEVEL);

    result = executeRule("[\"getLightSwitch\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), static_cast<float>(IS_SWITCH_ON));
}

// Test 2: Temperature-based automation (classic example)
TEST_F(BridgeIntegrationTest, TemperatureBasedAutomation) {
    // Simple temperature rule: turn on if temp > 25°C
    std::string rule = "[\"GT\", [\"getTemperature\"], 25]";

    processRule(rule, 0);

    // Temperature is 27.5°C > 25°C, so relay should be ON
    EXPECT_EQ(RELAY_VALUES[0], ON);

    // Test with higher threshold
    rule = "[\"GT\", [\"getTemperature\"], 30]";
    processRule(rule, 0);

    // Temperature is 27.5°C < 30°C, so relay should be OFF
    EXPECT_EQ(RELAY_VALUES[0], OFF);
}

// Test 3: Humidity-based automation
TEST_F(BridgeIntegrationTest, HumidityBasedAutomation) {
    // Turn on dehumidifier if humidity > 60%
    std::string rule = "[\"GT\", [\"getHumidity\"], 60]";

    processRule(rule, 1);

    // Humidity is 65% > 60%, so relay should be ON
    EXPECT_EQ(RELAY_VALUES[1], ON);
}

// Test 4: Light-based automation
TEST_F(BridgeIntegrationTest, LightBasedAutomation) {
    // Turn on lights if it's dark (light level < 500) OR switch is on
    std::string rule =
        "[\"OR\", [\"LT\", [\"getPhotoSensor\"], 500], [\"EQ\", [\"getLightSwitch\"], 1]]";

    processRule(rule, 2);

    // Light level is 750 (not dark) but switch is ON, so relay should be ON
    EXPECT_EQ(RELAY_VALUES[2], ON);

    // Test with switch off
    IS_SWITCH_ON = 0;
    processRule(rule, 2);

    // Light level is 750 (not dark) and switch is OFF, so relay should be OFF
    EXPECT_EQ(RELAY_VALUES[2], OFF);

    // Reset switch state
    IS_SWITCH_ON = 1;
}

// Test 5: Complex multi-condition automation
TEST_F(BridgeIntegrationTest, ComplexMultiConditionAutomation) {
    // Smart garden watering: water if (temp > 25 AND humidity < 60) OR switch override
    std::string rule =
        "[\"OR\", "
        "[\"AND\", [\"GT\", [\"getTemperature\"], 25], [\"LT\", [\"getHumidity\"], 60]], "
        "[\"EQ\", [\"getLightSwitch\"], 1]]";

    processRule(rule, 0);

    // temp > 25 (true) AND humidity < 60 (false) = false
    // BUT switch is on (true), so OR result is true
    EXPECT_EQ(RELAY_VALUES[0], ON);

    // Test with switch off
    IS_SWITCH_ON = 0;
    processRule(rule, 0);

    // temp > 25 (true) AND humidity < 60 (false) = false
    // switch is off (false), so OR result is false
    EXPECT_EQ(RELAY_VALUES[0], OFF);

    // Reset switch state
    IS_SWITCH_ON = 1;
}

// Test 6: IF-SET explicit control (overrides automatic relay control)
TEST_F(BridgeIntegrationTest, IfSetExplicitControl) {
    // Explicit control: IF temp > 25 THEN set relay_0 to 1 ELSE set relay_0 to 0
    std::string rule =
        "[\"IF\", [\"GT\", [\"getTemperature\"], 25], "
        "[\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\", 0]]";

    auto result = executeRule(rule);
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);

    // Temperature is 27.5°C > 25°C, so relay should be explicitly set to ON
    EXPECT_EQ(RELAY_VALUES[0], ON);

    // Test with different temperature threshold
    rule =
        "[\"IF\", [\"GT\", [\"getTemperature\"], 30], "
        "[\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\", 0]]";

    result = executeRule(rule);
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);

    // Temperature is 27.5°C < 30°C, so relay should be explicitly set to OFF
    EXPECT_EQ(RELAY_VALUES[0], OFF);
}

// Test 7: Time-based automation
TEST_F(BridgeIntegrationTest, TimeBasedAutomation) {
    // Turn on lights after 6 PM (18:00:00)
    std::string rule = "[\"GT\", [\"getCurrentTime\"], \"@18:00:00\"]";

    processRule(rule, 0);

    // Mock time is 12:00:00 (43200 seconds) < 18:00:00 (64800 seconds)
    EXPECT_EQ(RELAY_VALUES[0], OFF);

    // Test with earlier time
    rule = "[\"GT\", [\"getCurrentTime\"], \"@10:00:00\"]";
    processRule(rule, 0);

    // Mock time is 12:00:00 > 10:00:00
    EXPECT_EQ(RELAY_VALUES[0], ON);
}

// Test 8: Sensor validation (zero arguments)
TEST_F(BridgeIntegrationTest, SensorValidation) {
    // Test that sensor functions reject additional arguments
    auto result = executeRule("[\"getTemperature\", 25]");
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, UNREC_FUNC_ERROR);

    result = executeRule("[\"getHumidity\", \"extra\", \"args\"]");
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, UNREC_FUNC_ERROR);
}

// Test 9: Real-world scenario: Smart greenhouse control
TEST_F(BridgeIntegrationTest, SmartGreenhouseControl) {
    // Scenario: Greenhouse ventilation system
    // - Turn on fan if temp > 26°C OR humidity > 70%
    // - But only during daylight hours (light > 400) unless manually overridden

    std::string rule =
        "[\"AND\", "
        "[\"OR\", [\"GT\", [\"getTemperature\"], 26], [\"GT\", [\"getHumidity\"], 70]], "
        "[\"OR\", [\"GT\", [\"getPhotoSensor\"], 400], [\"EQ\", [\"getLightSwitch\"], 1]]]";

    processRule(rule, 0);

    // temp > 26 (true) OR humidity > 70 (false) = true
    // light > 400 (true) OR switch on (true) = true
    // true AND true = true
    EXPECT_EQ(RELAY_VALUES[0], ON);

    // Test nighttime scenario (low light, switch off)
    LIGHT_LEVEL = 100.0f;
    IS_SWITCH_ON = 0;
    processRule(rule, 0);

    // temp > 26 (true) OR humidity > 70 (false) = true
    // light > 400 (false) OR switch on (false) = false
    // true AND false = false
    EXPECT_EQ(RELAY_VALUES[0], OFF);

    // Reset values
    LIGHT_LEVEL = 750.0f;
    IS_SWITCH_ON = 1;
}

// Test 10: Rule processing with different sensor values
TEST_F(BridgeIntegrationTest, RuleProcessingWithDifferentSensorValues) {
    // Test same rule with different sensor readings
    std::string rule =
        "[\"AND\", [\"LT\", [\"getTemperature\"], 30], [\"GT\", [\"getHumidity\"], 50]]";

    // Original values: temp=27.5, humidity=65
    processRule(rule, 0);
    EXPECT_EQ(RELAY_VALUES[0], ON);  // temp < 30 (true) AND humidity > 50 (true)

    // Change temperature
    CURRENT_TEMPERATURE = 35.0f;
    processRule(rule, 0);
    EXPECT_EQ(RELAY_VALUES[0], OFF);  // temp < 30 (false) AND humidity > 50 (true)

    // Change humidity
    CURRENT_TEMPERATURE = 27.5f;
    CURRENT_HUMIDITY = 40.0f;
    processRule(rule, 0);
    EXPECT_EQ(RELAY_VALUES[0], OFF);  // temp < 30 (true) AND humidity > 50 (false)

    // Reset to original values
    CURRENT_TEMPERATURE = 27.5f;
    CURRENT_HUMIDITY = 65.0f;
}

// Test 11: Error propagation in complex expressions
TEST_F(BridgeIntegrationTest, ErrorPropagationInComplexExpressions) {
    // Test that errors in nested expressions propagate correctly
    std::string rule = "[\"AND\", [\"GT\", [\"getTemperature\"], 25], [\"UNKNOWN_FUNCTION\"]]";

    auto result = executeRule(rule);

    // Should short-circuit and return false (not error) because first condition is true
    // but we're testing AND where first is true, so second gets evaluated
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, FUNCTION_NOT_FOUND);
}

// Test 12: Multiple relay control simulation
TEST_F(BridgeIntegrationTest, MultipleRelayControlSimulation) {
    // Simulate multiple rules like processNewRuleSet would handle
    std::vector<std::string> rules = {
        "[\"GT\", [\"getTemperature\"], 25]",   // relay_0: heating
        "[\"GT\", [\"getHumidity\"], 60]",      // relay_1: dehumidifier
        "[\"LT\", [\"getPhotoSensor\"], 500]",  // relay_2: lights
        "[\"EQ\", [\"getLightSwitch\"], 1]"     // relay_3: manual override
    };

    for (size_t i = 0; i < rules.size(); i++) {
        processRule(rules[i], static_cast<int>(i));
    }

    // Check expected relay states
    EXPECT_EQ(RELAY_VALUES[0], ON);   // temp 27.5 > 25
    EXPECT_EQ(RELAY_VALUES[1], ON);   // humidity 65 > 60
    EXPECT_EQ(RELAY_VALUES[2], OFF);  // light 750 not < 500
    EXPECT_EQ(RELAY_VALUES[3], ON);   // switch is on
}
