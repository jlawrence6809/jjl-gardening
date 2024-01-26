#include <Arduino.h>

#pragma once

// DEFINES
constexpr long BAUD = 115200;
constexpr int DS18B20_PIN = 19; // Digital pin connected to the DHT sensor

// pins to control led (will need pwm), a fan, and a heat mat
constexpr int LED_PIN = 18;
constexpr int FAN_PIN = 16;
constexpr int HEAT_MAT_PIN = 17;

// VARIABLES
extern const char *APP_NAME;

extern uint64_t CHIP_ID;
extern uint64_t DEVICE_1;
extern uint64_t DEVICE_2;
extern uint64_t DEVICE_3;

extern String SSID;
extern String PASSWORD;

extern float CURRENT_TEMPERATURE;
extern float CURRENT_HUMIDITY;

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
