#pragma once

#include <ArduinoJson.h>
#include <functional>
#include <string>

#include "rule_helpers.h" // Reuse TypeCode, ErrorCode, RuleReturn

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
 * - Parse time literals in "@HH:MM:SS" format
 * 
 * The callbacks make the rule engine platform-neutral - it can work on Arduino/ESP32,
 * native systems, or in unit tests by providing appropriate implementations.
 */
struct RuleCoreEnv {
    /**
     * @brief Sensor reading callback
     * @param name Sensor identifier (e.g., "temperature", "humidity", "photoSensor")
     * @param outVal Reference to store the sensor value if found
     * @return true if sensor exists and value was set, false if sensor unknown
     * 
     * Example implementation:
     * ```cpp
     * env.tryReadSensor = [](const std::string &name, float &out) {
     *     if (name == "temperature") { out = getTemperature(); return true; }
     *     if (name == "humidity") { out = getHumidity(); return true; }
     *     return false;
     * };
     * ```
     */
    std::function<bool(const std::string &name, float &outVal)> tryReadSensor;
    
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
    std::function<bool(const std::string &name, std::function<void(float)> &outSetter)> tryGetActuator;
    
    /**
     * @brief Current time provider callback
     * @return Current time in seconds since midnight (0-86399), or -1 on error
     * 
     * Used by rules that reference "currentTime" or compare against time literals.
     * 
     * Example: If current time is 14:30:45, this should return (14*3600 + 30*60 + 45) = 52245
     */
    std::function<int()> getCurrentSeconds;
    
    /**
     * @brief Time literal parser callback
     * @param hhmmss Time string in "@HH:MM:SS" format (e.g., "@14:30:00")
     * @return Time in seconds since midnight, or -1 if parsing failed
     * 
     * Used to parse time literals in rules like ["GT", "currentTime", "@18:00:00"]
     * 
     * Example: "@14:30:00" should return (14*3600 + 30*60 + 0) = 52200
     */
    std::function<int(const std::string &hhmmss)> parseTimeLiteral;
};

/**
 * @brief Core rule processing function
 * @param doc JSON document containing the rule expression to evaluate
 * @param env Environment context providing sensor/actuator access
 * @return RuleReturn structure containing the result type, value, and any errors
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
 * - FLOAT_TYPE: Numeric result (comparisons, sensors, literals)
 * - VOID_TYPE: No return value (SET, NOP operations)
 * - BOOL_ACTUATOR_TYPE: Actuator reference (for SET operations)
 * - ERROR_TYPE: Processing error occurred
 * 
 * Example usage:
 * ```cpp
 * DynamicJsonDocument doc(256);
 * deserializeJson(doc, "[\"GT\", \"temperature\", 25]");
 * RuleReturn result = processRuleCore(doc, env);
 * if (result.type == FLOAT_TYPE) {
 *     bool tempHigh = (result.val > 0); // true if temperature > 25
 * }
 * ```
 */
RuleReturn processRuleCore(JsonVariantConst doc, const RuleCoreEnv &env);

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
 *     "[\"IF\", [\"GT\", \"temperature\", 25], [\"SET\", \"relay_0\", 1], [\"SET\", \"relay_0\", 0]]"
 * };
 * processRuleSet(rules, 1, env);
 * ```
 * 
 * The function is platform-neutral and works on both embedded systems (Arduino/ESP32)
 * and native systems for testing.
 */
void processRuleSet(
    const std::string rules[], 
    int ruleCount, 
    const RuleCoreEnv &env
);


