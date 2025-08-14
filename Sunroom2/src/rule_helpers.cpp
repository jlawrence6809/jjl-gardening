#include "rule_helpers.h"
#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include <ArduinoJson.h>
#include <map>
#include <functional>

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

/**
 * Actuator to setter function mapping
 */

// std::map<String, std::function<void(float)>> ACTUATOR_SETTER_MAP = {
//     {"relay_0",
//      std::bind(setRelay, 0, std::placeholders::_1)},
//     {"relay_1",
//      std::bind(setRelay, 1, std::placeholders::_1)},
//     {"relay_2",
//      std::bind(setRelay, 2, std::placeholders::_1)},
//     {"relay_3",
//      std::bind(setRelay, 3, std::placeholders::_1)},
//     {"relay_4",
//      std::bind(setRelay, 4, std::placeholders::_1)},
//     {"relay_5",
//      std::bind(setRelay, 5, std::placeholders::_1)},
//     {"relay_6",
//      std::bind(setRelay, 6, std::placeholders::_1)},
//     {"relay_7",
//      std::bind(setRelay, 7, std::placeholders::_1)},
// };

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
 * Sensor to getter function mapping
 * For now only float sensors are supported
 */
std::map<String, float (*)()> FLOAT_SENSOR_MAP = {
    {"temperature", getTemperature},
    {"humidity", getHumidity},
    {"photoSensor", getPhotoSensor},
    {"lightSwitch", getLightSwitch},
};

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

/**
 * Create a rule return value
 */
RuleReturn createRuleReturn(TypeCode type, ErrorCode errorCode, float val, std::function<void(int)> actuatorSetter)
{
    return {type, errorCode, val, actuatorSetter};
}

RuleReturn createErrorRuleReturn(ErrorCode errorCode)
{
    return createRuleReturn(ERROR_TYPE, errorCode, 0.0, 0);
}

RuleReturn createBoolRuleReturn(bool boolV)
{
    return createRuleReturn(FLOAT_TYPE, NO_ERROR, boolV ? 1 : 0, 0);
}

RuleReturn createFloatRuleReturn(float floatV)
{
    return createRuleReturn(FLOAT_TYPE, NO_ERROR, floatV, 0);
}

RuleReturn createIntRuleReturn(int intV)
{
    float floatV = intV;
    return createRuleReturn(FLOAT_TYPE, NO_ERROR, floatV, 0);
}

RuleReturn createVoidRuleReturn()
{
    return createRuleReturn(VOID_TYPE, NO_ERROR, 0.0, 0);
}

RuleReturn createTimeRuleReturn(int timeV)
{
    if (timeV < 0)
    {
        return createErrorRuleReturn(TIME_ERROR);
    }
    return createIntRuleReturn(timeV);
}

RuleReturn createBoolActuatorRuleReturn(std::function<void(int)> actuatorSetter)
{
    return createRuleReturn(BOOL_ACTUATOR_TYPE, NO_ERROR, 0.0, actuatorSetter);
}

/**
 * recursive function to process the rules
 */
