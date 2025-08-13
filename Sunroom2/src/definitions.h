#include <Arduino.h>
#include <map>

#pragma once

// Board selection, choose one
// #define ESP32_S3
#define ESP32_NODE_MCU

// #define SUNROOM
#define BARN

#ifdef ESP32_S3
constexpr int DS18B20_PIN = 38; // Digital pin connected to the DHT sensor
constexpr int PHOTO_SENSOR_PIN = -1;
constexpr int LIGHT_SWITCH_PIN = -1;
#else
constexpr int DS18B20_PIN = 23; // Digital pin connected to the DHT sensor
constexpr int PHOTO_SENSOR_PIN = 36;
constexpr int LIGHT_SWITCH_PIN = 39;
#endif

// DEFINES
constexpr long BAUD = 115200;

// constexpr int LED_PIN = 18;
// constexpr int FAN_PIN = 16;
// constexpr int HEAT_MAT_PIN = 17;

/**
 * SENSORS
 */

#ifdef ESP32_S3
constexpr int RELAY_COUNT = 8;
constexpr int RELAY_PINS[RELAY_COUNT] = {4, 5, 6, 7, 16, 17, 18, 8};
// Inverted relays, such as the a/c relay (mosfet are not inverted)
constexpr bool RELAY_IS_INVERTED[RELAY_COUNT] = {true, true, true, true, true, true, true, true};
#endif

#ifdef SUNROOM
constexpr int RELAY_COUNT = 8;
constexpr int RELAY_PINS[RELAY_COUNT] = {15, 2, 4, 16, 17, 5, 18, 19};
// Inverted relays, such as the a/c relay (mosfet are not inverted)
constexpr bool RELAY_IS_INVERTED[RELAY_COUNT] = {true, true, true, true, true, true, true, true};
#endif

#ifdef BARN
constexpr int RELAY_COUNT = 13;
constexpr int RELAY_PINS[RELAY_COUNT] = {15, 2, 4, 16, 17, 5, 18, 19, 32, 33, 25, 26, 27};
// Inverted relays, such as the a/c relay (mosfet are not inverted)
constexpr bool RELAY_IS_INVERTED[RELAY_COUNT] = {true, true, true, true, true, true, true, true, false, false, false, false, false};
#endif

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

extern String RELAY_LABELS[RELAY_COUNT];

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
extern int LAST_RESET_REASON;
extern uint32_t FREE_HEAP;
