/**
 * @file new_core.cpp
 * @brief Platform-neutral core rule processing engine with function registry (UnifiedValue version)
 *
 * This file implements the core rule processing logic for the Sunroom2 automation system
 * using a pluggable function registry system. It processes rules expressed in a LISP-like JSON
 * syntax where all operations are function calls, providing better extensibility and modularity.
 *
 * The engine is designed to be platform-neutral and can run on Arduino/ESP32, native systems,
 * or in unit tests by providing appropriate function registration callbacks.
 *
 * Key features:
 * - Unified function system: everything is a function call
 * - Pluggable function registry for extensibility
 * - Recursive expression evaluation
 * - Type-safe sensor/actuator abstraction
 * - Comprehensive error reporting
 * - Short-circuit logical evaluation
 * - Time literal support (@HH:MM:SS format)
 * - Unified value system for all data types
 */

#include "core.h"
#include <cstring>
#include <string>
#include "time_helpers.h"

/**
 * @brief Core recursive rule processing function with function registry
 *
 * This function processes JSON expressions in LISP-like syntax using a pluggable
 * function registry. Unlike the original version, ALL operations are function calls,
 * providing a unified and extensible evaluation model.
 *
 * The function operates in two main modes:
 * 1. Literal evaluation: Process non-array JSON values (numbers, booleans, strings)
 * 2. Function evaluation: Process array-based function calls via registry lookup
 *
 * @param doc JSON variant containing the expression to evaluate
 * @param env Environment providing function registry and actuator access
 * @return UnifiedValue containing the result, error information, or actuator reference
 */
UnifiedValue processRuleCore(JsonVariantConst doc, const RuleCoreEnv& env) {
    // LITERAL EVALUATION SECTION
    // Handle non-array JSON values: strings, numbers, booleans
    if (!doc.is<JsonArrayConst>()) {
        // STRING LITERAL PROCESSING
        if (doc.is<const char*>()) {
            const char* cstr = doc.as<const char*>();
            std::string str = cstr ? std::string(cstr) : std::string();

            // TIME LITERAL: "@HH:MM:SS" format
            if (jsonIsTimeLiteral(str)) {
                int secs = parseTimeLiteral(str);
                return secs < 0 ? UnifiedValue::createError(TIME_ERROR) : UnifiedValue(secs);
            }

            // ACTUATOR REFERENCE: Try to resolve as actuator name
            // Returns ACTUATOR_TYPE with setter function for use in SET operations
            if (env.tryGetActuator) {
                std::function<void(float)> setter;
                if (env.tryGetActuator(str, setter) && setter) {
                    return UnifiedValue::createActuator(setter);
                }
            }

            // UNKNOWN STRING: In the new system, all value reading should be explicit function
            // calls Strings that aren't time literals or actuators are considered unknown
            return UnifiedValue::createError(UNREC_STR_ERROR);
        }
        // BOOLEAN LITERAL: true/false converted to 1.0/0.0
        else if (doc.is<bool>()) {
            return UnifiedValue(doc.as<bool>() ? 1.0f : 0.0f);
        }
        // INTEGER LITERAL: Converted to int for unified numeric system
        else if (doc.is<int>()) {
            return UnifiedValue(doc.as<int>());
        }
        // FLOAT LITERAL: Used directly
        else if (doc.is<float>()) {
            return UnifiedValue(doc.as<float>());
        }
        // UNKNOWN TYPE: JSON type not supported by the rule engine
        else {
            return UnifiedValue::createError(UNREC_TYPE_ERROR);
        }
    }

    // FUNCTION EVALUATION SECTION
    // Handle array-based function calls: ["FUNCTION_NAME", arg1, arg2, ...]
    JsonArrayConst array = doc.as<JsonArrayConst>();
    const char* functionName = array[0].as<const char*>();
    if (!functionName) {
        return UnifiedValue::createError(UNREC_FUNC_ERROR);
    }

    // FUNCTION REGISTRY LOOKUP
    // Create a temporary registry and populate it via the environment callback
    // Note: In a production implementation, you might want to cache the registry
    // rather than rebuilding it on every call, but this approach is simpler for now
    FunctionRegistry registry;
    if (env.registerFunctions) {
        env.registerFunctions(registry);
    }

    // Look up the function in the registry
    auto functionIt = registry.find(std::string(functionName));
    if (functionIt == registry.end()) {
        return UnifiedValue::createError(FUNCTION_NOT_FOUND);
    }

    // FUNCTION EXECUTION
    // Call the registered function with the full argument array and environment
    return functionIt->second(array, env);
}

/**
 * @brief Process multiple rules with automatic relay control (registry version)
 *
 * This function maintains the same behavior as the original processRuleSet but uses
 * the new function registry system. It processes an array of rule strings, providing
 * automatic relay control based on rule results.
 *
 * See original processRuleSet documentation for details on automatic relay control logic.
 *
 * @param rules Array of JSON rule strings to process
 * @param ruleCount Number of rules in the array
 * @param env Environment context providing function registry and actuator access
 */
void processRuleSet(const std::string rules[], int ruleCount, const RuleCoreEnv& env) {
    // Process each rule in sequence
    for (int i = 0; i < ruleCount; i++) {
        // RELAY INITIALIZATION: Set relay to "don't care" mode before processing
        // This allows the rule to take full control of the relay state
        if (env.tryGetActuator) {
            std::function<void(float)> setActuator;
            std::string actuatorName = "relay_" + std::to_string(i);
            if (env.tryGetActuator(actuatorName, setActuator) && setActuator) {
                setActuator(2.0f);  // "don't care" mode
            }
        }

        // RULE PARSING: Convert JSON string to document
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, rules[i].c_str());

        if (error) {
// PARSE ERROR HANDLING: Log error and skip to next rule
// Only log on Arduino platform to avoid dependencies in test environment
#ifdef ARDUINO
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
#endif
            continue;
        }

        // RULE EXECUTION: Process the parsed rule using the new registry system
        UnifiedValue result = processRuleCore(doc.as<JsonVariantConst>(), env);

        // RESULT HANDLING: Different actions based on return type (same as original)
        if (result.type == UnifiedValue::FLOAT_TYPE || result.type == UnifiedValue::INT_TYPE) {
// AUTOMATIC RELAY CONTROL: Set corresponding relay to the result value
// This implements the "simple rule" pattern where expressions like
// ["GT", ["getTemperature"], 25] automatically control relay_i
#ifdef ARDUINO
            Serial.println("Setting actuator: " + String(i) + " to: " + String(result.asFloat()));
#endif

            if (env.tryGetActuator) {
                std::function<void(float)> setter;
                std::string actuatorName = "relay_" + std::to_string(i);
                if (env.tryGetActuator(actuatorName, setter) && setter) {
                    setter(result.asFloat());
                }
            }
        } else if (result.type != UnifiedValue::VOID_TYPE) {
// ERROR HANDLING: Log unexpected results (errors, unknown types)
// VOID_TYPE is expected for SET/NOP operations and doesn't trigger logging
#ifdef ARDUINO
            Serial.println("Unexpected rule result: ");
            Serial.println("\ttype: " + String(static_cast<int>(result.type)));
            Serial.println("\terrorCode: " + String(static_cast<int>(result.errorCode)));
            Serial.println("\tval: " + String(result.asFloat()));
#endif
        }
        // VOID_TYPE results are successful no-ops (from SET, NOP), continue silently
        // These indicate explicit actuator control was performed within the rule
    }
}
