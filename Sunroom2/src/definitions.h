#include <Arduino.h>
#include <map>

#pragma once

// DEFINES
constexpr long BAUD = 115200;
constexpr int DS18B20_PIN = 23; // Digital pin connected to the DHT sensor

// constexpr int LED_PIN = 18;
// constexpr int FAN_PIN = 16;
// constexpr int HEAT_MAT_PIN = 17;

/**
 * SENSORS
 */
constexpr int PHOTO_SENSOR_PIN = 36;
constexpr int LIGHT_SWITCH_PIN = 39;

// Pins to control 8 relay module
constexpr int RELAY_COUNT = 8;
constexpr int RELAY_PINS[RELAY_COUNT] = {15, 2, 4, 16, 17, 5, 18, 19};

// Pin values
enum RelayValue
{
    FORCE_OFF_AUTO_OFF = 00,
    FORCE_OFF_AUTO_ON = 10,
    FORCE_OFF_AUTO_X = 20,
    FORCE_ON_AUTO_OFF = 01,
    FORCE_ON_AUTO_ON = 11,
    FORCE_ON_AUTO_X = 21,
    FORCE_X_AUTO_OFF = 02,
    FORCE_X_AUTO_ON = 12,
    /**
     * Relay will be off.
     */
    FORCE_X_AUTO_X = 22,
};
extern RelayValue RELAY_VALUES[RELAY_COUNT];

// array of rule string pointers
extern String RELAY_RULES[RELAY_COUNT];

// VARIABLES
extern const char *WIFI_NAME;
extern const char *AP_PASSWORD;

extern uint64_t CHIP_ID;
extern uint64_t DEVICE_1;
extern uint64_t DEVICE_2;
extern uint64_t DEVICE_3;

extern String SSID;
extern String PASSWORD;

constexpr float NULL_TEMPERATURE = -100;

extern float CURRENT_TEMPERATURE;
extern float CURRENT_HUMIDITY;
extern float INTERNAL_CHIP_TEMPERATURE;
extern float CURRENT_PROBE_TEMPERATURE;

extern bool IS_HEAT_MAT_ON;
extern bool IS_FAN_ON;
extern float LED_LEVEL;

extern int LIGHT_LEVEL;
extern int IS_SWITCH_ON;

extern float DESIRED_TEMPERATURE;
extern float TEMPERATURE_RANGE;
extern float DESIRED_HUMIDITY;
extern float HUMIDITY_RANGE;

extern bool USE_NATURAL_LIGHTING_CYCLE;
extern int TURN_LIGHTS_ON_AT_MINUTE;
extern int TURN_LIGHTS_OFF_AT_MINUTE;

extern int RESET_COUNTER;