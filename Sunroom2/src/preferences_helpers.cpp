#include <Preferences.h>
#include "definitions.h"
#include "esp_system.h"

Preferences preferences;

/**
 * Writes a preference to the NVS
 */
void writePreference(const char *key, char *value) {
    preferences.begin("app", false);    // Start the NVS "my-app" namespace
    preferences.putString(key, value);  // Store a string
    preferences.end();                  // End the NVS session
    Serial.println("wrote preference: " + String(key) + " = " + String(value));
}

/**
 * Reads a preference from the NVS
 */
String readPreference(const char *key, const char *defaultValue) {
    preferences.begin("app", false);  // Start the NVS namespace
    String value;
    if (preferences.isKey(key)) {
        value = preferences.getString(key, defaultValue);
    } else {
        value = String(defaultValue);
    }
    preferences.end();  // End the NVS session
    Serial.println("read preference: " + String(key) + " = " + String(value));
    return value;
}

/**
 * Writes wifi credentials to the NVS
 */
void writeWifiCredentials(String ssid, String password) {
    writePreference("ssid", (char *)ssid.c_str());
    writePreference("pass", (char *)password.c_str());
}
/**
 * Writes the desired temperature/humidity to the NVS
 */
void writeEnvironmentalControlValues(float temperature, float temperatureRange, float humidity,
                                     float humidityRange, bool useNaturalLightingCycle,
                                     int turnLightsOnAtMinute, int turnLightsOffAtMinute) {
    writePreference("dt", (char *)String(temperature).c_str());
    writePreference("tr", (char *)String(temperatureRange).c_str());
    writePreference("dh", (char *)String(humidity).c_str());
    writePreference("hr", (char *)String(humidityRange).c_str());
    writePreference("unlc", (char *)String(useNaturalLightingCycle ? 1 : 0).c_str());
    writePreference("tloonam", (char *)String(turnLightsOnAtMinute).c_str());
    writePreference("tloffam", (char *)String(turnLightsOffAtMinute).c_str());
}

static void loadRelayConfig() {
    // Load runtime relay configuration: count, pins, inversion.
    // Defaults to 0 relays if not set.
    String countStr = readPreference("rc", "-1");
    int count = countStr.toInt();
    if (count < 0) {
        RUNTIME_RELAY_COUNT = 0;
        return;
    }
    if (count > MAX_RELAYS) {
        count = MAX_RELAYS;
    }
    RUNTIME_RELAY_COUNT = count;
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        char pinKey[16];
        char invKey[16];
        snprintf(pinKey, sizeof(pinKey), "rpin%d", i);
        snprintf(invKey, sizeof(invKey), "rinv%d", i);
        RUNTIME_RELAY_PINS[i] = readPreference(pinKey, "-1").toInt();
        RUNTIME_RELAY_IS_INVERTED[i] = readPreference(invKey, "0").toInt() == 1;
    }
}

void writeRelayConfig() {
    writePreference("rc", (char *)String(RUNTIME_RELAY_COUNT).c_str());
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        char pinKey[16];
        char invKey[16];
        snprintf(pinKey, sizeof(pinKey), "rpin%d", i);
        snprintf(invKey, sizeof(invKey), "rinv%d", i);
        writePreference(pinKey, (char *)String(RUNTIME_RELAY_PINS[i]).c_str());
        writePreference(invKey, (char *)String(RUNTIME_RELAY_IS_INVERTED[i] ? 1 : 0).c_str());
    }
}

void writeRelayValues() {
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        char relayKey[16];
        snprintf(relayKey, sizeof(relayKey), "rly%d", i);
        writePreference(relayKey, (char *)String(RELAY_VALUES[i]).c_str());
    }
}

void writeRelayRules() {
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        char rulesKey[16];
        snprintf(rulesKey, sizeof(rulesKey), "rlyrl%d", i);
        writePreference(rulesKey, (char *)(RELAY_RULES[i]).c_str());
    }
}

void writeRelayLabels() {
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        char labelKey[16];
        snprintf(labelKey, sizeof(labelKey), "rlylbl%d", i);
        writePreference(labelKey, (char *)(RELAY_LABELS[i]).c_str());
    }
}

void setupRelay() {
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++) {
        char relayKey[16];
        char rulesKey[16];
        char labelKey[16];
        char defaultLabel[32];

        snprintf(relayKey, sizeof(relayKey), "rly%d", i);
        snprintf(rulesKey, sizeof(rulesKey), "rlyrl%d", i);
        snprintf(labelKey, sizeof(labelKey), "rlylbl%d", i);
        snprintf(defaultLabel, sizeof(defaultLabel), "Relay %d", i);

        RELAY_VALUES[i] = static_cast<RelayValue>(readPreference(relayKey, "0").toInt());
        /**
         * Rules are json blobs that define the conditions for a relay to be turned on or off
         */
        RELAY_RULES[i] = readPreference(rulesKey, "[\"NOP\"]");

        /**
         * Labels are the names of the relays
         */
        RELAY_LABELS[i] = readPreference(labelKey, defaultLabel);
    }
}
void setupPreferences() {
    loadRelayConfig();
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

    // if desired temperature/humidity is not valid (bad float/0), set it to default values of 23c
    // and 60%
    if (DESIRED_TEMPERATURE <= 0 || DESIRED_HUMIDITY <= 0) {
        DESIRED_TEMPERATURE = 23;
        TEMPERATURE_RANGE = 5;
        DESIRED_HUMIDITY = 60;
        HUMIDITY_RANGE = 5;
        USE_NATURAL_LIGHTING_CYCLE = false;
        TURN_LIGHTS_ON_AT_MINUTE = 0;
        TURN_LIGHTS_OFF_AT_MINUTE = 12 * 60;
        writeEnvironmentalControlValues(DESIRED_TEMPERATURE, TEMPERATURE_RANGE, DESIRED_HUMIDITY,
                                        HUMIDITY_RANGE, USE_NATURAL_LIGHTING_CYCLE,
                                        TURN_LIGHTS_ON_AT_MINUTE, TURN_LIGHTS_OFF_AT_MINUTE);
    }
}