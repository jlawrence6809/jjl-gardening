#pragma once

#include <cstring>
#include <functional>
#include <string>

/**
 * @file unified_value.h
 * @brief Unified value type system for automation DSL
 *
 * This file implements a single value type that can represent both input values
 * (sensor readings, user inputs) and execution results (rule outcomes, errors)
 * with consistent error handling and type safety.
 *
 * Key features:
 * - Unified type system for all automation DSL values
 * - Consistent error handling across sensor reads and rule evaluation
 * - Type-safe storage of multiple value types in a single union
 * - Automatic type conversion for compatibility
 * - Support for actuator functions with runtime safety
 */

/**
 * @enum ErrorCode
 * @brief Unified error reporting for all automation DSL operations
 */
enum ErrorCode {
    NO_ERROR = 0,

    // Value conversion errors
    PARSE_ERROR,              ///< String->number conversion failed
    TYPE_CONVERSION_ERROR,    ///< Invalid type conversion requested

    // Rule execution errors
    UNREC_TYPE_ERROR,         ///< Unknown JSON type
    UNREC_FUNC_ERROR,         ///< Unknown function name
    UNREC_STR_ERROR,          ///< Unknown string identifier
    IF_CONDITION_ERROR,       ///< IF condition not boolean
    BOOL_ACTUATOR_ERROR,      ///< SET operation type mismatch
    AND_OR_ERROR,             ///< AND/OR operand type error
    NOT_ERROR,                ///< NOT operand type error
    COMPARISON_TYPE_ERROR,    ///< Comparison operand type error
    TIME_ERROR,               ///< Time literal parsing error
    UNREC_ACTUATOR_ERROR,     ///< Unknown actuator name

    // Sensor/hardware errors (future)
    SENSOR_READ_ERROR,        ///< Hardware sensor failure
    ACTUATOR_SET_ERROR        ///< Hardware actuator failure
};

/**
 * @struct UnifiedValue
 * @brief Unified value type for all automation DSL operations
 *
 * This structure can represent input values (sensor readings), execution results
 * (rule outcomes), errors, and actuator functions with consistent error handling
 * and type safety.
 *
 * **Design Philosophy:**
 * Everything in the automation DSL is conceptually a "function result" - whether
 * reading a sensor value like "temperature" or evaluating a rule like
 * ["GT", "temperature", 20]. Both return values with potential errors.
 *
 * **Memory Layout:**
 * - Type enum: 4 bytes (aligned)
 * - ErrorCode: 4 bytes (aligned)
 * - Union storage: ~24 bytes (std::function object size)
 * - Total: ~32 bytes per value + heap data for strings/functions
 *
 * **Usage Examples:**
 * ```cpp
 * UnifiedValue temp(25.5f);                    // Float sensor value
 * UnifiedValue count(42);                      // Integer sensor value
 * UnifiedValue status("connected");            // String sensor value
 * UnifiedValue result = processRule(...);     // Rule evaluation result
 * UnifiedValue error = createError(PARSE_ERROR); // Error result
 *
 * if (temp.type == ERROR_TYPE) {
 *     Serial.println("Error: " + String(temp.errorCode));
 * } else {
 *     float value = temp.asFloat();
 * }
 * ```
 */
struct UnifiedValue {
    /**
     * @enum Type
     * @brief Enumeration of all possible value types in the automation DSL
     */
    enum Type {
        // Value types (input data)
        FLOAT_TYPE,       ///< Floating-point values (temperature, humidity, etc.)
        INT_TYPE,         ///< Integer values (counts, time, discrete states)
        STRING_TYPE,      ///< String values (status, modes, messages)

        // Execution types (results)
        VOID_TYPE,        ///< Successful operations with no return value
        ACTUATOR_TYPE,    ///< Actuator reference with setter function
        ERROR_TYPE        ///< Any kind of error
    } type;

    /**
     * @brief Error code for detailed error information
     *
     * Only meaningful when type == ERROR_TYPE. Provides specific error
     * information for debugging and error handling.
     */
    ErrorCode errorCode;

