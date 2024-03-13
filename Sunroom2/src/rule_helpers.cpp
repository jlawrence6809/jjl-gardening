#include "rule_helpers.h"
#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include <ArduinoJson.h>
#include <map>
#include <functional>

/**
 * Turn on or off a relay
 *
 * value = 2 means dont care
 * value = 1 means auto on
 * value = 0 means auto off
 */
void setRelay(int index, int value)
{
    RelayValue currentValue = RELAY_VALUES[index];

    // lowest (ones) tenths digit is the "force" value that we don't want to change, just update the auto value (tens digit)
    int onesDigit = currentValue % 10;
    int newValue = (value * 10) + onesDigit;

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

std::map<String, std::function<void(int)>> ACTUATOR_SETTER_MAP = {
    {"relay_0",
     std::bind(setRelay, 0, std::placeholders::_1)},
    {"relay_1",
     std::bind(setRelay, 1, std::placeholders::_1)},
    {"relay_2",
     std::bind(setRelay, 2, std::placeholders::_1)},
    {"relay_3",
     std::bind(setRelay, 3, std::placeholders::_1)},
    {"relay_4",
     std::bind(setRelay, 4, std::placeholders::_1)},
    {"relay_5",
     std::bind(setRelay, 5, std::placeholders::_1)},
    {"relay_6",
     std::bind(setRelay, 6, std::placeholders::_1)},
    {"relay_7",
     std::bind(setRelay, 7, std::placeholders::_1)},
};

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
 * Convert @HH:MM to minutes
 */
int mintuesFromHHMM(String hhmm)
{
    return hhmm.substring(1, 2).toInt() * 60 + hhmm.substring(4, 5).toInt();
}

/**
 * Get the current time in minutes
 */
int getCurrentMinutes()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return -1;
    }
    return (timeinfo.tm_hour * 60) + timeinfo.tm_min;
}

/**
 * Create a rule return value
 */
RuleReturn createRuleReturn(TypeCode type, ErrorCode errorCode, bool boolV, int intV, float floatV, int timeV, std::function<void(int)> actuatorSetter)
{
    return {type, errorCode, boolV, intV, floatV, timeV, actuatorSetter};
}

RuleReturn createErrorRuleReturn(ErrorCode errorCode)
{
    return createRuleReturn(ERROR_TYPE, errorCode, false, 0, 0.0, 0, 0);
}

RuleReturn createBoolRuleReturn(bool boolV)
{
    return createRuleReturn(BOOL_TYPE, NO_ERROR, boolV, 0, 0.0, 0, 0);
}

RuleReturn createIntRuleReturn(int intV)
{
    return createRuleReturn(INT_TYPE, NO_ERROR, false, intV, 0.0, 0, 0);
}

RuleReturn createFloatRuleReturn(float floatV)
{
    return createRuleReturn(FLOAT_TYPE, NO_ERROR, false, 0, floatV, 0, 0);
}

RuleReturn createVoidRuleReturn()
{
    return createRuleReturn(VOID_TYPE, NO_ERROR, false, 0, 0.0, 0, 0);
}

RuleReturn createTimeRuleReturn(int timeV)
{
    return createRuleReturn(TIME_TYPE, NO_ERROR, false, 0, 0.0, timeV, 0);
}

RuleReturn createBoolActuatorRuleReturn(std::function<void(int)> actuatorSetter)
{
    return createRuleReturn(BOOL_ACTUATOR_TYPE, NO_ERROR, false, 0, 0.0, 0, actuatorSetter);
}

/**
 * recursive function to process the rules
 */
