/**
 * @file rule_core.cpp
 * @brief Platform-neutral core rule processing engine implementation
 *
 * This file implements the core rule processing logic for the Sunroom2 automation system.
 * It processes rules expressed in a LISP-like JSON syntax, supporting sensors, actuators,
 * comparisons, logical operations, and control flow.
 *
 * The engine is designed to be platform-neutral and can run on Arduino/ESP32, native systems,
 * or in unit tests by providing appropriate environment callbacks.
 *
 * Key features:
 * - Recursive expression evaluation
 * - Type-safe operations with error handling
 * - Short-circuit evaluation for logical operations
 * - Unified numeric system (all values stored as floats)
 * - Time literal support (@HH:MM:SS format)
 * - Sensor reading and actuator control abstraction
 */

#include "core.h"
#include <cstring>
#include <string>
#include "value_types.h"

/**
 * @brief Create a RuleReturn structure with specified parameters
 * @param type The type code for the return value
 * @param errorCode Error code (NO_ERROR if successful)
 * @param val Numeric value (meaningful for FLOAT_TYPE)
 * @return Constructed RuleReturn structure
 *
 * This is the low-level constructor for all RuleReturn values.
 * The actuatorSetter is always initialized to nullptr.
 */
static RuleReturn createRuleReturn(TypeCode type, ErrorCode errorCode, float val) {
    return {type, errorCode, val, nullptr};
}

/**
 * @brief Create an error RuleReturn with specified error code
 * @param error The specific error that occurred
 * @return RuleReturn with ERROR_TYPE and the specified error code
 *
 * Used throughout the engine to report various types of errors:
 * - UNREC_TYPE_ERROR: Unknown JSON type
 * - UNREC_FUNC_ERROR: Unknown function name
 * - UNREC_STR_ERROR: Unknown string literal (sensor/actuator)
 * - TIME_ERROR: Time parsing failed
 * - Various operation-specific errors
 */
static RuleReturn createErrorRuleReturn(ErrorCode error) {
    return createRuleReturn(ERROR_TYPE, error, 0.0f);
}

/**
 * @brief Create a successful float RuleReturn
 * @param v The float value to return
 * @return RuleReturn with FLOAT_TYPE and the specified value
 *
 * Used for:
 * - Sensor readings
 * - Comparison results (1.0 for true, 0.0 for false)
 * - Arithmetic operations
 * - Time values in seconds
 */
static RuleReturn createFloatRuleReturn(float v) {
    return createRuleReturn(FLOAT_TYPE, NO_ERROR, v);
}

/**
 * @brief Create a successful float RuleReturn from an integer
 * @param v The integer value to convert and return
 * @return RuleReturn with FLOAT_TYPE and the converted value
 *
 * Used for integer literals and time values. All numeric values
 * are internally represented as floats for consistency.
 */
static RuleReturn createIntRuleReturn(int v) {
    return createFloatRuleReturn(static_cast<float>(v));
}

/**
 * @brief Create a void RuleReturn for operations with no return value
 * @return RuleReturn with VOID_TYPE indicating successful completion
 *
 * Used for:
 * - SET operations (after they complete successfully)
 * - NOP operations
 * - Operations that perform side effects but don't return values
 */
static RuleReturn createVoidRuleReturn() { return createRuleReturn(VOID_TYPE, NO_ERROR, 0.0f); }

/**
 * @brief Create a boolean RuleReturn
 * @param b The boolean value to convert
 * @return RuleReturn with FLOAT_TYPE, 1.0 for true, 0.0 for false
 *
 * The rule engine uses a unified numeric system where booleans
 * are represented as floats: true = 1.0, false = 0.0
 *
 * This allows boolean results to be used in numeric comparisons
 * and enables consistent type handling throughout the system.
 */
static RuleReturn createBoolRuleReturn(bool b) { return createFloatRuleReturn(b ? 1.0f : 0.0f); }

/**
 * @brief Check if a string is a time literal
 * @param s The string to check
 * @return true if the string starts with '@', indicating a time literal
 *
 * Time literals are in the format "@HH:MM:SS" (e.g., "@14:30:00").
 * They are used in rules to compare against current time:
 * ["GT", "currentTime", "@18:00:00"]
 */
static bool jsonIsTimeLiteral(const std::string &s) { return !s.empty() && s[0] == '@'; }

