#include "rule_helpers.h"
#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include <ArduinoJson.h>
#include <map>
#include <functional>
#include "rule_core.h"

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
 * Convert @HH:MM:SS to seconds
 */
int mintuesFromHHMMSS(String hhmm)
{
    int hours = hhmm.substring(1, 3).toInt();
    int minutes = hhmm.substring(4, 6).toInt();
    int seconds = hhmm.substring(7, 9).toInt();
    return (hours * 60 * 60) + (minutes * 60) + seconds;
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

    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++)
    {
        // Serial.println("Processing relay rule:");
        // Serial.println(RELAY_RULES[i]);

        // Set the relay auto digit to dont care
        setRelay(i, 2);

        // parse json
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, RELAY_RULES[i]);

        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            continue;
        }

        // Bridge to reusable, platform-neutral core evaluator
        RuleCoreEnv env{};
        env.tryReadSensor = [](const std::string &name, float &out) {
            if (name == "temperature") { out = getTemperature(); return true; }
            if (name == "humidity") { out = getHumidity(); return true; }
            if (name == "photoSensor") { out = getPhotoSensor(); return true; }
            if (name == "lightSwitch") { out = getLightSwitch(); return true; }
            return false;
        };
        env.tryGetActuator = [](const std::string &name, std::function<void(float)> &setter) {
            String relayPrefix = "relay_";
            if (name.rfind(relayPrefix.c_str(), 0) == 0) {
                int index = atoi(name.c_str() + relayPrefix.length());
                setter = std::bind(setRelay, index, std::placeholders::_1);
                return true;
            }
            return false;
        };
        env.getCurrentSeconds = [](){ return getCurrentSeconds(); };
        env.parseTimeLiteral = [](const std::string &hhmm){ return mintuesFromHHMMSS(String(hhmm.c_str())); };

        RuleReturn result = processRuleCore(doc, env);

        if (result.type == FLOAT_TYPE)
        {
            Serial.println("Setting actuator: " + String(i) + " to: " + String(result.val));
            getActuatorSetter("relay_" + String(i))(result.val);
        }
        else if (result.type != VOID_TYPE)
        {
            Serial.println("Unexpected rule result: ");
            printRuleReturn(result);
        }
        // else
        // {
        //     Serial.println("Rule processed successfully");
        // }
    }
}