    /**
     * @union ValueUnion
     * @brief Union for storage of different value types
     */
    union ValueUnion {
        float f;                           ///< Float value storage
        int i;                             ///< Integer value storage
        std::string s;                     ///< String value storage (owned copy)
        std::function<void(float)> fn;     ///< Actuator setter function

        // Default constructor for union
        ValueUnion() : f(0.0f) {}
        // Destructor needed for complex types in union
        ~ValueUnion() {}
    } value;

    // Constructors for different value types

    /**
     * @brief Construct UnifiedValue from float
     * @param val Float value to store
     */
    UnifiedValue(float val) : type(FLOAT_TYPE), errorCode(NO_ERROR) { value.f = val; }

    /**
     * @brief Construct UnifiedValue from integer
     * @param val Integer value to store
     */
    UnifiedValue(int val) : type(INT_TYPE), errorCode(NO_ERROR) { value.i = val; }

    /**
     * @brief Construct UnifiedValue from C string
     * @param val C string to copy and store
     */
    UnifiedValue(const char* val) : type(STRING_TYPE), errorCode(NO_ERROR) {
        new (&value.s) std::string(val ? val : "");
    }

    /**
     * @brief Construct UnifiedValue from std::string
     * @param val String to copy and store
     */
    UnifiedValue(const std::string& val) : type(STRING_TYPE), errorCode(NO_ERROR) {
        new (&value.s) std::string(val);
    }

    /**
     * @brief Construct UnifiedValue from actuator function
     * @param fn Actuator setter function
     */
    UnifiedValue(const std::function<void(float)>& fn) : type(ACTUATOR_TYPE), errorCode(NO_ERROR) {
        new (&value.fn) std::function<void(float)>(fn);
    }

    /**
     * @brief Copy constructor
     */
    UnifiedValue(const UnifiedValue& other) : type(other.type), errorCode(other.errorCode) {
        switch (type) {
            case FLOAT_TYPE:
                value.f = other.value.f;
                break;
            case INT_TYPE:
                value.i = other.value.i;
                break;
            case STRING_TYPE:
                new (&value.s) std::string(other.value.s);
                break;
            case ACTUATOR_TYPE:
                new (&value.fn) std::function<void(float)>(other.value.fn);
                break;
            case VOID_TYPE:
            case ERROR_TYPE:
                // No additional data to copy
                break;
        }
    }

    /**
     * @brief Assignment operator
     */
    UnifiedValue& operator=(const UnifiedValue& other) {
        if (this != &other) {
            // Destroy current value if complex type
            destroyValue();

            type = other.type;
            errorCode = other.errorCode;

            switch (type) {
                case FLOAT_TYPE:
                    value.f = other.value.f;
                    break;
                case INT_TYPE:
                    value.i = other.value.i;
                    break;
                case STRING_TYPE:
                    new (&value.s) std::string(other.value.s);
                    break;
                case ACTUATOR_TYPE:
                    new (&value.fn) std::function<void(float)>(other.value.fn);
                    break;
                case VOID_TYPE:
                case ERROR_TYPE:
                    // No additional data to copy
                    break;
            }
        }
        return *this;
    }

    /**
     * @brief Destructor - properly destroys complex types if needed
     */
    ~UnifiedValue() { destroyValue(); }

    // Type conversion methods with automatic casting

    /**
     * @brief Convert value to float with automatic type conversion
     * @return Float representation of the value, or 0.0f for errors
     *
     * Conversion rules:
     * - FLOAT_TYPE: Return as-is
     * - INT_TYPE: Cast to float (e.g., 1 -> 1.0f)
     * - STRING_TYPE: Parse as float, return 0.0f if parsing fails
     * - VOID_TYPE/ACTUATOR_TYPE/ERROR_TYPE: Return 0.0f
     */
    float asFloat() const {
        switch (type) {
            case FLOAT_TYPE:
                return value.f;
            case INT_TYPE:
                return static_cast<float>(value.i);
            case STRING_TYPE:
                return parseStringAsFloat(value.s.c_str());
            case VOID_TYPE:
            case ACTUATOR_TYPE:
            case ERROR_TYPE:
                return 0.0f;
        }
        return 0.0f; // Fallback
    }