/**
 * @brief Parse time literal string to seconds since midnight
 * @param timeStr Time string in "@HH:MM:SS" format (e.g., "@14:30:00")
 * @return Time in seconds since midnight, or -1 if parsing failed
 *
 * Platform-neutral implementation that only uses standard C++ libraries.
 * Used to parse time literals in rules like ["GT", "currentTime", "@18:00:00"]
 *
 * Examples:
 * - "@14:30:00" returns (14*3600 + 30*60 + 0) = 52200
 * - "@23:59:59" returns (23*3600 + 59*60 + 59) = 86399
 * - "@00:00:00" returns 0
 * - Invalid formats return -1
 */
static int parseTimeLiteral(const std::string &timeStr) {
    // Check minimum length: "@HH:MM:SS" = 9 characters
    if (timeStr.length() != 9) {
        return -1;
    }

    // Check format: must start with '@' and have colons at positions 3 and 6
    if (timeStr[0] != '@' || timeStr[3] != ':' || timeStr[6] != ':') {
        return -1;
    }

    // Extract and validate hour, minute, second strings
    std::string hourStr = timeStr.substr(1, 2);
    std::string minuteStr = timeStr.substr(4, 2);
    std::string secondStr = timeStr.substr(7, 2);

    // Check that all characters are digits
    for (char c : hourStr)
        if (c < '0' || c > '9') return -1;
    for (char c : minuteStr)
        if (c < '0' || c > '9') return -1;
    for (char c : secondStr)
        if (c < '0' || c > '9') return -1;

    // Convert to integers
    int hours = std::stoi(hourStr);
    int minutes = std::stoi(minuteStr);
    int seconds = std::stoi(secondStr);

    // Validate ranges
    if (hours < 0 || hours > 23) return -1;
    if (minutes < 0 || minutes > 59) return -1;
    if (seconds < 0 || seconds > 59) return -1;

    // Convert to seconds since midnight
    return (hours * 3600) + (minutes * 60) + seconds;
}

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
 * @return RuleReturn containing the result, error information, or actuator reference
 */
