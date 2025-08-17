#pragma once

#include <ArduinoJson.h>
#include <functional>
#include <string>

#include "unified_value.h"  // Unified value type system

/**
 * @file rule_core.h
 * @brief Platform-neutral core rule processing engine for IoT automation
 *
 * This header defines the core interfaces for processing automation rules expressed
 * in a LISP-like JSON syntax. The rule engine supports sensors, actuators, comparisons,
 * logical operations, and control flow constructs.
 *
 * Example rule: ["IF", ["GT", "temperature", 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 * This turns relay_0 ON when temperature > 25°C, OFF otherwise.
 */

/**
 * @struct RuleCoreEnv
 * @brief Environment context for rule execution
 *
 * This structure provides callback functions that allow the rule engine to:
 * - Read sensor values from the physical environment
 * - Control actuators in the physical environment
 * - Get current time information
 *
 * Time literal parsing ("@HH:MM:SS" format) is handled internally by the rule engine.
 *
 * The callbacks make the rule engine platform-neutral - it can work on Arduino/ESP32,
 * native systems, or in unit tests by providing appropriate implementations.
 */
struct RuleCoreEnv {
    /**
     * @brief Value reading callback
     * @param name Value identifier (e.g., "temperature", "humidity", "photoSensor", "currentTime")
     * @param outVal Reference to store the value if found
     * @return true if value exists and was set, false if value unknown
     *
     * Example implementation:
     * ```cpp
     * env.tryReadValue = [](const std::string &name, UnifiedValue &out) {
     *     if (name == "temperature") { out = UnifiedValue(getTemperature()); return true; }
     *     if (name == "humidity") { out = UnifiedValue(getHumidity()); return true; }
     *     if (name == "status") { out = UnifiedValue("connected"); return true; }
     *     return false;
     * };
     * ```
     */
    std::function<bool(const std::string &name, UnifiedValue &outVal)> tryReadValue;

    /**
     * @brief Actuator control callback
     * @param name Actuator identifier (e.g., "relay_0", "relay_1", "fan_speed")
     * @param outSetter Reference to store the actuator control function if found
     * @return true if actuator exists and setter was provided, false if actuator unknown
     *
     * The outSetter function, when called with a float value, should control the actuator.
     * For relays: 0.0 = OFF, 1.0 = ON, 2.0 = DON'T CARE/AUTO
     *
     * Example implementation:
     * ```cpp
     * env.tryGetActuator = [](const std::string &name, std::function<void(float)> &setter) {
     *     if (name == "relay_0") {
     *         setter = [](float val) { setRelay(0, val); };
     *         return true;
     *     }
     *     return false;
     * };
     * ```
     */
    std::function<bool(const std::string &name, std::function<void(float)> &outSetter)>
        tryGetActuator;
};

/**
 * @brief Core rule processing function
 * @param doc JSON document containing the rule expression to evaluate
 * @param env Environment context providing sensor/actuator access
 * @return UnifiedValue containing the result, error information, or actuator reference
 *
 * This is the heart of the rule engine. It recursively processes JSON expressions
 * in LISP-like syntax:
 *
 * - Literals: 25, true, "temperature", "@14:30:00"
 * - Comparisons: ["GT", expr1, expr2], ["EQ", expr1, expr2]
 * - Logic: ["AND", expr1, expr2], ["OR", expr1, expr2], ["NOT", expr]
 * - Control: ["IF", condition, then_expr, else_expr]
 * - Actions: ["SET", actuator, value], ["NOP"]
 *
 * Return types:
 * - FLOAT_TYPE/INT_TYPE/STRING_TYPE: Value results (comparisons, sensors, literals)
 * - VOID_TYPE: No return value (SET, NOP operations)
 * - ACTUATOR_TYPE: Actuator reference (for SET operations)
 * - ERROR_TYPE: Processing error occurred
 *
 * Example usage:
 * ```cpp
 * DynamicJsonDocument doc(256);
 * deserializeJson(doc, "[\"GT\", \"temperature\", 25]");
 * UnifiedValue result = processRuleCore(doc, env);
 * if (result.type == UnifiedValue::FLOAT_TYPE) {
 *     bool tempHigh = (result.asFloat() > 0); // true if temperature > 25
 * }
 * ```
 */
UnifiedValue processRuleCore(JsonVariantConst doc, const RuleCoreEnv &env);

/**
 * @brief Process a set of rules with automatic relay control
 * @param rules Array of rule strings in JSON format
 * @param ruleCount Number of rules in the array
 * @param env Environment context for rule execution
 *
 * This function processes multiple rules in sequence, with special handling for
 * automatic relay control:
 *
 * 1. For each rule index i, first sets relay_i to "don't care" mode (value 2.0)
 * 2. Parses and executes the rule
 * 3. If the rule returns a FLOAT_TYPE result, automatically sets relay_i to that value
 * 4. VOID_TYPE results (from SET, NOP) don't trigger automatic relay control
 * 5. ERROR_TYPE results are logged but don't affect relay state
 *
 * This enables two usage patterns:
 *
 * **Simple pattern** (automatic relay control):
 * ```cpp
 * std::string rules[] = {
 *     "[\"GT\", \"temperature\", 25]",  // relay_0 = 1 if temp > 25, else 0
 *     "[\"LT\", \"humidity\", 80]"      // relay_1 = 1 if humidity < 80, else 0
 * };
 * processRuleSet(rules, 2, env);
 * ```
 *
 * **Explicit pattern** (manual relay control):
 * ```cpp
 * std::string rules[] = {
 *     "[\"IF\", [\"GT\", \"temperature\", 25], [\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\",
 * 0]]"
 * };
 * processRuleSet(rules, 1, env);
 * ```
 *
 * The function is platform-neutral and works on both embedded systems (Arduino/ESP32)
 * and native systems for testing.
 */
void processRuleSet(const std::string rules[], int ruleCount, const RuleCoreEnv &env);
