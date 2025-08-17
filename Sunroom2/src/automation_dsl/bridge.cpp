#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>
#include <time.h>
#include "bridge_functions.h"
#include "core.h"
#include "definitions.h"
#include "interval_timer.h"
#include "registry_functions.h"
#include "unified_value.h"

/**
 * This file contains the logic for processing relay rules using the new function registry system.
 *
 * The new system uses unified function syntax where everything is a function call:
 *
 * Temperature rules:
 * ["IF", ["GT", ["getTemperature"], 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 * ["AND", ["GT", ["getTemperature"], 25], ["LT", ["getTemperature"], 30]]
 *
 * Time rules:
 * ["GT", ["getCurrentTime"], "@12:00:00"]
 *
 * Light rules:
 * ["GT", ["getPhotoSensor"], 1000]
 *
 * Switch rules:
 * ["EQ", ["getLightSwitch"], 1]
 *
 * Simple rules automatically control relays:
 * ["GT", ["getTemperature"], 25]  // Automatically sets relay to 1 if temp > 25°C
 *
 * Complex rules use explicit control:
 * ["IF", condition, ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 */

/**
 * Transition function idea: "becameTime"
 *
 * ["IF", ["becameTime", "@12:00:00"], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
 * Implementation:
 * - store the last time the rules were processed
 * - compare the last time to the current time
 * - if the last time is less than the target time and the current time is greater than the target
 * time, return true
 *
 * Could also implement it by allowing for a "lastCheckedTime" variable to be referenced:
 *
 * ["IF", ["AND", ["GT", "currentTime", "@12:00:00"], ["LT", "lastCheckedTime", "@12:00:00"]],
 * ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
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

std::function<void(float)> getActuatorSetter(String name) {
    String relayPrefix = "relay_";
    if (name.startsWith(relayPrefix)) {
        int index = name.substring(relayPrefix.length()).toInt();
        return std::bind(setRelay, index, std::placeholders::_1);
    }

    return 0;
}

void printUnifiedValue(UnifiedValue result) {
    Serial.println("UnifiedValue:");
    Serial.println("\ttype: " + String(static_cast<int>(result.type)));
    if (result.type == UnifiedValue::ERROR_TYPE) {
        Serial.println("\terrorCode: " + String(static_cast<int>(result.errorCode)));
        Serial.println("\terrorString: " + String(result.asString()));
    } else {
        switch (result.type) {
            case UnifiedValue::FLOAT_TYPE:
                Serial.println("\tfloatValue: " + String(result.asFloat()));
                break;
            case UnifiedValue::INT_TYPE:
                Serial.println("\tintValue: " + String(result.asInt()));
                break;
            case UnifiedValue::STRING_TYPE:
                Serial.println("\tstringValue: " + String(result.asString()));
                break;
            case UnifiedValue::VOID_TYPE:
                Serial.println("\tvoidType (success)");
                break;
            case UnifiedValue::ACTUATOR_TYPE:
                Serial.println("\tactuatorType");
                break;
            default:
                Serial.println("\tunknown type");
                break;
        }
    }
}

/**
 * Function registration callback for the new registry system
 */
void registerFunctions(FunctionRegistry &registry) {
    // Register core DSL functions (GT, LT, EQ, AND, OR, NOT, IF, SET, NOP)
    registerCoreFunctions(registry);

    // Register platform-specific sensor functions
    registerBridgeFunctions(registry);
}

/**
 * Process the automation DSL using the function registry system
 */
void processAutomationDsl() {
    // Set up the new environment with function registry
    RuleCoreEnv env{};
    env.registerFunctions = registerFunctions;
    env.tryGetActuator = [](const std::string &name, std::function<void(float)> &setter) {
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
    };

    // Convert Arduino String array to std::string array for platform-neutral core
    std::string stdRules[RUNTIME_RELAY_COUNT];
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        stdRules[i] = std::string(RELAY_RULES[i].c_str());
    }

    // Use the new rule processing function with unified function system
    processRuleSet(stdRules, RUNTIME_RELAY_COUNT, env);
}
