#pragma once

#include <ArduinoJson.h>
#include <functional>
#include <map>
#include <string>

#include "unified_value.h"  // Unified value type system

/**
 * @file new_core.h
 * @brief Platform-neutral core rule processing engine with function registry (UnifiedValue version)
 *
 * This header defines the core interfaces for processing automation rules expressed
 * in a LISP-like JSON syntax using a pluggable function registry system. The rule engine
 * supports sensors, actuators, comparisons, logical operations, and control flow constructs.
 *
 * Example rule: ["IF", ["GT", ["getTemperature"], 25], ["SET", "relay_0", 1], ["SET", "relay_0",
 * 0]] This turns relay_0 ON when temperature > 25°C, OFF otherwise.
 *
 * Key improvements over original core:
 * - Unified function system: everything is a function call
 * - Pluggable function registry for extensibility
 * - Platform-neutral core with platform-specific function registration
 */

// Forward declaration for the environment structure
struct RuleCoreEnv;

/**
 * @typedef FunctionHandler
 * @brief Function signature for all DSL functions
 *
 * All functions in the DSL follow this signature:
 * - Take a JsonArrayConst containing the function call (including function name)
 * - Take the environment context for accessing other functions/actuators
 * - Return a UnifiedValue containing the result, error, or actuator reference
 */
using FunctionHandler = std::function<UnifiedValue(JsonArrayConst, const RuleCoreEnv &)>;

/**
 * @typedef FunctionRegistry
 * @brief Registry mapping function names to their handlers
 *
 * This is the core of the new architecture - a simple map from function names
 * to their implementation functions. Allows dynamic registration of functions
 * from different modules/platforms.
 */
using FunctionRegistry = std::map<std::string, FunctionHandler>;

/**
 * @struct NewRuleCoreEnv
 * @brief Environment context for rule execution with function registry
 *
 * This structure provides the pluggable function registration mechanism that allows
 * different platforms (Arduino, native tests, different sensor modules) to register
 * their specific functions into the rule engine.
 *
 * The registry-based approach replaces the old callback-based value reading system
 * with a unified function system where everything is a function call.
 */
struct RuleCoreEnv {
    /**
     * @brief Function registration callback
     * @param registry Reference to the function registry to populate
     *
     * This callback is called during rule engine initialization to allow the
     * platform/module to register all available functions. Different platforms
     * can register different sets of functions.
     *
     * Example implementation:
     * ```cpp
     * env.registerFunctions = [](FunctionRegistry& registry) {
     *     // Register sensor functions
     *     registry["getTemperature"] = [](JsonArrayConst args, const RuleCoreEnv& env) {
     *         return UnifiedValue(getCurrentTemperature());
     *     };
     *
     *     // Register operator functions
     *     registry["GT"] = [](JsonArrayConst args, const RuleCoreEnv& env) {
     *         // Implementation for greater-than comparison
     *         // ...
     *     };
     * };
     * ```
     */
    std::function<void(FunctionRegistry &)> registerFunctions;

    /**
     * @brief Actuator control callback (unchanged from original)
     * @param name Actuator identifier (e.g., "relay_0", "relay_1", "fan_speed")
     * @param outSetter Reference to store the actuator control function if found
     * @return true if actuator exists and setter was provided, false if actuator unknown
     *
     * Note: Actuators remain as a separate callback system since they represent
     * physical hardware control rather than data/computation functions.
     */
    std::function<bool(const std::string &name, std::function<void(float)> &outSetter)>
        tryGetActuator;
};

/**
 * @brief Core rule processing function with function registry
 * @param doc JSON document containing the rule expression to evaluate
 * @param env Environment context providing function registry and actuator access
 * @return UnifiedValue containing the result, error information, or actuator reference
 *
 * This is the heart of the new rule engine. It processes JSON expressions using
 * the function registry system:
 *
 * - All operations are function calls: ["functionName", arg1, arg2, ...]
 * - Functions are looked up in the registry provided by the environment
 * - Supports same operations as original but with unified syntax:
 *   - Sensors: ["getTemperature"], ["getHumidity"]
 *   - Comparisons: ["GT", expr1, expr2], ["EQ", expr1, expr2]
 *   - Logic: ["AND", expr1, expr2], ["OR", expr1, expr2], ["NOT", expr]
 *   - Control: ["IF", condition, then_expr, else_expr]
 *   - Actions: ["SET", actuator, value], ["NOP"]
 *
 * Return types (same as original):
 * - FLOAT_TYPE/INT_TYPE/STRING_TYPE: Value results
 * - VOID_TYPE: No return value (SET, NOP operations)
 * - ACTUATOR_TYPE: Actuator reference (for SET operations)
 * - ERROR_TYPE: Processing error occurred
 *
 * Example usage:
 * ```cpp
 * DynamicJsonDocument doc(256);
 * deserializeJson(doc, "[\"GT\", [\"getTemperature\"], 25]");
 * UnifiedValue result = processNewRuleCore(doc, env);
 * if (result.type == UnifiedValue::FLOAT_TYPE) {
 *     bool tempHigh = (result.asFloat() > 0); // true if temperature > 25
 * }
 * ```
 */
UnifiedValue processRuleCore(JsonVariantConst doc, const RuleCoreEnv &env);

/**
 * @brief Process a set of rules with automatic relay control (registry version)
 * @param rules Array of rule strings in JSON format
 * @param ruleCount Number of rules in the array
 * @param env Environment context for rule execution
 *
 * This function maintains the same automatic relay control behavior as the original
 * but uses the new function registry system. See original documentation for details
 * on the automatic relay control logic.
 *
 * Note: Rules must use the new unified syntax with explicit function calls.
 */
void processRuleSet(const std::string rules[], int ruleCount, const RuleCoreEnv &env);