RuleReturn processRuleCore(JsonVariantConst doc, const RuleCoreEnv &env) {
    // Pre-create a void return for operations that don't return values
    RuleReturn voidReturn = createVoidRuleReturn();

    // LITERAL EVALUATION SECTION
    // Handle non-array JSON values: strings, numbers, booleans
    if (!doc.is<JsonArrayConst>()) {
        // STRING LITERAL PROCESSING
        if (doc.is<const char *>()) {
            const char *cstr = doc.as<const char *>();
            std::string str = cstr ? std::string(cstr) : std::string();

            // TIME LITERAL: "@HH:MM:SS" format
            if (jsonIsTimeLiteral(str)) {
                int secs = parseTimeLiteral(str);
                return secs < 0 ? createErrorRuleReturn(TIME_ERROR) : createIntRuleReturn(secs);
            }

            // ACTUATOR REFERENCE: Try to resolve as actuator name
            // Returns BOOL_ACTUATOR_TYPE with setter function for use in SET operations
            if (env.tryGetActuator) {
                std::function<void(float)> setter;
                if (env.tryGetActuator(str, setter) && setter) {
                    RuleReturn r = createRuleReturn(BOOL_ACTUATOR_TYPE, NO_ERROR, 0.0f);
                    r.actuatorSetter = setter;
                    return r;
                }
            }

            // VALUE READING: Try to read value (sensors, computed values, etc.)
            if (env.tryReadValue) {
                SensorValue val(0.0f);
                if (env.tryReadValue(str, val)) {
                    // Convert SensorValue to float for backward compatibility
                    return createFloatRuleReturn(val.asFloat());
                }
            }

            // UNKNOWN STRING: Not a time literal, value, or actuator
            return createErrorRuleReturn(UNREC_STR_ERROR);
        }
        // BOOLEAN LITERAL: true/false converted to 1.0/0.0
        else if (doc.is<bool>()) {
            return createBoolRuleReturn(doc.as<bool>());
        }
        // INTEGER LITERAL: Converted to float for unified numeric system
        else if (doc.is<int>()) {
            return createIntRuleReturn(doc.as<int>());
        }
        // FLOAT LITERAL: Used directly
        else if (doc.is<float>()) {
            return createFloatRuleReturn(doc.as<float>());
        }
        // UNKNOWN TYPE: JSON type not supported by the rule engine
        else {
            return createErrorRuleReturn(UNREC_TYPE_ERROR);
        }
    }

    // FUNCTION CALL PROCESSING SECTION
    // Handle array-based function calls: ["FUNCTION_NAME", arg1, arg2, ...]
    JsonArrayConst array = doc.as<JsonArrayConst>();
    const char *type = array[0].as<const char *>();
    if (!type) {
        return createErrorRuleReturn(UNREC_FUNC_ERROR);
    }

    // NOP OPERATION: No-operation, returns void
    // Syntax: ["NOP"]
    // Use case: Placeholder or debugging
    if (std::strcmp(type, "NOP") == 0) {
        return voidReturn;
    }

    // CONDITIONAL OPERATION: If-then-else
    // Syntax: ["IF", condition, then_expr, else_expr]
    // Evaluates condition; if truthy (> 0), executes then_expr, otherwise else_expr
    // Example: ["IF", ["GT", "temperature", 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
    else if (std::strcmp(type, "IF") == 0) {
        RuleReturn cond = processRuleCore(array[1], env);
        if (cond.type == ERROR_TYPE) return cond;
        if (cond.type != FLOAT_TYPE) return createErrorRuleReturn(IF_CONDITION_ERROR);
        return cond.val > 0 ? processRuleCore(array[2], env) : processRuleCore(array[3], env);
    }

    // SET OPERATION: Set actuator to a value
    // Syntax: ["SET", actuator_name, value]
    // First arg must resolve to BOOL_ACTUATOR_TYPE, second to FLOAT_TYPE
    // Calls the actuator's setter function with the specified value
    // Example: ["SET", "relay_0", 1] - turns on relay_0
    else if (std::strcmp(type, "SET") == 0) {
        RuleReturn act = processRuleCore(array[1], env);
        RuleReturn val = processRuleCore(array[2], env);
        if (act.type == ERROR_TYPE) return act;
        if (val.type == ERROR_TYPE) return val;
        if (act.type != BOOL_ACTUATOR_TYPE || val.type != FLOAT_TYPE)
            return createErrorRuleReturn(BOOL_ACTUATOR_ERROR);
        if (act.actuatorSetter) act.actuatorSetter(val.val);
        return voidReturn;
    }

    // LOGICAL OPERATIONS: AND/OR with short-circuit evaluation
    // Syntax: ["AND", expr1, expr2] or ["OR", expr1, expr2]
    // Both expressions must evaluate to FLOAT_TYPE (numeric/boolean)
    // Short-circuit: AND stops if first is false, OR stops if first is true
    // Returns 1.0 for true, 0.0 for false
    else if (std::strcmp(type, "AND") == 0 || std::strcmp(type, "OR") == 0) {
        // Evaluate first operand
        RuleReturn a = processRuleCore(array[1], env);
        if (a.type == ERROR_TYPE) return a;

        // Short-circuit evaluation
        if (std::strcmp(type, "AND") == 0 && !(a.val > 0)) return createBoolRuleReturn(false);
        if (std::strcmp(type, "OR") == 0 && (a.val > 0)) return createBoolRuleReturn(true);

        // Evaluate second operand only if needed
        RuleReturn b = processRuleCore(array[2], env);
        if (a.type == ERROR_TYPE) return a;  // Note: this checks 'a' again due to potential bug
        if (b.type == ERROR_TYPE) return b;
        if (a.type != FLOAT_TYPE || b.type != FLOAT_TYPE)
            return createErrorRuleReturn(AND_OR_ERROR);

        // Perform the logical operation
        bool result =
            (std::strcmp(type, "AND") == 0) ? (a.val > 0 && b.val > 0) : (a.val > 0 || b.val > 0);
        return createBoolRuleReturn(result);
    }

    // NOT OPERATION: Logical negation
    // Syntax: ["NOT", expr]
    // Expression must evaluate to FLOAT_TYPE
    // Returns 1.0 if expr is falsy (≤ 0), 0.0 if expr is truthy (> 0)
    else if (std::strcmp(type, "NOT") == 0) {
        RuleReturn a = processRuleCore(array[1], env);
        if (a.type == ERROR_TYPE) return a;
        if (a.type != FLOAT_TYPE) return createErrorRuleReturn(NOT_ERROR);
        return createBoolRuleReturn(!(a.val > 0));
    }

    // COMPARISON OPERATIONS: EQ, NE, GT, LT, GTE, LTE
    // Syntax: ["OP", expr1, expr2] where OP is one of the comparison operators
    // Both expressions must evaluate to FLOAT_TYPE
    // Returns 1.0 for true comparison, 0.0 for false
    // Examples:
    //   ["GT", "temperature", 25] - true if temperature > 25
    //   ["EQ", "lightSwitch", 1] - true if switch is on
    //   ["LTE", "humidity", 80] - true if humidity ≤ 80
    else if (std::strcmp(type, "EQ") == 0 || std::strcmp(type, "NE") == 0 ||
             std::strcmp(type, "GT") == 0 || std::strcmp(type, "LT") == 0 ||
             std::strcmp(type, "GTE") == 0 || std::strcmp(type, "LTE") == 0) {
        RuleReturn a = processRuleCore(array[1], env);
        RuleReturn b = processRuleCore(array[2], env);
        if (a.type == ERROR_TYPE) return a;
        if (b.type == ERROR_TYPE) return b;
        if (a.type != FLOAT_TYPE || b.type != FLOAT_TYPE)
            return createErrorRuleReturn(COMPARISON_TYPE_EQUALITY_ERROR);

        // Perform the appropriate comparison
        bool res = false;
        if (std::strcmp(type, "EQ") == 0)
            res = a.val == b.val;  // Equal
        else if (std::strcmp(type, "NE") == 0)
            res = a.val != b.val;  // Not equal
        else if (std::strcmp(type, "GT") == 0)
            res = a.val > b.val;  // Greater than
        else if (std::strcmp(type, "LT") == 0)
            res = a.val < b.val;  // Less than
        else if (std::strcmp(type, "GTE") == 0)
            res = a.val >= b.val;  // Greater than or equal
        else if (std::strcmp(type, "LTE") == 0)
            res = a.val <= b.val;  // Less than or equal

        return createBoolRuleReturn(res);
    }

    // UNKNOWN FUNCTION: Function name not recognized
    return createErrorRuleReturn(UNREC_FUNC_ERROR);
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
 * 3. If result is FLOAT_TYPE, set relay_i to that value (typically 0.0 or 1.0)
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
void processRuleSet(const std::string rules[], int ruleCount, const RuleCoreEnv &env) {
    // Process each rule in sequence
    for (int i = 0; i < ruleCount; i++) {
        // RELAY INITIALIZATION: Set relay to "don't care" mode before processing
        // This allows the rule to take full control of the relay state
        // Value 2.0 means "auto mode" - neither forced on nor forced off
        if (env.tryGetActuator) {
            std::function<void(float)> setter;
            if (env.tryGetActuator("relay_" + std::to_string(i), setter) && setter) {
                setter(2.0f);
            }
        }

        // JSON PARSING: Convert rule string to JSON document
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

        // RULE EXECUTION: Process the parsed rule using the core engine
        RuleReturn result = processRuleCore(doc, env);

        // RESULT HANDLING: Different actions based on return type
        if (result.type == FLOAT_TYPE) {
// AUTOMATIC RELAY CONTROL: Set corresponding relay to the result value
// This implements the "simple rule" pattern where expressions like
// ["GT", "temperature", 25] automatically control relay_i
#ifdef ARDUINO
            Serial.println("Setting actuator: " + String(i) + " to: " + String(result.val));
#endif

            if (env.tryGetActuator) {
                std::function<void(float)> setter;
                if (env.tryGetActuator("relay_" + std::to_string(i), setter) && setter) {
                    setter(result.val);
                }
            }
        } else if (result.type != VOID_TYPE) {
// ERROR HANDLING: Log unexpected results (errors, unknown types)
// VOID_TYPE is expected for SET/NOP operations and doesn't trigger logging
#ifdef ARDUINO
            Serial.println("Unexpected rule result: ");
            // Note: printRuleReturn is Arduino-specific, keep in rule_helpers.cpp
            Serial.println("RuleReturn:");
            Serial.println("\ttype: " + String(result.type));
            Serial.println("\terrorCode: " + String(result.errorCode));
            Serial.println("\tval: " + String(result.val));
#endif
        }
        // VOID_TYPE results are successful no-ops (from SET, NOP), continue silently
        // These indicate explicit actuator control was performed within the rule
    }
}