    /**
     * @brief Convert value to integer with automatic type conversion
     * @return Integer representation of the value, or 0 for errors
     *
     * Conversion rules:
     * - INT_TYPE: Return as-is
     * - FLOAT_TYPE: Truncate to integer (e.g., 1.6f -> 1)
     * - STRING_TYPE: Parse as integer using two-stage parsing
     * - VOID_TYPE/ACTUATOR_TYPE/ERROR_TYPE: Return 0
     */
    int asInt() const {
        switch (type) {
            case INT_TYPE:
                return value.i;
            case FLOAT_TYPE:
                return static_cast<int>(value.f);
            case STRING_TYPE:
                return parseStringAsInt(value.s.c_str());
            case VOID_TYPE:
            case ACTUATOR_TYPE:
            case ERROR_TYPE:
                return 0;
        }
        return 0; // Fallback
    }

    /**
     * @brief Convert value to string representation
     * @return C string representation of the value
     *
     * Conversion rules:
     * - STRING_TYPE: Return as-is
     * - FLOAT_TYPE/INT_TYPE: Convert to string (static buffer, not thread-safe)
     * - VOID_TYPE: Return "void"
     * - ACTUATOR_TYPE: Return "actuator"
     * - ERROR_TYPE: Return error description
     */
    const char* asString() const {
        switch (type) {
            case STRING_TYPE:
                return value.s.c_str();
            case FLOAT_TYPE:
                return floatToString(value.f);
            case INT_TYPE:
                return intToString(value.i);
            case VOID_TYPE:
                return "void";
            case ACTUATOR_TYPE:
                return "actuator";
            case ERROR_TYPE:
                return errorToString(errorCode);
        }
        return ""; // Fallback
    }

    /**
     * @brief Get actuator setter function
     * @return Actuator function if type is ACTUATOR_TYPE, nullptr otherwise
     */
    std::function<void(float)> getActuatorSetter() const {
        if (type == ACTUATOR_TYPE) {
            return value.fn;
        }
        return nullptr;
    }

    // Comparison operators for use in rules

    /**
     * @brief Equality comparison with automatic type conversion
     * @param other Value to compare against
     * @return true if values are equal (after type conversion)
     */
    bool operator==(const UnifiedValue& other) const {
        // Error types are never equal to anything
        if (type == ERROR_TYPE || other.type == ERROR_TYPE) {
            return false;
        }

        // If both are strings, compare as strings
        if (type == STRING_TYPE && other.type == STRING_TYPE) {
            return value.s == other.value.s;
        }

        // Otherwise compare as floats
        return asFloat() == other.asFloat();
    }

    /**
     * @brief Inequality comparison
     */
    bool operator!=(const UnifiedValue& other) const { return !(*this == other); }

    /**
     * @brief Greater-than comparison
     */
    bool operator>(const UnifiedValue& other) const {
        if (type == ERROR_TYPE || other.type == ERROR_TYPE) return false;
        return asFloat() > other.asFloat();
    }

    /**
     * @brief Less-than comparison
     */
    bool operator<(const UnifiedValue& other) const {
        if (type == ERROR_TYPE || other.type == ERROR_TYPE) return false;
        return asFloat() < other.asFloat();
    }

    /**
     * @brief Greater-than-or-equal comparison
     */
    bool operator>=(const UnifiedValue& other) const {
        if (type == ERROR_TYPE || other.type == ERROR_TYPE) return false;
        return asFloat() >= other.asFloat();
    }

    /**
     * @brief Less-than-or-equal comparison
     */
    bool operator<=(const UnifiedValue& other) const {
        if (type == ERROR_TYPE || other.type == ERROR_TYPE) return false;
        return asFloat() <= other.asFloat();
    }

    // Static factory methods for creating special values

    /**
     * @brief Create a void result (successful operation with no return value)
     */
    static UnifiedValue createVoid() {
        UnifiedValue result;
        result.type = VOID_TYPE;
        result.errorCode = NO_ERROR;
        return result;
    }

    /**
     * @brief Create an error result
     * @param error The specific error that occurred
     */
    static UnifiedValue createError(ErrorCode error) {
        UnifiedValue result;
        result.type = ERROR_TYPE;
        result.errorCode = error;
        return result;
    }

