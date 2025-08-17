#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include <ArduinoJson.h>
#include "../src/automation_dsl/core.h"

// ANSI color codes
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLUE "\033[34m"
#define COLOR_YELLOW "\033[33m"

int failures = 0;

void log_info(const std::string& message) {
    std::cout << COLOR_BLUE "[INFO] " COLOR_RESET << message << std::endl;
}

void log_debug(const std::string& message) {
    std::cout << COLOR_YELLOW "[DEBUG] " COLOR_RESET << message << std::endl;
}

void log_success(const std::string& message) {
    std::cout << COLOR_GREEN "[PASS] " COLOR_RESET << message << std::endl;
}

void log_error(const std::string& message) {
    std::cout << COLOR_RED "[FAIL] " COLOR_RESET << message << std::endl;
}

void check(bool condition, const std::string& message) {
    if (condition) {
        log_success(message);
    } else {
        log_error(message);
        failures++;
    }
}

int main() {
    log_info("Running unified rule system tests...");

    // Test 1: Basic literal evaluation
    {
        log_debug("Test 1: Basic literal evaluation");

        DynamicJsonDocument doc(256);
        RuleCoreEnv env{};
        env.tryReadValue = [](const std::string&, UnifiedValue&) { return false; };

        // Test float literal
        deserializeJson(doc, "25.5");
        auto result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 25.5f,
              "Float literal evaluation");

        // Test integer literal
        deserializeJson(doc, "42");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::INT_TYPE && result.asInt() == 42, "Integer literal evaluation");

        // Test boolean literals
        deserializeJson(doc, "true");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "Boolean true literal evaluation");

        deserializeJson(doc, "false");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 0.0f,
              "Boolean false literal evaluation");
    }

    // Test 2: Value reading
    {
        log_debug("Test 2: Value reading");

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
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 25.5f,
              "Float value reading");

        // Test int value reading
        deserializeJson(doc, "\"count\"");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::INT_TYPE && result.asInt() == 42, "Int value reading");

        // Test string value reading
        deserializeJson(doc, "\"status\"");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::STRING_TYPE &&
                  std::string(result.asString()) == "online",
              "String value reading");

        // Test unknown value
        deserializeJson(doc, "\"unknown\"");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::ERROR_TYPE && result.errorCode == UNREC_STR_ERROR,
              "Unknown value returns error");
    }

    // Test 3: Time literal parsing
    {
        log_debug("Test 3: Time literal parsing");

        DynamicJsonDocument doc(256);
        RuleCoreEnv env{};

        // Valid time literals
        deserializeJson(doc, "\"@14:30:00\"");
        auto result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::INT_TYPE && result.asInt() == (14 * 3600 + 30 * 60),
              "Valid time literal parsing");

        deserializeJson(doc, "\"@00:00:00\"");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::INT_TYPE && result.asInt() == 0,
              "Midnight time literal parsing");

        // Invalid time literal
        deserializeJson(doc, "\"@25:00:00\"");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::ERROR_TYPE && result.errorCode == TIME_ERROR,
              "Invalid time literal returns error");
    }

    // Test 4: Comparison operations
    {
        log_debug("Test 4: Comparison operations");

        DynamicJsonDocument doc(512);
        RuleCoreEnv env{};
        env.tryReadValue = [](const std::string& name, UnifiedValue& out) {
            if (name == "temperature") {
                out = UnifiedValue(25.5f);
                return true;
            }
            if (name == "status") {
                out = UnifiedValue("online");
                return true;
            }
            return false;
        };

        // GT comparison
        deserializeJson(doc, "[\"GT\", \"temperature\", 20.0]");
        auto result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "GT comparison (true)");

        deserializeJson(doc, "[\"GT\", \"temperature\", 30.0]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 0.0f,
              "GT comparison (false)");

        // EQ comparison with strings
        deserializeJson(doc, "[\"EQ\", \"status\", \"online\"]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "EQ string comparison (true)");

        deserializeJson(doc, "[\"EQ\", \"status\", \"offline\"]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 0.0f,
              "EQ string comparison (false)");
    }

    // Test 5: Logical operations with short-circuiting
    {
        log_debug("Test 5: Logical operations");

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
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "AND operation (both true)");

        // AND operation (first false - should short-circuit)
        deserializeJson(doc, "[\"AND\", [\"GT\", \"temperature\", 30], [\"LT\", \"humidity\", 80]]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 0.0f,
              "AND operation (first false)");

        // OR operation (first true - should short-circuit)
        deserializeJson(doc, "[\"OR\", [\"GT\", \"temperature\", 20], [\"GT\", \"humidity\", 80]]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "OR operation (first true)");

        // NOT operation
        deserializeJson(doc, "[\"NOT\", [\"GT\", \"temperature\", 30]]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "NOT operation");
    }

    // Test 6: IF-THEN-ELSE conditional
    {
        log_debug("Test 6: IF-THEN-ELSE conditional");

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
        check(result.type == UnifiedValue::INT_TYPE && result.asInt() == 1, "IF condition true");

        // IF condition false
        deserializeJson(doc, "[\"IF\", [\"GT\", \"temperature\", 30], 1, 0]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::INT_TYPE && result.asInt() == 0, "IF condition false");
    }

    // Test 7: Actuator operations
    {
        log_debug("Test 7: Actuator operations");

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
        check(result.type == UnifiedValue::VOID_TYPE, "SET operation returns VOID");
        check(capturedValue == 1.0f, "SET operation calls actuator function");

        // Test actuator reference resolution
        deserializeJson(doc, "\"relay_0\"");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::ACTUATOR_TYPE, "Actuator reference resolution");
        auto setter = result.getActuatorSetter();
        check(setter != nullptr, "Actuator reference has setter function");
        if (setter) {
            setter(2.5f);
            check(capturedValue == 2.5f, "Retrieved actuator function works");
        }
    }

    // Test 8: Time comparisons
    {
        log_debug("Test 8: Time comparisons");

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
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "currentTime > time literal");

        // Time literal vs time literal
        deserializeJson(doc, "[\"LT\", \"@12:00:00\", \"@18:00:00\"]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::FLOAT_TYPE && result.asFloat() == 1.0f,
              "Time literal < time literal");
    }

    // Test 9: Error handling
    {
        log_debug("Test 9: Error handling");

        DynamicJsonDocument doc(512);
        RuleCoreEnv env{};
        env.tryReadValue = [](const std::string&, UnifiedValue&) { return false; };

        // Unknown function
        deserializeJson(doc, "[\"UNKNOWN_FUNC\", 1, 2]");
        auto result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::ERROR_TYPE && result.errorCode == UNREC_FUNC_ERROR,
              "Unknown function returns error");

        // Invalid IF condition
        deserializeJson(doc, "[\"IF\", \"string_literal\", 1, 0]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::ERROR_TYPE && result.errorCode == IF_CONDITION_ERROR,
              "Invalid IF condition returns error");

        // Type mismatch in comparison
        deserializeJson(doc, "[\"GT\", \"unknown\", 20]");
        result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::ERROR_TYPE, "Type error in comparison");
    }

    // Test 10: Complex nested expressions
    {
        log_debug("Test 10: Complex nested expressions");

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
        check(result.type == UnifiedValue::INT_TYPE && result.asInt() == 1,
              "Complex nested expression evaluation");
    }

    // Test 11: processRuleSet function
    {
        log_debug("Test 11: processRuleSet function");

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

        check(relayValues[0] == 1.0f, "processRuleSet: Rule 0 sets relay_0 automatically");
        check(relayValues[1] == 0.5f, "processRuleSet: Rule 1 explicit SET");
        check(relayValues[2] == 1.0f, "processRuleSet: Rule 2 sets relay_2 automatically");
    }

    // Test 12: NOP operation
    {
        log_debug("Test 12: NOP operation");

        DynamicJsonDocument doc(256);
        RuleCoreEnv env{};

        deserializeJson(doc, "[\"NOP\"]");
        auto result = processRuleCore(doc, env);
        check(result.type == UnifiedValue::VOID_TYPE, "NOP returns VOID_TYPE");
    }

    // Test results
    if (failures == 0) {
        log_success("All unified rule system tests passed!");
        return 0;
    } else {
        log_error("Unified rule system tests failed: " + std::to_string(failures) + " failures");
        return 1;
    }
}
