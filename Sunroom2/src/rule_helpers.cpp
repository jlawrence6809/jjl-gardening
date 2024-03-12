#include "rule_helpers.h"
#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include <ArduinoJson.h>
#include <map>

/**
 * Turn on or off a relay
 */
void turnOnRelay(int index, bool value)
{
    Serial.println("Setting relay_" + String(index) + " to " + String(value));
    if (value)
    {
        RELAY_VALUES[index] = 1;
        digitalWrite(RELAY_PINS[index], HIGH);
    }
    else
    {
        RELAY_VALUES[index] = 0;
        digitalWrite(RELAY_PINS[index], LOW);
    }
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
 * Actuator to getter function mapping
 *
 * For now only bool actuators are supported
 */
std::map<String, void (*)(int, bool)> BOOL_ACTUATOR_MAP = {
    {"relay_0", turnOnRelay},
    {"relay_1", turnOnRelay},
    {"relay_2", turnOnRelay},
    {"relay_3", turnOnRelay},
    {"relay_4", turnOnRelay},
    {"relay_5", turnOnRelay},
    {"relay_6", turnOnRelay},
    {"relay_7", turnOnRelay},
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
RuleReturn createRuleReturn(TypeCode type, ErrorCode errorCode, bool boolV, int intV, float floatV, int timeV, void (*actuatorSetter)(int, bool), int actuatorSetterIndex)
{
    return {type, errorCode, boolV, intV, floatV, timeV, actuatorSetter, actuatorSetterIndex};
}

RuleReturn createErrorRuleReturn(ErrorCode errorCode)
{
    return createRuleReturn(ERROR_TYPE, errorCode, false, 0, 0.0, 0, 0, 0);
}

RuleReturn createBoolRuleReturn(bool boolV)
{
    return createRuleReturn(BOOL_TYPE, NO_ERROR, boolV, 0, 0.0, 0, 0, 0);
}

RuleReturn createIntRuleReturn(int intV)
{
    return createRuleReturn(INT_TYPE, NO_ERROR, false, intV, 0.0, 0, 0, 0);
}

RuleReturn createFloatRuleReturn(float floatV)
{
    return createRuleReturn(FLOAT_TYPE, NO_ERROR, false, 0, floatV, 0, 0, 0);
}

RuleReturn createVoidRuleReturn()
{
    return createRuleReturn(VOID_TYPE, NO_ERROR, false, 0, 0.0, 0, 0, 0);
}

RuleReturn createTimeRuleReturn(int timeV)
{
    return createRuleReturn(TIME_TYPE, NO_ERROR, false, 0, 0.0, timeV, 0, 0);
}

RuleReturn createBoolActuatorRuleReturn(void (*actuatorSetter)(int, bool), int actuatorSetterIndex)
{
    return createRuleReturn(BOOL_ACTUATOR_TYPE, NO_ERROR, false, 0, 0.0, 0, actuatorSetter, actuatorSetterIndex);
}

/**
 * recursive function to process the rules
 */
RuleReturn processRelayRule(JsonVariantConst doc)
{
    RuleReturn voidReturn = createRuleReturn(VOID_TYPE, NO_ERROR, false, 0, 0.0, 0, 0, 0);

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
            if (BOOL_ACTUATOR_MAP.find(str) != BOOL_ACTUATOR_MAP.end())
            {
                int index = str.substring(6, 7).toInt();
                return createBoolActuatorRuleReturn(BOOL_ACTUATOR_MAP[str], index);
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

        actuatorResult.actuatorSetter(actuatorResult.actuatorSetterIndex, valResult.boolV);
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
    Serial.println("\tactuatorSetterIndex: " + String(result.actuatorSetterIndex));
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
            result.actuatorSetter(result.actuatorSetterIndex, result.boolV);
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