    /**
     * @brief Create an actuator result
     * @param setter Function to control the actuator
     */
    static UnifiedValue createActuator(const std::function<void(float)>& setter) {
        return UnifiedValue(setter);
    }

  private:
    /**
     * @brief Destroy the current value if it's a complex type
     */
    void destroyValue() {
        switch (type) {
            case STRING_TYPE:
                value.s.~basic_string();
                break;
            case ACTUATOR_TYPE:
                value.fn.~function();
                break;
            case FLOAT_TYPE:
            case INT_TYPE:
            case VOID_TYPE:
            case ERROR_TYPE:
                // No cleanup needed
                break;
        }
    }

    /**
     * @brief Parse string as float value with strict validation
     */
    static float parseStringAsFloat(const char* str) {
        if (!str) return 0.0f;

        char* endPtr;
        float result = strtof(str, &endPtr);

        // Strict validation: entire string must convert successfully
        if (endPtr == str || *endPtr != '\0') {
            return 0.0f; // No conversion or partial conversion
        }

        return result;
    }

    /**
     * @brief Parse string as integer value with two-stage parsing
     */
    static int parseStringAsInt(const char* str) {
        if (!str) return 0;

        // Stage 1: Try parsing as pure integer
        char* endPtr;
        long result = strtol(str, &endPtr, 10);

        if (endPtr != str && *endPtr == '\0' && result <= INT_MAX && result >= INT_MIN) {
            // Successfully parsed as pure integer
            return static_cast<int>(result);
        }

        // Stage 2: Try parsing as float, then truncate
        float floatResult = parseStringAsFloat(str);

        // Check if float parsing succeeded (non-zero result or valid zero)
        if (floatResult != 0.0f || isValidZeroFloat(str)) {
            return static_cast<int>(floatResult);
        }

        // Both parsing attempts failed
        return 0;
    }

    /**
     * @brief Check if string represents a valid zero float
     */
    static bool isValidZeroFloat(const char* str) {
        if (!str) return false;

        char* endPtr;
        strtof(str, &endPtr);
        return (endPtr != str && *endPtr == '\0');
    }

    /**
     * @brief Convert float to string representation
     */
    static const char* floatToString(float val) {
        static char buffer[32]; // Static buffer, not thread-safe
        snprintf(buffer, sizeof(buffer), "%.3f", val);
        return buffer;
    }

    /**
     * @brief Convert integer to string representation
     */
    static const char* intToString(int val) {
        static char buffer[16]; // Static buffer, not thread-safe
        snprintf(buffer, sizeof(buffer), "%d", val);
        return buffer;
    }

    /**
     * @brief Convert error code to string representation
     */
    static const char* errorToString(ErrorCode error) {
        switch (error) {
            case NO_ERROR:
                return "no_error";
            case PARSE_ERROR:
                return "parse_error";
            case TYPE_CONVERSION_ERROR:
                return "type_conversion_error";
            case UNREC_TYPE_ERROR:
                return "unrecognized_type_error";
            case UNREC_FUNC_ERROR:
                return "unrecognized_function_error";
            case UNREC_STR_ERROR:
                return "unrecognized_string_error";
            case IF_CONDITION_ERROR:
                return "if_condition_error";
            case BOOL_ACTUATOR_ERROR:
                return "bool_actuator_error";
            case AND_OR_ERROR:
                return "and_or_error";
            case NOT_ERROR:
                return "not_error";
            case COMPARISON_TYPE_ERROR:
                return "comparison_type_error";
            case TIME_ERROR:
                return "time_error";
            case UNREC_ACTUATOR_ERROR:
                return "unrecognized_actuator_error";
            case SENSOR_READ_ERROR:
                return "sensor_read_error";
            case ACTUATOR_SET_ERROR:
                return "actuator_set_error";
            default:
                return "unknown_error";
        }
    }

  private:
    // Default constructor for factory methods
    UnifiedValue() : type(VOID_TYPE), errorCode(NO_ERROR) { value.f = 0.0f; }
};
