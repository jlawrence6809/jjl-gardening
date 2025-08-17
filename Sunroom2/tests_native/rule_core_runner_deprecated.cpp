#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "../src/automation_dsl/core.h"

// Test fixture class for shared setup
class RuleSystemTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test 1: Basic literal evaluation
TEST_F(RuleSystemTest, BasicLiteralEvaluation) {
    DynamicJsonDocument doc(256);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string&, UnifiedValue&) { return false; };

    // Test float literal
    deserializeJson(doc, "25.5");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 25.5f);

    // Test integer literal
    deserializeJson(doc, "42");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 42);

    // Test boolean literals
    deserializeJson(doc, "true");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);

    deserializeJson(doc, "false");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);
}

// Test 2: Value reading
TEST_F(RuleSystemTest, ValueReading) {
    DynamicJsonDocument doc(256);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "temperature") {
            out = UnifiedValue(25.5f);
            return true;
        }
        if (name == "count") {
            out = UnifiedValue(42);
            return true;
        }
        if (name == "status") {
            out = UnifiedValue("online");
            return true;
        }
        return false;
    };

    // Test float value reading
    deserializeJson(doc, "\"temperature\"");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 25.5f);

    // Test int value reading
    deserializeJson(doc, "\"count\"");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 42);

    // Test string value reading
    deserializeJson(doc, "\"status\"");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::STRING_TYPE);
    EXPECT_STREQ(result.asString(), "online");

    // Test unknown value
    deserializeJson(doc, "\"unknown\"");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, UNREC_STR_ERROR);
}

// Test 3: Time literal parsing
TEST_F(RuleSystemTest, TimeLiteralParsing) {
    DynamicJsonDocument doc(256);
    RuleCoreEnv env{};

    // Valid time literals
    deserializeJson(doc, "\"@14:30:00\"");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 14 * 3600 + 30 * 60);

    deserializeJson(doc, "\"@00:00:00\"");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 0);

    // Invalid time literal
    deserializeJson(doc, "\"@25:00:00\"");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, TIME_ERROR);
}

// Test 4a: GT comparison (greater than, true case)
TEST_F(RuleSystemTest, GTComparisonTrue) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "temperature") {
            out = UnifiedValue(25.5f);
            return true;
        }
        return false;
    };

    deserializeJson(doc, "[\"GT\", \"temperature\", 20.0]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);
}

// Test 4b: GT comparison (greater than, false case)
TEST_F(RuleSystemTest, GTComparisonFalse) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "temperature") {
            out = UnifiedValue(25.5f);
            return true;
        }
        return false;
    };

    deserializeJson(doc, "[\"GT\", \"temperature\", 30.0]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);
}

// Test 4c: EQ comparison with strings (equal, true case)
// Todo: This test is expected to fail because the string comparison is not implemented yet.
TEST_F(RuleSystemTest, EQComparisonStringTrue) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "status") {
            out = UnifiedValue("online");
            return true;
        }
        return false;
    };

    deserializeJson(doc, "[\"EQ\", \"status\", \"online\"]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);
}

// Test 4d: EQ comparison with strings (equal, false case)
// Todo: This test is expected to fail because the string comparison is not implemented yet.
TEST_F(RuleSystemTest, EQComparisonStringFalse) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "status") {
            out = UnifiedValue("online");
            return true;
        }
        return false;
    };

    deserializeJson(doc, "[\"EQ\", \"status\", \"offline\"]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);
}

// Test 5: Logical operations with short-circuiting
TEST_F(RuleSystemTest, LogicalOperations) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "temperature") {
            out = UnifiedValue(25.5f);
            return true;
        }
        if (name == "humidity") {
            out = UnifiedValue(60.0f);
            return true;
        }
        return false;
    };

    // AND operation (both true)
    deserializeJson(doc, "[\"AND\", [\"GT\", \"temperature\", 20], [\"LT\", \"humidity\", 80]]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);

    // AND operation (first false - should short-circuit)
    deserializeJson(doc, "[\"AND\", [\"GT\", \"temperature\", 30], [\"LT\", \"humidity\", 80]]");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 0.0f);

    // OR operation (first true - should short-circuit)
    deserializeJson(doc, "[\"OR\", [\"GT\", \"temperature\", 20], [\"GT\", \"humidity\", 80]]");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);

    // NOT operation
    deserializeJson(doc, "[\"NOT\", [\"GT\", \"temperature\", 30]]");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);
}

// Test 6: IF-THEN-ELSE conditional
TEST_F(RuleSystemTest, ConditionalOperations) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "temperature") {
            out = UnifiedValue(25.5f);
            return true;
        }
        return false;
    };

    // IF condition true
    deserializeJson(doc, "[\"IF\", [\"GT\", \"temperature\", 20], 1, 0]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 1);

    // IF condition false
    deserializeJson(doc, "[\"IF\", [\"GT\", \"temperature\", 30], 1, 0]");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 0);
}

