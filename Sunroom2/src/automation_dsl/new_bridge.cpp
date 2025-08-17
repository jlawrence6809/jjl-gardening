#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>
#include <time.h>
#include "bridge_functions.h"
#include "definitions.h"
#include "interval_timer.h"
#include "new_core.h"
#include "registry_functions.h"

/**
 * @file new_bridge.cpp
 * @brief Bridge layer using the new function registry system
 *
 * This file contains the logic for processing relay rules using the new
 * function registry architecture. It replaces the old bridge.cpp approach
 * with a cleaner, more extensible system.
 *
 * Key improvements:
 * - Unified function system: all operations are function calls
 * - Pluggable function registration
 * - Clean separation between core logic and platform-specific functions
 */

/**
 * Turn on or off a relay
 *
 * value = 2 means dont care
 * value = 1 means auto on
 * value = 0 means auto off
 */
void setRelay(int index, float value) {
    RelayValue currentValue = RELAY_VALUES[index];

    int intVal = static_cast<int>(value);

    // lowest (ones) tenths digit is the "force" value that we don't want to change, just update the
    // auto value (tens digit)
    int onesDigit = currentValue % 10;
    int newValue = (intVal * 10) + onesDigit;

    RELAY_VALUES[index] = static_cast<RelayValue>(newValue);
}

/**
 * Actuator control callback for the new registry system
 */
bool newTryGetActuator(const std::string &name, std::function<void(float)> &setter) {
    // Define prefix for relay actuators
    String relayPrefix = "relay_";
    // Check if actuator name starts with "relay_" prefix
    if (name.rfind(relayPrefix.c_str(), 0) == 0) {
        // Extract relay index number from the substring after "relay_"
        int index = atoi(name.c_str() + relayPrefix.length());
        // Create bound function that calls setRelay with the extracted index
        setter = std::bind(setRelay, index, std::placeholders::_1);
        // Return true to indicate actuator was found and setter was assigned
        return true;
    }
    // Return false if name doesn't match "relay_" pattern (actuator not found)
    return false;
}

/**
 * Function registration callback for the new registry system
 */
void newRegisterFunctions(FunctionRegistry &registry) {
    // Register core DSL functions (GT, LT, EQ, AND, OR, NOT, IF, SET, NOP)
    registerCoreFunctions(registry);

    // Register platform-specific sensor functions
    registerBridgeFunctions(registry);
}

/**
 * Process the automation DSL using the new function registry system
 */
void processNewAutomationDsl() {
    // Set up the new environment with function registry
    NewRuleCoreEnv env{};
    env.registerFunctions = newRegisterFunctions;
    env.tryGetActuator = newTryGetActuator;

    // Convert Arduino String array to std::string array for platform-neutral core
    std::string stdRules[RUNTIME_RELAY_COUNT];
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        stdRules[i] = std::string(RELAY_RULES[i].c_str());
    }

    // Use the new rule processing function with unified function system
    processNewRuleSet(stdRules, RUNTIME_RELAY_COUNT, env);
}
