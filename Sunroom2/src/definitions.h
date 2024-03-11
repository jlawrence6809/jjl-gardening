#include <Arduino.h>
#include <map>
#include <ArduinoJson.h>

#pragma once

// DEFINES
constexpr long BAUD = 115200;
constexpr int DS18B20_PIN = 23; // Digital pin connected to the DHT sensor

// constexpr int LED_PIN = 18;
// constexpr int FAN_PIN = 16;
// constexpr int HEAT_MAT_PIN = 17;

/**
 * Analog
 */
constexpr int PHOTO_SENSOR_PIN = 36;
constexpr int LIGHT_SWITCH_PIN = 39;

// Pins to control 8 relay module
constexpr int RELAY_1_PIN = 15;
constexpr int RELAY_2_PIN = 2;
constexpr int RELAY_3_PIN = 4;
constexpr int RELAY_4_PIN = 16;
constexpr int RELAY_5_PIN = 17;
constexpr int RELAY_6_PIN = 5;
constexpr int RELAY_7_PIN = 18;
constexpr int RELAY_8_PIN = 19;

// Pin values
extern std::map<int, bool> RELAY_VALUES;

// extern DynamicJsonDocument relay_1_rules;
// extern DynamicJsonDocument relay_2_rules;
// extern DynamicJsonDocument relay_3_rules;
// extern DynamicJsonDocument relay_4_rules;
// extern DynamicJsonDocument relay_5_rules;
// extern DynamicJsonDocument relay_6_rules;
// extern DynamicJsonDocument relay_7_rules;
// extern DynamicJsonDocument relay_8_rules;

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

extern float DESIRED_TEMPERATURE;
extern float TEMPERATURE_RANGE;
extern float DESIRED_HUMIDITY;
extern float HUMIDITY_RANGE;

extern bool USE_NATURAL_LIGHTING_CYCLE;
extern int TURN_LIGHTS_ON_AT_MINUTE;
extern int TURN_LIGHTS_OFF_AT_MINUTE;

extern int RESET_COUNTER;