// Test 7: Actuator operations
TEST_F(RuleSystemTest, ActuatorOperations) {
    DynamicJsonDocument doc(512);
    float capturedValue = -1.0f;
    RuleCoreEnv env{};
    env.tryGetActuator = [&capturedValue](const std::string& name,
                                          std::function<void(float)>& setter) {
        if (name == "relay_0") {
            setter = [&capturedValue](float v) { capturedValue = v; };
            return true;
        }
        return false;
    };

    // Test SET operation
    deserializeJson(doc, "[\"SET\", \"relay_0\", 1.0]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);
    EXPECT_FLOAT_EQ(capturedValue, 1.0f);

    // Test actuator reference resolution
    deserializeJson(doc, "\"relay_0\"");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::ACTUATOR_TYPE);
    auto setter = result.getActuatorSetter();
    EXPECT_NE(setter, nullptr);
    if (setter) {
        setter(2.5f);
        EXPECT_FLOAT_EQ(capturedValue, 2.5f);
    }
}

// Test 8: Time comparisons
TEST_F(RuleSystemTest, TimeComparisons) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "currentTime") {
            out = UnifiedValue(15 * 3600 + 30 * 60);  // 15:30:00
            return true;
        }
        return false;
    };

    // currentTime vs time literal
    deserializeJson(doc, "[\"GT\", \"currentTime\", \"@14:30:00\"]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);

    // Time literal vs time literal
    deserializeJson(doc, "[\"LT\", \"@12:00:00\", \"@18:00:00\"]");
    result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::FLOAT_TYPE);
    EXPECT_FLOAT_EQ(result.asFloat(), 1.0f);
}

// Test 9a: Unknown function error handling
TEST_F(RuleSystemTest, ErrorHandling_UnknownFunction) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string&, UnifiedValue&) { return false; };

    deserializeJson(doc, "[\"UNKNOWN_FUNC\", 1, 2]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, UNREC_FUNC_ERROR);
}

// Test 9b: Invalid IF condition error handling
// Todo: This test is expected to fail because the string comparison is not implemented yet.
TEST_F(RuleSystemTest, ErrorHandling_InvalidIfCondition) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string&, UnifiedValue&) { return false; };

    deserializeJson(doc, "[\"IF\", \"string_literal\", 1, 0]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
    EXPECT_EQ(result.errorCode, IF_CONDITION_ERROR);
}

// Test 9c: Type mismatch in comparison error handling
TEST_F(RuleSystemTest, ErrorHandling_TypeMismatchInComparison) {
    DynamicJsonDocument doc(512);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string&, UnifiedValue&) { return false; };

    deserializeJson(doc, "[\"GT\", \"unknown\", 20]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::ERROR_TYPE);
}

// Test 10: Complex nested expressions
TEST_F(RuleSystemTest, ComplexNestedExpressions) {
    DynamicJsonDocument doc(1024);
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "temperature") {
            out = UnifiedValue(25.0f);
            return true;
        }
        if (name == "humidity") {
            out = UnifiedValue(60.0f);
            return true;
        }
        if (name == "lightLevel") {
            out = UnifiedValue(800);
            return true;
        }
        return false;
    };

    // Complex rule: IF temperature > 20 AND humidity < 70 AND lightLevel > 500 THEN 1 ELSE 0
    deserializeJson(doc, "[\"IF\", [\"AND\", [\"AND\", [\"GT\", \"temperature\", 20], [\"LT\", "
                       "\"humidity\", 70]], [\"GT\", \"lightLevel\", 500]], 1, 0]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::INT_TYPE);
    EXPECT_EQ(result.asInt(), 1);
}

// Test 11: processRuleSet function
TEST_F(RuleSystemTest, ProcessRuleSet) {
    std::vector<float> relayValues(3, -1.0f);  // Track 3 relay values
    RuleCoreEnv env{};
    env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
        if (name == "temperature") {
            out = UnifiedValue(25.0f);
            return true;
        }
        return false;
    };
    env.tryGetActuator = [&relayValues](const std::string& name,
                                        std::function<void(float)>& setter) {
        if (name.substr(0, 6) == "relay_") {
            int index = std::stoi(name.substr(6));
            if (index >= 0 && index < 3) {
                setter = [&relayValues, index](float v) { relayValues[index] = v; };
                return true;
            }
        }
        return false;
    };

    std::string rules[] = {"[\"GT\", \"temperature\", 20]",  // Should set relay_0 to 1.0
                           "[\"SET\", \"relay_1\", 0.5]",     // Explicit SET, no auto control
                           "[\"LT\", \"temperature\", 30]"};  // Should set relay_2 to 1.0

    processRuleSet(rules, 3, env);

    EXPECT_FLOAT_EQ(relayValues[0], 1.0f);
    EXPECT_FLOAT_EQ(relayValues[1], 0.5f);
    EXPECT_FLOAT_EQ(relayValues[2], 1.0f);
}

// Test 12: NOP operation
TEST_F(RuleSystemTest, NOPOperation) {
    DynamicJsonDocument doc(256);
    RuleCoreEnv env{};

    deserializeJson(doc, "[\"NOP\"]");
    auto result = processRuleCore(doc, env);
    EXPECT_EQ(result.type, UnifiedValue::VOID_TYPE);
}