RuleReturn processRelayRule(JsonVariantConst doc)
{
    RuleReturn voidReturn = createRuleReturn(VOID_TYPE, NO_ERROR, 0.0, 0);

    if (!doc.is<JsonArrayConst>())
    {
        // check if it's a string
        if (doc.is<String>())
        {
            String str = doc.as<String>();
            if (str.startsWith("@"))
            {
                return createTimeRuleReturn(mintuesFromHHMMSS(str));
            }

            // check if in actuator map
            std::function<void(float)> actuatorSetter = getActuatorSetter(str);
            if (actuatorSetter != 0)
            {
                return createBoolActuatorRuleReturn(actuatorSetter);
            }

            // check if in sensor map
            if (FLOAT_SENSOR_MAP.find(str) != FLOAT_SENSOR_MAP.end())
            {
                return createFloatRuleReturn(FLOAT_SENSOR_MAP[str]());
            }

            if (str == "currentTime")
            {
                return createTimeRuleReturn(getCurrentSeconds());
            }

            return createErrorRuleReturn(UNREC_STR_ERROR);
        }
        else if (doc.is<bool>())
        {
            return createBoolRuleReturn(doc.as<bool>());
        }
        else if (doc.is<int>())
        {
            return createIntRuleReturn(doc.as<int>());
        }
        else if (doc.is<float>())
        {
            return createFloatRuleReturn(doc.as<float>());
        }
        else
        {
            return createErrorRuleReturn(UNREC_TYPE_ERROR);
        }
    }

    // get first element from array as string:
    JsonArrayConst array = doc.as<JsonArrayConst>();

    String type = array[0];
    if (type == "NOP")
    {
        return voidReturn;
    }
    else if (type == "IF")
    {
        RuleReturn conditionResult = processRelayRule(array[1]);
        if (conditionResult.type == ERROR_TYPE)
        {
            return conditionResult;
        }
        if (conditionResult.type != FLOAT_TYPE)
        {
            return createErrorRuleReturn(IF_CONDITION_ERROR);
        }
        if (conditionResult.val > 0)
        {
            return processRelayRule(array[2]);
        }
        else
        {
            return processRelayRule(array[3]);
        }
    }
    else if (type == "SET")
    {
        RuleReturn actuatorResult = processRelayRule(array[1]);
        RuleReturn valResult = processRelayRule(array[2]);

        if (actuatorResult.type == ERROR_TYPE)
        {
            return actuatorResult;
        }
        if (valResult.type == ERROR_TYPE)
        {
            return valResult;
        }

        // Currently only support bool actuators
        if (actuatorResult.type != BOOL_ACTUATOR_TYPE || valResult.type != FLOAT_TYPE)
        {
            return createErrorRuleReturn(BOOL_ACTUATOR_ERROR);
        }

        actuatorResult.actuatorSetter(valResult.val);
        return voidReturn;
    }
    else if (type == "AND" || type == "OR")
    {
        RuleReturn aResult = processRelayRule(array[1]);
        RuleReturn bResult = processRelayRule(array[2]);
        if (aResult.type == ERROR_TYPE)
        {
            return aResult;
        }
        if (bResult.type == ERROR_TYPE)
        {
            return bResult;
        }
        if (aResult.type != FLOAT_TYPE || bResult.type != FLOAT_TYPE)
        {
            return createErrorRuleReturn(AND_OR_ERROR);
        }
        float aVal = aResult.val;
        float bVal = bResult.val;

        float aBool = aVal > 0 ? 1 : 0;
        float bBool = bVal > 0 ? 1 : 0;

        bool result = type == "AND" ? (aBool && bBool) : (aBool || bBool);
        Serial.println("AND/OR");
        Serial.println(aBool);
        Serial.println(bBool);
        Serial.println(result);

        return createBoolRuleReturn(result);
    }
    else if (type == "NOT")
    {
        RuleReturn aResult = processRelayRule(array[1]);
        if (aResult.type == ERROR_TYPE)
        {
            return aResult;
        }
        if (aResult.type != FLOAT_TYPE)
        {
            return createErrorRuleReturn(NOT_ERROR);
        }
        return createBoolRuleReturn(aResult.val <= 0 ? true : false);
    }
    else if (
        type == "EQ" ||
        type == "NE" ||
        type == "GT" ||
        type == "LT" ||
        type == "GTE" ||
        type == "LTE")
    {
        RuleReturn aResult = processRelayRule(array[1]);
        RuleReturn bResult = processRelayRule(array[2]);
        if (aResult.type == ERROR_TYPE)
        {
            return aResult;
        }
        if (bResult.type == ERROR_TYPE)
        {
            return bResult;
        }
        if (aResult.type != FLOAT_TYPE || bResult.type != FLOAT_TYPE)
        {
            return createErrorRuleReturn(COMPARISON_TYPE_EQUALITY_ERROR);
        }

        bool result = false;

        if (type == "EQ")
        {
            result = aResult.val == bResult.val;
        }
        else if (type == "NE")
        {
            result = aResult.val != bResult.val;
        }
        else if (type == "GT")
        {
            result = aResult.val > bResult.val;
        }
        else if (type == "LT")
        {
            result = aResult.val < bResult.val;
        }
        else if (type == "GTE")
        {
            result = aResult.val >= bResult.val;
        }
        else if (type == "LTE")
        {
            result = aResult.val <= bResult.val;
        }

        Serial.println("Comparison");
        Serial.println(type);
        Serial.println(aResult.val);
        Serial.println(bResult.val);
        Serial.println(result);

        return createBoolRuleReturn(result);
    }

    return createErrorRuleReturn(UNREC_FUNC_ERROR);
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

    for (int i = 0; i < RELAY_PINS.size(); i++)
    {
        Serial.println("Processing relay rule:");
        Serial.println(RELAY_RULES[i]);

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

        RuleReturn result = processRelayRule(doc);

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
        else
        {
            Serial.println("Rule processed successfully");
        }
    }
}

// void testRule(String rule)
// {
//     Serial.println("Testing Rule: ");
//     try
//     {
//         Serial.println(rule);
//         DynamicJsonDocument doc(1024);
//         DeserializationError error = deserializeJson(doc, rule);
//         RuleReturn result = processRelayRule(doc);
//         printRuleReturn(result);
//     }
//     catch (...)
//     {
//         Serial.println("Error in testRule");
//     }
//     Serial.println("Finished testing Rule");
// }

// void runTests()
// {
//     testRule("[\"EQ\", true, true]");
//     testRule("[\"IF\", [\"EQ\", true, true], [\"SET\", \"relay_0\", true], [\"SET\", \"relay_0\", false]]");
//     testRule("[\"IF\", [\"EQ\", true, false], [\"SET\", \"relay_0\", true], [\"SET\", \"relay_0\", false]]");
//     testRule("[\"IF\", [\"EQ\", \"@12:00:00\", \"@12:00:00\"], [\"SET\", \"relay_0\", true], [\"SET\", \"relay_0\", false]]");
//     testRule("[\"IF\", [\"GT\", \"currentTime\", \"@12:00:00\"], [\"SET\", \"relay_0\", true], [\"SET\", \"relay_0\", false]]");
// }