#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include <ArduinoJson.h>
#include <gtest/gtest.h>
#include "../src/automation_dsl/bridge_functions.h"
#include "../src/automation_dsl/new_core.h"
#include "../src/automation_dsl/registry_functions.h"
#include "test_mocks.h"

// Test fixture class for function registry tests
class FunctionRegistryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Set up environment with function registration
        env.registerFunctions = [](FunctionRegistry& registry) {
            registerCoreFunctions(registry);
            registerTestSensorFunctions(registry);
        };
        env.tryGetActuator = [](const std::string& name, std::function<void(float)>& setter) {
            if (name.rfind("relay_", 0) == 0) {
                int index = atoi(name.c_str() + 6);  // Skip "relay_" prefix
                setter = std::bind(setRelay, index, std::placeholders::_1);
                return true;
            }
            return false;
        };
    }

    void TearDown() override {}

    // Helper function to register test sensor functions
    static void registerTestSensorFunctions(FunctionRegistry& registry) {
        registry["getTemperature"] = [](JsonArrayConst args, const NewRuleCoreEnv& env) {
            return UnifiedValue(CURRENT_TEMPERATURE);
        };

        registry["getHumidity"] = [](JsonArrayConst args, const NewRuleCoreEnv& env) {
            return UnifiedValue(CURRENT_HUMIDITY);
        };

        registry["getPhotoSensor"] = [](JsonArrayConst args, const NewRuleCoreEnv& env) {
            return UnifiedValue(LIGHT_LEVEL);
        };

        registry["getLightSwitch"] = [](JsonArrayConst args, const NewRuleCoreEnv& env) {
            return UnifiedValue(static_cast<float>(IS_SWITCH_ON));
        };

        registry["getCurrentTime"] = [](JsonArrayConst args, const NewRuleCoreEnv& env) {
            return UnifiedValue(43200.0f);  // 12:00:00 noon
        };
    }

    // Helper function to parse and execute a rule
    UnifiedValue executeRule(const std::string& rule) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, rule.c_str());
        if (error) {
            return UnifiedValue::createError(UNREC_FUNC_ERROR);
        }
        return processNewRuleCore(doc.as<JsonVariantConst>(), env);
    }

    NewRuleCoreEnv env;
};