RuleReturn processRelayRule(JsonVariantConst doc)
{
    RuleReturn voidReturn = createRuleReturn(VOID_TYPE, NO_ERROR, false, 0, 0.0, 0, 0);

    if (!doc.is<JsonArrayConst>())
    {
        // check if it's a string
        if (doc.is<String>())
        {
            String str = doc.as<String>();
            if (str.startsWith("@"))
            {
                return createTimeRuleReturn(mintuesFromHHMM(str));
            }
            // check if in actuator map
            if (ACTUATOR_SETTER_MAP.find(str) != ACTUATOR_SETTER_MAP.end())
            {
                return createBoolActuatorRuleReturn(ACTUATOR_SETTER_MAP[str]);
            }

            // check if in sensor map
            if (FLOAT_SENSOR_MAP.find(str) != FLOAT_SENSOR_MAP.end())
            {
                return createFloatRuleReturn(FLOAT_SENSOR_MAP[str]());
            }

            if (str == "currentTime")
            {
                return createTimeRuleReturn(getCurrentMinutes());
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
        if (conditionResult.type != BOOL_TYPE)
        {
            return createErrorRuleReturn(IF_CONDITION_ERROR);
        }
        if (conditionResult.boolV)
        {
            processRelayRule(array[2]);
        }
        else
        {
            processRelayRule(array[3]);
        }
        return voidReturn;
    }
    else if (type == "SET")
    {
        RuleReturn actuatorResult = processRelayRule(array[1]);
        RuleReturn valResult = processRelayRule(array[2]);

        // Currently only support bool actuators
        if (actuatorResult.type != BOOL_ACTUATOR_TYPE || valResult.type != BOOL_TYPE)
        {
            return createErrorRuleReturn(BOOL_ACTUATOR_ERROR);
        }

        actuatorResult.actuatorSetter(valResult.boolV ? 1 : 0);
        return voidReturn;
    }
    else if (type == "AND" || type == "OR")
    {
        RuleReturn aResult = processRelayRule(array[1]);
        RuleReturn bResult = processRelayRule(array[2]);
        if (aResult.type != BOOL_TYPE || bResult.type != BOOL_TYPE)
        {
            return createErrorRuleReturn(AND_OR_ERROR);
        }
        bool result = type == "AND" ? aResult.boolV && bResult.boolV : aResult.boolV || bResult.boolV;

        return createBoolRuleReturn(result);
    }
    else if (type == "NOT")
    {
        RuleReturn aResult = processRelayRule(array[1]);
        if (aResult.type != BOOL_TYPE)
        {
            return createErrorRuleReturn(NOT_ERROR);
        }
        return createBoolRuleReturn(!aResult.boolV);
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
        if (aResult.type != bResult.type)
        {
            return createErrorRuleReturn(COMPARISON_TYPE_EQUALITY_ERROR);
        }

        bool result = false;
        float aCompare = 0.0;
        float bCompare = 0.0;

        if (aResult.type == INT_TYPE)
        {
            aCompare = aResult.intV;
            bCompare = bResult.intV;
        }
        else if (aResult.type == FLOAT_TYPE)
        {
            aCompare = aResult.floatV;
            bCompare = bResult.floatV;
        }
        else if (aResult.type == TIME_TYPE)
        {
            aCompare = aResult.timeV;
            bCompare = bResult.timeV;
        }
        else if (aResult.type == BOOL_TYPE)
        {
            aCompare = aResult.boolV;
            bCompare = bResult.boolV;
        }
        else
        {
            return createErrorRuleReturn(COMPARISON_TYPE_ERROR);
        }

        if (type == "EQ")
        {
            result = aCompare == bCompare;
        }
        else if (type == "NE")
        {
            result = aCompare != bCompare;
        }
        else if (type == "GT")
        {
            result = aCompare > bCompare;
        }
        else if (type == "LT")
        {
            result = aCompare < bCompare;
        }
        else if (type == "GTE")
        {
            result = aCompare >= bCompare;
        }
        else if (type == "LTE")
        {
            result = aCompare <= bCompare;
        }

        return createBoolRuleReturn(result);
    }

    return createErrorRuleReturn(UNREC_FUNC_ERROR);
}

void printRuleReturn(RuleReturn result)
{
    Serial.println("RuleReturn:");
    Serial.println("\ttype: " + String(result.type));
    Serial.println("\terrorCode: " + String(result.errorCode));
    Serial.println("\tboolV: " + String(result.boolV));
    Serial.println("\tintV: " + String(result.intV));
    Serial.println("\tfloatV: " + String(result.floatV));
    Serial.println("\ttimeV: " + String(result.timeV));
}

/**
 * Process the relay rules
 */
void processRelayRules()
{
    for (int i = 0; i < RELAY_COUNT; i++)
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

        if (result.type == BOOL_ACTUATOR_TYPE)
        {
            Serial.println("Setting actuator");
            result.actuatorSetter(result.boolV ? 1 : 0);
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
//     testRule("[\"IF\", [\"EQ\", \"@12:00\", \"@12:00\"], [\"SET\", \"relay_0\", true], [\"SET\", \"relay_0\", false]]");
//     testRule("[\"IF\", [\"GT\", \"currentTime\", \"@12:00\"], [\"SET\", \"relay_0\", true], [\"SET\", \"relay_0\", false]]");
// }