#pragma once

#include "new_core.h"

/**
 * @file bridge_functions.h
 * @brief Bridge-specific sensor function implementations for Arduino/ESP32
 *
 * This file contains sensor functions that interface with the physical hardware
 * on the Arduino/ESP32 platform. These functions are registered by the bridge
 * layer and provide access to temperature, humidity, light sensors, switches, etc.
 */

/**
 * @brief Register all bridge/sensor functions into the provided registry
 * @param registry The function registry to populate with sensor functions
 *
 * This function registers sensor functions that read from physical hardware:
 * - getTemperature: Read temperature sensor
 * - getHumidity: Read humidity sensor
 * - getPhotoSensor: Read light level sensor
 * - getLightSwitch: Read physical switch state
 * - getCurrentTime: Get current time in seconds since midnight
 */
void registerBridgeFunctions(FunctionRegistry& registry);

// Individual sensor function implementations
namespace BridgeFunctions {

// Sensor reading functions
UnifiedValue functionGetTemperature(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionGetHumidity(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionGetPhotoSensor(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionGetLightSwitch(JsonArrayConst args, const NewRuleCoreEnv& env);
UnifiedValue functionGetCurrentTime(JsonArrayConst args, const NewRuleCoreEnv& env);

// Helper function to validate zero-argument sensor calls
UnifiedValue validateZeroArgSensor(JsonArrayConst args, std::function<float()> sensor);
}  // namespace BridgeFunctions
