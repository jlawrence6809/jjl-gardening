#include "rule_helpers.h"
#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include <ArduinoJson.h>
#include <map>
#include <functional>
#include "rule_core.h"
#include "sensor_value.h"

/**
 * This file contains the logic for processing the relay rules
 * Here are some possible rules:
 * Temperature rules:
 * ["IF", ["EQ", "temperature", 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 * ["IF", ["GT", "temperature", 25], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 * ["IF", ["AND", ["GT", "temperature", 25], ["LT", "temperature", 30]], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
 * Time rules:
 * ["IF", ["GT", "currentTime", "@12:00:00"], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 * ["IF", ["EQ", "currentTime", "@12:00:00"], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
 * Light rules:
 * ["IF", ["GT", "photoSensor", 1000], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
 * Switch rules:
 * ["IF", ["EQ", "lightSwitch", 1], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
 */

/**
 * Transition function idea: "becameTime"
 *
 * ["IF", ["becameTime", "@12:00:00"], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
 * Implementation:
 * - store the last time the rules were processed
 * - compare the last time to the current time
 * - if the last time is less than the target time and the current time is greater than the target time, return true
 *
 * Could also implement it by allowing for a "lastCheckedTime" variable to be referenced:
 *
 * ["IF", ["AND", ["GT", "currentTime", "@12:00:00"], ["LT", "lastCheckedTime", "@12:00:00"]], ["SET", "relay_0", 1], ["SET", "relay_0", 0]]
 *
 */

/**
 * Turn on or off a relay
 *
 * value = 2 means dont care
 * value = 1 means auto on
 * value = 0 means auto off
 */
void setRelay(int index, float value)
{
    RelayValue currentValue = RELAY_VALUES[index];

    int intVal = static_cast<int>(value);

    // lowest (ones) tenths digit is the "force" value that we don't want to change, just update the auto value (tens digit)
    int onesDigit = currentValue % 10;
    int newValue = (intVal * 10) + onesDigit;

    RELAY_VALUES[index] = static_cast<RelayValue>(newValue);
}

float getTemperature()
{
    return CURRENT_TEMPERATURE;
}

float getHumidity()
{
    return CURRENT_HUMIDITY;
}

float getPhotoSensor()
{
    float out = LIGHT_LEVEL;
    return out;
}

float getLightSwitch()
{
    float out = IS_SWITCH_ON;
    return out;
}

std::function<void(float)> getActuatorSetter(String name)
{
    String relayPrefix = "relay_";
    if (name.startsWith(relayPrefix))
    {
        int index = name.substring(relayPrefix.length()).toInt();
        return std::bind(setRelay, index, std::placeholders::_1);
    }

    return 0;
}



/**
 * Get the current time in minutes
 */
int getCurrentSeconds()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return -1;
    }
    int hours = timeinfo.tm_hour;
    int minutes = timeinfo.tm_min;
    int seconds = timeinfo.tm_sec;
    return (hours * 60 * 60) + (minutes * 60) + seconds;
}


void printRuleReturn(RuleReturn result)
{
    Serial.println("RuleReturn:");
    Serial.println("\ttype: " + String(result.type));
    Serial.println("\terrorCode: " + String(result.errorCode));
    Serial.println("\val: " + String(result.val));
}

/**
 * Process the relay rules
 */
void processRelayRules()
{
    // // setup some test values
    // CURRENT_TEMPERATURE = 25.0;
    // CURRENT_HUMIDITY = 50.0;
    // LIGHT_LEVEL = 1000;
    // IS_SWITCH_ON = 1;

    // Bridge to reusable, platform-neutral core evaluator
    RuleCoreEnv env{};
    env.tryReadSensor = [](const std::string &name, SensorValue &out) {
        if (name == "temperature") { out = SensorValue(getTemperature()); return true; }
        if (name == "humidity") { out = SensorValue(getHumidity()); return true; }
        if (name == "photoSensor") { out = SensorValue(getPhotoSensor()); return true; }
        if (name == "lightSwitch") { out = SensorValue(getLightSwitch()); return true; }
        return false;
    };
    // Set up actuator lookup function for rule processing system
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
    env.getCurrentSeconds = [](){ return getCurrentSeconds(); };

    // Convert Arduino String array to std::string array for platform-neutral core
    std::string stdRules[RUNTIME_RELAY_COUNT];
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        stdRules[i] = std::string(RELAY_RULES[i].c_str());
    }

    // Use the generic rule processing function with unified actuator system
    processRuleSet(stdRules, RUNTIME_RELAY_COUNT, env);
}
