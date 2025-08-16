#pragma once

#include <cstring>
#include <string>

/**
 * @file sensor_value.h
 * @brief Variant type system for sensor values supporting floats, integers, and strings
 *
 * This file implements a flexible variant type that can hold different sensor value types
 * while maintaining backward compatibility with the existing float-based system.
 *
 * Key features:
 * - Type-safe storage of float, int, and string values
 * - Automatic type conversion for compatibility
 * - Memory-efficient union storage
 * - Easy integration with existing sensor APIs
 */

/**
 * @struct SensorValue
 * @brief Variant type for sensor values
 *
 * This structure can hold float, integer, or string values with automatic
 * type conversion to maintain compatibility with existing code.
 *
 * **IMPORTANT: String values are COPIED and owned by the SensorValue object.**
 * This prevents dangling pointer issues but uses more memory than pointer storage.
 * The std::string implementation uses Small String Optimization (SSO) for short
 * strings and heap allocation for longer ones.
 *
 * Memory layout:
 * - Type enum: 4 bytes (aligned)
 * - Union storage: ~24 bytes (std::string object size)
 * - Total: ~28 bytes per value + string data for long strings
 *
 * Usage examples:
 * ```cpp
 * SensorValue temp(25.5f);        // Float sensor
 * SensorValue count(42);          // Integer sensor
 * SensorValue status("connected"); // String sensor
 *
 * float tempVal = temp.asFloat(); // 25.5
 * bool isConnected = (status.asString() == std::string("connected"));
 * ```
 */
struct SensorValue {
    /**
     * @enum Type
     * @brief Enumeration of supported sensor value types
     */
    enum Type {
        FLOAT,  ///< Floating-point sensor value (temperature, humidity, etc.)
        INT,    ///< Integer sensor value (counts, time, discrete states)
        STRING  ///< String sensor value (status, modes, error messages)
    } type;

    /**
     * @union ValueUnion
     * @brief Union for storage of different value types
     */
    union ValueUnion {
        float f;        ///< Float value storage
        int i;          ///< Integer value storage
        std::string s;  ///< String value storage (owned copy)

        // Default constructor for union
        ValueUnion() : f(0.0f) {}
        // Destructor needed for std::string in union
        ~ValueUnion() {}
    } value;

    // Constructors for easy creation from different types

    /**
     * @brief Construct SensorValue from float
     * @param val Float value to store
     */
    SensorValue(float val) : type(FLOAT) { value.f = val; }

    /**
     * @brief Construct SensorValue from integer
     * @param val Integer value to store
     */
    SensorValue(int val) : type(INT) { value.i = val; }

    /**
     * @brief Construct SensorValue from C string
     * @param val C string to copy and store
     */
    SensorValue(const char* val) : type(STRING) { new (&value.s) std::string(val ? val : ""); }

    /**
     * @brief Construct SensorValue from std::string
     * @param val String to copy and store
     */
    SensorValue(const std::string& val) : type(STRING) { new (&value.s) std::string(val); }

    /**
     * @brief Copy constructor
     */
    SensorValue(const SensorValue& other) : type(other.type) {
        switch (type) {
            case FLOAT:
                value.f = other.value.f;
                break;
            case INT:
                value.i = other.value.i;
                break;
            case STRING:
                new (&value.s) std::string(other.value.s);
                break;
        }
    }

    /**
     * @brief Assignment operator
     */
    SensorValue& operator=(const SensorValue& other) {
        if (this != &other) {
            // Destroy current value if string
            if (type == STRING) {
                value.s.~basic_string();
            }

            type = other.type;
            switch (type) {
                case FLOAT:
                    value.f = other.value.f;
                    break;
                case INT:
                    value.i = other.value.i;
                    break;
                case STRING:
                    new (&value.s) std::string(other.value.s);
                    break;
            }
        }
        return *this;
    }

    /**
     * @brief Destructor - properly destroys std::string if needed
     */
    ~SensorValue() {
        if (type == STRING) {
            value.s.~basic_string();
        }
    }

    // Type conversion methods with automatic casting

    /**
     * @brief Convert value to float with automatic type conversion
     * @return Float representation of the value
     *
     * Conversion rules:
     * - FLOAT: Return as-is
     * - INT: Cast to float, eg: 1 -> 1.0f
     * - STRING: Parse as float, return 0.0 if parsing fails
     */
    float asFloat() const {
        switch (type) {
            case FLOAT:
                return value.f;
            case INT:
                return static_cast<float>(value.i);
            case STRING:
                return parseStringAsFloat(value.s.c_str());
        }
        return 0.0f;  // Fallback
    }

