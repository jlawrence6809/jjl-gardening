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
    preferences.begin("app", false); // Start the NVS namespace
    String value;
    if (preferences.isKey(key))
    {
        value = preferences.getString(key, defaultValue);
    }
    else
    {
        value = String(defaultValue);
    }
    preferences.end(); // End the NVS session
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
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        writePreference(("rly" + String(i)).c_str(), (char *)String(RELAY_VALUES[i]).c_str());
    }
}

void writeRelayRules()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        writePreference(("rlyrl" + String(i)).c_str(), (char *)(RELAY_RULES[i]).c_str());
    }
}

void writeRelayLabels()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        writePreference(("rlylbl" + String(i)).c_str(), (char *)(RELAY_LABELS[i]).c_str());
    }
}

void setupRelay()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        RELAY_VALUES[i] = static_cast<RelayValue>(readPreference(("rly" + String(i)).c_str(), "0").toInt());
        /**
         * Rules are json blobs that define the conditions for a relay to be turned on or off
         */
        RELAY_RULES[i] = readPreference(("rlyrl" + String(i)).c_str(), "[\"NOP\"]");

        /**
         * Labels are the names of the relays
         */
        RELAY_LABELS[i] = readPreference(("rlylbl" + String(i)).c_str(), (char *)("Relay " + String(i)).c_str());
    }
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

    // Get the last reset reason
    LAST_RESET_REASON = (int)esp_reset_reason();

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