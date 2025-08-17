/**
 * @file core.cpp
 * @brief Platform-neutral core rule processing engine implementation (UnifiedValue version)
 *
 * This file implements the core rule processing logic for the Sunroom2 automation system
 * using the new UnifiedValue type system. It processes rules expressed in a LISP-like JSON
 * syntax, supporting sensors, actuators, comparisons, logical operations, and control flow.
 *
 * The engine is designed to be platform-neutral and can run on Arduino/ESP32, native systems,
 * or in unit tests by providing appropriate environment callbacks.
 *
 * Key features:
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
 * @brief Core recursive rule processing function
 *
 * This function processes JSON expressions in LISP-like syntax. It handles both
 * literal values (numbers, booleans, strings) and function calls (arrays).
 *
 * The function operates in two main modes:
 * 1. Literal evaluation: Process non-array JSON values
 * 2. Function evaluation: Process array-based function calls
 *
 * @param doc JSON variant containing the expression to evaluate
 * @param env Environment providing sensor/actuator access and time functions
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

            // VALUE READING: Try to read value (sensors, computed values, etc.)
            if (env.tryReadValue) {
                UnifiedValue val(0.0f);
                if (env.tryReadValue(str, val)) {
                    return val;  // Return the value as-is (could be any type)
                }
            }

            // UNKNOWN STRING: Not a time literal, value, or actuator
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
    const char* type = array[0].as<const char*>();
    if (!type) {
        return UnifiedValue::createError(UNREC_FUNC_ERROR);
    }

    // NO-OPERATION: Placeholder function
    // Syntax: ["NOP"]
    // Use case: Placeholder or debugging
    if (std::strcmp(type, "NOP") == 0) {
        return UnifiedValue::createVoid();
    }

    // CONDITIONAL OPERATION: If-then-else
    // Syntax: ["IF", condition, then_expr, else_expr]
    // Evaluates condition; if truthy (> 0), executes then_expr, otherwise else_expr
    // Example: ["IF", ["GT", "temperature", 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
    else if (std::strcmp(type, "IF") == 0) {
        UnifiedValue cond = processRuleCore(array[1], env);
        if (cond.type == UnifiedValue::ERROR_TYPE) return cond;
        if (cond.type != UnifiedValue::FLOAT_TYPE && cond.type != UnifiedValue::INT_TYPE) {
            return UnifiedValue::createError(IF_CONDITION_ERROR);
        }
        return cond.asFloat() > 0 ? processRuleCore(array[2], env) : processRuleCore(array[3], env);
    }

    // SET OPERATION: Set actuator to a value
    // Syntax: ["SET", actuator_name, value]
    // First arg must resolve to ACTUATOR_TYPE, second to numeric type
    // Calls the actuator's setter function with the specified value
    // Example: ["SET", "relay_0", 1] - turns on relay_0
    else if (std::strcmp(type, "SET") == 0) {
        UnifiedValue act = processRuleCore(array[1], env);
        UnifiedValue val = processRuleCore(array[2], env);
        if (act.type == UnifiedValue::ERROR_TYPE) return act;
        if (val.type == UnifiedValue::ERROR_TYPE) return val;
        if (act.type != UnifiedValue::ACTUATOR_TYPE ||
            (val.type != UnifiedValue::FLOAT_TYPE && val.type != UnifiedValue::INT_TYPE)) {
            return UnifiedValue::createError(BOOL_ACTUATOR_ERROR);
        }
        auto setter = act.getActuatorSetter();
        if (setter) setter(val.asFloat());
        return UnifiedValue::createVoid();
    }

    // LOGICAL OPERATIONS: AND/OR with short-circuit evaluation
    // Syntax: ["AND", expr1, expr2] or ["OR", expr1, expr2]
    // Both expressions must evaluate to numeric types
    // Short-circuit: AND stops if first is false, OR stops if first is true
    // Returns 1.0 for true, 0.0 for false
    else if (std::strcmp(type, "AND") == 0 || std::strcmp(type, "OR") == 0) {
        // Evaluate first operand
        UnifiedValue a = processRuleCore(array[1], env);
        if (a.type == UnifiedValue::ERROR_TYPE) return a;

        // Short-circuit evaluation
        if (std::strcmp(type, "AND") == 0 && !(a.asFloat() > 0)) {
            return UnifiedValue(0.0f);  // false
        }
        if (std::strcmp(type, "OR") == 0 && (a.asFloat() > 0)) {
            return UnifiedValue(1.0f);  // true
        }

        // Evaluate second operand only if needed
        UnifiedValue b = processRuleCore(array[2], env);
        if (b.type == UnifiedValue::ERROR_TYPE) return b;
        if ((a.type != UnifiedValue::FLOAT_TYPE && a.type != UnifiedValue::INT_TYPE) ||
            (b.type != UnifiedValue::FLOAT_TYPE && b.type != UnifiedValue::INT_TYPE)) {
            return UnifiedValue::createError(AND_OR_ERROR);
        }

        // Perform the logical operation
        bool result = (std::strcmp(type, "AND") == 0) ? (a.asFloat() > 0 && b.asFloat() > 0)
                                                      : (a.asFloat() > 0 || b.asFloat() > 0);
        return UnifiedValue(result ? 1.0f : 0.0f);
    }

    // NOT OPERATION: Logical negation
    // Syntax: ["NOT", expr]
    // Expression must evaluate to numeric type
    // Returns 1.0 if expr is falsy (≤ 0), 0.0 if expr is truthy (> 0)
    else if (std::strcmp(type, "NOT") == 0) {
        UnifiedValue a = processRuleCore(array[1], env);
        if (a.type == UnifiedValue::ERROR_TYPE) return a;
        if (a.type != UnifiedValue::FLOAT_TYPE && a.type != UnifiedValue::INT_TYPE) {
            return UnifiedValue::createError(NOT_ERROR);
        }
        return UnifiedValue(!(a.asFloat() > 0) ? 1.0f : 0.0f);
    }

    // COMPARISON OPERATIONS: EQ, NE, GT, LT, GTE, LTE
    // Syntax: ["OP", expr1, expr2] where OP is one of the comparison operators
    // Both expressions must evaluate to numeric types
    // Returns 1.0 for true, 0.0 for false
    // Examples:
    //   ["GT", "temperature", 25] - true if temperature > 25
    //   ["EQ", "lightSwitch", 1] - true if switch is on
    //   ["LTE", "humidity", 80] - true if humidity ≤ 80
    else if (std::strcmp(type, "EQ") == 0 || std::strcmp(type, "NE") == 0 ||
             std::strcmp(type, "GT") == 0 || std::strcmp(type, "LT") == 0 ||
             std::strcmp(type, "GTE") == 0 || std::strcmp(type, "LTE") == 0) {
        UnifiedValue a = processRuleCore(array[1], env);
        UnifiedValue b = processRuleCore(array[2], env);
        if (a.type == UnifiedValue::ERROR_TYPE) return a;
        if (b.type == UnifiedValue::ERROR_TYPE) return b;

        // For EQ and NE, allow string comparisons
        if (std::strcmp(type, "EQ") == 0) {
            return UnifiedValue((a == b) ? 1.0f : 0.0f);
        }
        if (std::strcmp(type, "NE") == 0) {
            return UnifiedValue((a != b) ? 1.0f : 0.0f);
        }

        // For ordering comparisons, require numeric types
        if ((a.type != UnifiedValue::FLOAT_TYPE && a.type != UnifiedValue::INT_TYPE) ||
            (b.type != UnifiedValue::FLOAT_TYPE && b.type != UnifiedValue::INT_TYPE)) {
            return UnifiedValue::createError(COMPARISON_TYPE_ERROR);
        }

        // Perform the appropriate comparison
        bool res = false;
        if (std::strcmp(type, "GT") == 0)
            res = a > b;
        else if (std::strcmp(type, "LT") == 0)
            res = a < b;
        else if (std::strcmp(type, "GTE") == 0)
            res = a >= b;
        else if (std::strcmp(type, "LTE") == 0)
            res = a <= b;

        return UnifiedValue(res ? 1.0f : 0.0f);
    }

    // UNKNOWN FUNCTION: Function name not recognized
    return UnifiedValue::createError(UNREC_FUNC_ERROR);
}

/**
 * @brief Process multiple rules with automatic relay control
 *
 * This function processes an array of rule strings, providing automatic relay control
 * based on rule results. It implements the high-level rule processing workflow used
 * by the main application.
 *
 * AUTOMATIC RELAY CONTROL LOGIC:
 * 1. For rule index i, first set relay_i to "don't care" mode (2.0)
 * 2. Parse and execute the rule
 * 3. If the rule returns a FLOAT_TYPE/INT_TYPE result, automatically sets relay_i to that value
 * 4. If result is VOID_TYPE, no automatic control (explicit SET commands handled)
 * 5. If result is ERROR_TYPE, log error but don't change relay state
 *
 * This enables two rule patterns:
 * - Simple: ["GT", "temperature", 25] → automatic relay control
 * - Explicit: ["SET", "relay_0", 1] → manual relay control
 *
 * @param rules Array of JSON rule strings to process
 * @param ruleCount Number of rules in the array
 * @param env Environment context providing sensor/actuator access
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

        // RULE EXECUTION: Process the parsed rule
        UnifiedValue result = processRuleCore(doc.as<JsonVariantConst>(), env);

        // RESULT HANDLING: Different actions based on return type
        if (result.type == UnifiedValue::FLOAT_TYPE || result.type == UnifiedValue::INT_TYPE) {
// AUTOMATIC RELAY CONTROL: Set corresponding relay to the result value
// This implements the "simple rule" pattern where expressions like
// ["GT", "temperature", 25] automatically control relay_i
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