    /**
     * @brief Convert value to integer with automatic type conversion
     * @return Integer representation of the value
     *
     * Conversion rules:
     * - INT: Return as-is
     * - FLOAT: Truncate to integer, eg: 1.6f -> 1
     * - STRING: Parse as integer, return 0 if parsing fails
     */
    int asInt() const {
        switch (type) {
            case INT:
                return value.i;
            case FLOAT:
                return static_cast<int>(value.f);
            case STRING:
                return parseStringAsInt(value.s.c_str());
        }
        return 0;  // Fallback
    }

    /**
     * @brief Convert value to string representation
     * @return C string representation of the value
     *
     * Conversion rules:
     * - STRING: Return as-is
     * - FLOAT: Convert to string representation (caller must handle lifetime)
     * - INT: Convert to string representation (caller must handle lifetime)
     *
     * Note: For FLOAT and INT types, this returns a pointer to a static buffer
     * that may be overwritten on subsequent calls. Not thread-safe.
     */
    const char* asString() const {
        switch (type) {
            case STRING:
                return value.s.c_str();
            case FLOAT:
                return floatToString(value.f);
            case INT:
                return intToString(value.i);
        }
        return "";  // Fallback
    }

    // Comparison operators for use in rules

    /**
     * @brief Equality comparison with automatic type conversion
     * @param other Value to compare against
     * @return true if values are equal (after type conversion)
     */
    bool operator==(const SensorValue& other) const {
        // If both are strings, compare as strings
        if (type == STRING && other.type == STRING) {
            return value.s == other.value.s;
        }
        // Otherwise compare as floats
        return asFloat() == other.asFloat();
    }

    /**
     * @brief Inequality comparison
     * @param other Value to compare against
     * @return true if values are not equal
     */
    bool operator!=(const SensorValue& other) const { return !(*this == other); }

    /**
     * @brief Less-than comparison with automatic type conversion
     * @param other Value to compare against
     * @return true if this value is less than other (after conversion to float)
     */
    bool operator<(const SensorValue& other) const { return asFloat() < other.asFloat(); }

    /**
     * @brief Greater-than comparison with automatic type conversion
     * @param other Value to compare against
     * @return true if this value is greater than other (after conversion to float)
     */
    bool operator>(const SensorValue& other) const { return asFloat() > other.asFloat(); }

  private:
    /**
     * @brief Parse string as float value with strict validation
     * @param str C string to parse
     * @return Parsed float value, or 0.0 if parsing fails
     *
     * Uses strict parsing: entire string must be a valid number.
     * Rejects partial conversions like "123abc" or "123 extra".
     */
    static float parseStringAsFloat(const char* str) {
        if (!str) return 0.0f;

        char* endPtr;
        float result = strtof(str, &endPtr);

        // Strict validation: entire string must convert successfully
        if (endPtr == str || *endPtr != '\0') {
            return 0.0f;  // No conversion or partial conversion
        }

        return result;
    }

    /**
     * @brief Parse string as integer value with two-stage parsing
     * @param str C string to parse
     * @return Parsed integer value, or 0 if parsing fails
     *
     * Two-stage parsing approach:
     * 1. First try parsing as pure integer ("123" -> 123)
     * 2. If that fails, try parsing as float then truncate ("123.45" -> 123)
     * This provides consistency: SensorValue(123.45f).asInt() == SensorValue("123.45").asInt()
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
     * @param str C string to check
     * @return true if string is "0", "0.0", "0.00", etc.
     */
    static bool isValidZeroFloat(const char* str) {
        if (!str) return false;

        // Use parseStringAsFloat's validation - if it succeeds and returns 0.0,
        // and the original parseStringAsFloat logic would have succeeded, then it's valid
        char* endPtr;
        strtof(str, &endPtr);
        return (endPtr != str && *endPtr == '\0');
    }

    /**
     * @brief Convert float to string representation
     * @param val Float value to convert
     * @return C string representation (static buffer, not thread-safe)
     */
    static const char* floatToString(float val) {
        static char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.3f", val);
        return buffer;
    }

    /**
     * @brief Convert integer to string representation
     * @param val Integer value to convert
     * @return C string representation (static buffer, not thread-safe)
     */
    static const char* intToString(int val) {
        static char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d", val);
        return buffer;
    }
};
