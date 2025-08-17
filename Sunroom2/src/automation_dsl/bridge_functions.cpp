/**
 * @file bridge_functions.cpp
 * @brief Bridge-specific sensor function implementations for Arduino/ESP32
 *
 * This file implements sensor functions that interface with physical hardware.
 * These functions read from the global variables that are updated by the main
 * sensor reading loop in the Arduino application.
 */

#include "bridge_functions.h"
#include "new_core.h"

// Include necessary Arduino/ESP32 headers when compiling for embedded target
#ifdef ARDUINO
    #include <Arduino.h>
    #include <time.h>
#endif

// External sensor value declarations (defined in main Arduino application)
// These are updated by the main sensor reading loop
extern float CURRENT_TEMPERATURE;
extern float CURRENT_HUMIDITY;
extern float LIGHT_LEVEL;
extern int IS_SWITCH_ON;

// Forward declarations of existing bridge functions
float getTemperature();
float getHumidity();
float getPhotoSensor();
float getLightSwitch();
int getCurrentSeconds();

/**
 * @brief Register all bridge/sensor functions into the provided registry
 */
void registerBridgeFunctions(FunctionRegistry& registry) {
    registry["getTemperature"] = BridgeFunctions::functionGetTemperature;
    registry["getHumidity"] = BridgeFunctions::functionGetHumidity;
    registry["getPhotoSensor"] = BridgeFunctions::functionGetPhotoSensor;
    registry["getLightSwitch"] = BridgeFunctions::functionGetLightSwitch;
    registry["getCurrentTime"] = BridgeFunctions::functionGetCurrentTime;
}

namespace BridgeFunctions {

/**
 * @brief Helper function to validate zero-argument sensor calls
 */
UnifiedValue validateZeroArgSensor(JsonArrayConst args, std::function<float()> sensor) {
    // Sensor functions should have no arguments (just the function name)
    if (args.size() != 1) {
        return UnifiedValue::createError(UNREC_FUNC_ERROR);
    }

    float value = sensor();
    return UnifiedValue(value);
}

/**
 * @brief Get current temperature reading
 * Syntax: ["getTemperature"]
 */
UnifiedValue functionGetTemperature(JsonArrayConst args, const NewRuleCoreEnv& env) {
    return validateZeroArgSensor(args, []() { return getTemperature(); });
}

/**
 * @brief Get current humidity reading
 * Syntax: ["getHumidity"]
 */
UnifiedValue functionGetHumidity(JsonArrayConst args, const NewRuleCoreEnv& env) {
    return validateZeroArgSensor(args, []() { return getHumidity(); });
}

/**
 * @brief Get current photo sensor (light level) reading
 * Syntax: ["getPhotoSensor"]
 */
UnifiedValue functionGetPhotoSensor(JsonArrayConst args, const NewRuleCoreEnv& env) {
    return validateZeroArgSensor(args, []() { return getPhotoSensor(); });
}

/**
 * @brief Get current light switch state
 * Syntax: ["getLightSwitch"]
 */
UnifiedValue functionGetLightSwitch(JsonArrayConst args, const NewRuleCoreEnv& env) {
    return validateZeroArgSensor(args, []() { return getLightSwitch(); });
}

/**
 * @brief Get current time in seconds since midnight
 * Syntax: ["getCurrentTime"]
 */
UnifiedValue functionGetCurrentTime(JsonArrayConst args, const NewRuleCoreEnv& env) {
    return validateZeroArgSensor(args, []() { return static_cast<float>(getCurrentSeconds()); });
}

}  // namespace BridgeFunctions

// Define the existing bridge functions here for compatibility
// These will be called by the new function implementations above

float getTemperature() { return CURRENT_TEMPERATURE; }

float getHumidity() { return CURRENT_HUMIDITY; }

float getPhotoSensor() { return LIGHT_LEVEL; }

float getLightSwitch() { return static_cast<float>(IS_SWITCH_ON); }

int getCurrentSeconds() {
#ifdef ARDUINO
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return -1;
    }
    int hours = timeinfo.tm_hour;
    int minutes = timeinfo.tm_min;
    int seconds = timeinfo.tm_sec;
    return (hours * 60 * 60) + (minutes * 60) + seconds;
#else
    // For testing/native compilation, return a mock time value
    return 43200;  // 12:00:00 noon
#endif
}