// Test 1: Basic sensor reading
TEST_F(FunctionRegistryTest, BasicSensorReading) {
    auto result = executeRule("[\"getTemperature\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 27.5f);

    result = executeRule("[\"getHumidity\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 65.0f);

    result = executeRule("[\"getPhotoSensor\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 750.0f);

    result = executeRule("[\"getLightSwitch\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);
}

// Test 2: Comparison operations
TEST_F(FunctionRegistryTest, ComparisonOperations) {
    // GT (Greater Than)
    auto result = executeRule("[\"GT\", [\"getTemperature\"], 25]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 27.5 > 25 = true

    result = executeRule("[\"GT\", [\"getTemperature\"], 30]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);  // 27.5 > 30 = false

    // LT (Less Than)
    result = executeRule("[\"LT\", [\"getHumidity\"], 70]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 65 < 70 = true

    // EQ (Equal)
    result = executeRule("[\"EQ\", [\"getLightSwitch\"], 1]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 1 == 1 = true

    // NE (Not Equal)
    result = executeRule("[\"NE\", [\"getLightSwitch\"], 0]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 1 != 0 = true

    // GTE (Greater Than or Equal)
    result = executeRule("[\"GTE\", [\"getTemperature\"], 27.5]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 27.5 >= 27.5 = true

    // LTE (Less Than or Equal)
    result = executeRule("[\"LTE\", [\"getHumidity\"], 65]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 65 <= 65 = true
}

// Test 3: Logical operations
TEST_F(FunctionRegistryTest, LogicalOperations) {
    // AND operation
    auto result = executeRule(
        "[\"AND\", [\"GT\", [\"getTemperature\"], 25], [\"LT\", [\"getHumidity\"], 70]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // true AND true = true

    result = executeRule(
        "[\"AND\", [\"GT\", [\"getTemperature\"], 30], [\"LT\", [\"getHumidity\"], 70]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);  // false AND true = false

    // OR operation
    result = executeRule(
        "[\"OR\", [\"GT\", [\"getTemperature\"], 30], [\"EQ\", [\"getLightSwitch\"], 1]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // false OR true = true

    result = executeRule(
        "[\"OR\", [\"GT\", [\"getTemperature\"], 30], [\"EQ\", [\"getLightSwitch\"], 0]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);  // false OR false = false

    // NOT operation
    result = executeRule("[\"NOT\", [\"GT\", [\"getTemperature\"], 30]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // NOT false = true

    result = executeRule("[\"NOT\", [\"GT\", [\"getTemperature\"], 25]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);  // NOT true = false
}

// Test 4: IF-THEN-ELSE control flow
TEST_F(FunctionRegistryTest, IfThenElseControlFlow) {
    // Test IF with true condition
    auto result = executeRule("[\"IF\", [\"GT\", [\"getTemperature\"], 25], 100, 200]");
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 100);  // Condition is true, return then branch

    // Test IF with false condition
    result = executeRule("[\"IF\", [\"GT\", [\"getTemperature\"], 30], 100, 200]");
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 200);  // Condition is false, return else branch
}

// Test 5: SET operations with actuators
TEST_F(FunctionRegistryTest, SetOperationsWithActuators) {
    // Reset relay state
    RELAY_VALUES[0] = DONT_CARE;

    // Test SET operation
    auto result = executeRule("[\"SET\", \"relay_0\", 1]");
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);
    EXPECT_EQ(RELAY_VALUES[0], ON);  // Relay should be set to ON (10)

    // Test SET with different value
    result = executeRule("[\"SET\", \"relay_0\", 0]");
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);
    EXPECT_EQ(RELAY_VALUES[0], OFF);  // Relay should be set to OFF (0)
}

// Test 6: NOP (No Operation)
TEST_F(FunctionRegistryTest, NoOperation) {
    auto result = executeRule("[\"NOP\"]");
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);
    EXPECT_EQ(result.errorCode, NO_ERROR);
}

// Test 7: Error handling
TEST_F(FunctionRegistryTest, ErrorHandling) {
    // Test unknown function
    auto result = executeRule("[\"UNKNOWN_FUNCTION\", 1, 2]");
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, FUNCTION_NOT_FOUND);

    // Test empty array
    result = executeRule("[]");
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, UNREC_FUNC_ERROR);

    // Test array with non-string function name
    result = executeRule("[123, 1, 2]");
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, UNREC_FUNC_ERROR);
}

// Test 8: Complex nested expressions
TEST_F(FunctionRegistryTest, ComplexNestedExpressions) {
    // Nested logical expression: (temp > 25 AND humidity < 70) OR switch == 1
    std::string complexRule =
        "[\"OR\", "
        "[\"AND\", [\"GT\", [\"getTemperature\"], 25], [\"LT\", [\"getHumidity\"], 70]], "
        "[\"EQ\", [\"getLightSwitch\"], 1]]";

    auto result = executeRule(complexRule);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // Should be true

    // Complex IF statement with nested conditions
    std::string ifRule =
        "[\"IF\", "
        "[\"AND\", [\"GT\", [\"getTemperature\"], 25], [\"LT\", [\"getHumidity\"], 60]], "
        "[\"SET\", \"relay_0\", 1], "
        "[\"SET\", \"relay_0\", 0]]";

    RELAY_VALUES[0] = DONT_CARE;
    result = executeRule(ifRule);
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);
    // Condition: temp > 25 (true) AND humidity < 60 (false) = false
    // Should execute else branch: SET relay_0 to 0
    EXPECT_EQ(RELAY_VALUES[0], OFF);
}

// Test 9: Time-based operations
TEST_F(FunctionRegistryTest, TimeBasedOperations) {
    // Test time literal parsing
    auto result = executeRule("[\"GT\", \"@10:00:00\", \"@09:00:00\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 10:00 > 09:00 = true

    // Test current time comparison (mock time is 12:00:00 = 43200 seconds)
    result = executeRule("[\"GT\", [\"getCurrentTime\"], \"@10:00:00\"]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // 12:00 > 10:00 = true
}

// Test 10: Short-circuit evaluation
TEST_F(FunctionRegistryTest, ShortCircuitEvaluation) {
    // Test AND short-circuit: if first is false, second should not be evaluated
    // Using a condition that would cause error if evaluated
    auto result =
        executeRule("[\"AND\", [\"GT\", [\"getTemperature\"], 30], [\"UNKNOWN_FUNCTION\"]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);  // Should be false without error

    // Test OR short-circuit: if first is true, second should not be evaluated
    result = executeRule("[\"OR\", [\"GT\", [\"getTemperature\"], 25], [\"UNKNOWN_FUNCTION\"]]");
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);  // Should be true without error
}
