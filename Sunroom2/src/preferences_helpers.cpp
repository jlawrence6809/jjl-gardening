#include <Preferences.h>
#include "definitions.h"

Preferences preferences;

/**
 * Writes a preference to the NVS
 */
void writePreference(const char *key, char *value)
{
    preferences.begin("app", false);   // Start the NVS "my-app" namespace
    preferences.putString(key, value); // Store a string
    preferences.end();                 // End the NVS session
    Serial.println("wrote preference: " + String(key) + " = " + String(value));
}

/**
 * Reads a preference from the NVS
 */
String readPreference(const char *key, const char *defaultValue)
{
    preferences.begin("app", false);                         // Start the NVS "my-app" namespace
    String value = preferences.getString(key, defaultValue); // Get the string, return "default" if it doesn't exist
    preferences.end();                                       // End the NVS session
    Serial.println("read preference: " + String(key) + " = " + String(value));
    return value;
}

/**
 * Writes wifi credentials to the NVS
 */
void writeWifiCredentials(String ssid, String password)
{
    writePreference("ssid", (char *)ssid.c_str());
    writePreference("pass", (char *)password.c_str());
}
/**
 * Writes the desired temperature/humidity to the NVS
 */
void writeEnvironmentalControlValues(float temperature, float temperatureRange, float humidity, float humidityRange, bool useNaturalLightingCycle, int turnLightsOnAtMinute, int turnLightsOffAtMinute)
{
    writePreference("dt", (char *)String(temperature).c_str());
    writePreference("tr", (char *)String(temperatureRange).c_str());
    writePreference("dh", (char *)String(humidity).c_str());
    writePreference("hr", (char *)String(humidityRange).c_str());
    writePreference("unlc", (char *)String(useNaturalLightingCycle ? 1 : 0).c_str());
    writePreference("tloonam", (char *)String(turnLightsOnAtMinute).c_str());
    writePreference("tloffam", (char *)String(turnLightsOffAtMinute).c_str());
}

void writeRelayValues()
{
    writePreference("relay1", (char *)String(RELAY_VALUES[RELAY_1_PIN] ? 1 : 0).c_str());
    writePreference("relay2", (char *)String(RELAY_VALUES[RELAY_2_PIN] ? 1 : 0).c_str());
    writePreference("relay3", (char *)String(RELAY_VALUES[RELAY_3_PIN] ? 1 : 0).c_str());
    writePreference("relay4", (char *)String(RELAY_VALUES[RELAY_4_PIN] ? 1 : 0).c_str());
    writePreference("relay5", (char *)String(RELAY_VALUES[RELAY_5_PIN] ? 1 : 0).c_str());
    writePreference("relay6", (char *)String(RELAY_VALUES[RELAY_6_PIN] ? 1 : 0).c_str());
    writePreference("relay7", (char *)String(RELAY_VALUES[RELAY_7_PIN] ? 1 : 0).c_str());
    writePreference("relay8", (char *)String(RELAY_VALUES[RELAY_8_PIN] ? 1 : 0).c_str());
}

void setupRelay()
{
    RELAY_VALUES = {
        {RELAY_1_PIN, false},
        {RELAY_2_PIN, false},
        {RELAY_3_PIN, false},
        {RELAY_4_PIN, false},
        {RELAY_5_PIN, false},
        {RELAY_6_PIN, false},
        {RELAY_7_PIN, false},
        {RELAY_8_PIN, false}};

    RELAY_VALUES[RELAY_1_PIN] = readPreference("relay1", "0").toInt() == 1;
    RELAY_VALUES[RELAY_2_PIN] = readPreference("relay2", "0").toInt() == 1;
    RELAY_VALUES[RELAY_3_PIN] = readPreference("relay3", "0").toInt() == 1;
    RELAY_VALUES[RELAY_4_PIN] = readPreference("relay4", "0").toInt() == 1;
    RELAY_VALUES[RELAY_5_PIN] = readPreference("relay5", "0").toInt() == 1;
    RELAY_VALUES[RELAY_6_PIN] = readPreference("relay6", "0").toInt() == 1;
    RELAY_VALUES[RELAY_7_PIN] = readPreference("relay7", "0").toInt() == 1;
    RELAY_VALUES[RELAY_8_PIN] = readPreference("relay8", "0").toInt() == 1;
}

void setupPreferences()
{
    SSID = readPreference("ssid", "");
    PASSWORD = readPreference("pass", "");
    DESIRED_TEMPERATURE = readPreference("dt", "0").toFloat();
    TEMPERATURE_RANGE = readPreference("tr", "0").toFloat();
    DESIRED_HUMIDITY = readPreference("dh", "0").toFloat();
    HUMIDITY_RANGE = readPreference("hr", "0").toFloat();
    USE_NATURAL_LIGHTING_CYCLE = readPreference("unlc", "0").toInt() == 1;
    TURN_LIGHTS_ON_AT_MINUTE = readPreference("tloonam", "0").toInt();
    TURN_LIGHTS_OFF_AT_MINUTE = readPreference("tloffam", "0").toInt();

    setupRelay();

    RESET_COUNTER = readPreference("resets", "0").toInt();
    writePreference("resets", (char *)String(RESET_COUNTER + 1).c_str());

    // if desired temperature/humidity is not valid (bad float/0), set it to default values of 23c and 60%
    if (DESIRED_TEMPERATURE <= 0 || DESIRED_HUMIDITY <= 0)
    {
        DESIRED_TEMPERATURE = 23;
        TEMPERATURE_RANGE = 5;
        DESIRED_HUMIDITY = 60;
        HUMIDITY_RANGE = 5;
        USE_NATURAL_LIGHTING_CYCLE = false;
        TURN_LIGHTS_ON_AT_MINUTE = 0;
        TURN_LIGHTS_OFF_AT_MINUTE = 12 * 60;
        writeEnvironmentalControlValues(
            DESIRED_TEMPERATURE,
            TEMPERATURE_RANGE,
            DESIRED_HUMIDITY,
            HUMIDITY_RANGE,
            USE_NATURAL_LIGHTING_CYCLE,
            TURN_LIGHTS_ON_AT_MINUTE,
            TURN_LIGHTS_OFF_AT_MINUTE);
    }
}