#include <Arduino.h>
#include "definitions.h"

// VARIABLES
const char *WIFI_NAME = "barn";

// Note: must be longer than 8 characters
const char *AP_PASSWORD = "esp32iscool!";

uint64_t CHIP_ID = 0;
uint64_t DEVICE_1 = 0xa85627a4ae30;
uint64_t DEVICE_2 = 0xc8216c12cfa4;
uint64_t DEVICE_3 = 0xe0286c12cfa4;

String SSID = "";
String PASSWORD = "";

int RESET_COUNTER = 0;

float CURRENT_TEMPERATURE = -100;
float CURRENT_HUMIDITY = -1;
float INTERNAL_CHIP_TEMPERATURE = -100;
float CURRENT_PROBE_TEMPERATURE = -100;

bool IS_HEAT_MAT_ON = false;
bool IS_FAN_ON = false;
float LED_LEVEL = 0;

// environmental control variables, defaults are found in preferences_helpers.cpp
float DESIRED_TEMPERATURE = -1;
float TEMPERATURE_RANGE = -1;
float DESIRED_HUMIDITY = -1;
float HUMIDITY_RANGE = -1;
bool USE_NATURAL_LIGHTING_CYCLE = false;
int TURN_LIGHTS_ON_AT_MINUTE = -1;
int TURN_LIGHTS_OFF_AT_MINUTE = -1;

int LIGHT_LEVEL = -1;
int IS_SWITCH_ON = 0;

uint32_t FREE_HEAP = 0;

RelayValue RELAY_VALUES[RELAY_COUNT] = {FORCE_OFF_AUTO_X};
String RELAY_RULES[RELAY_COUNT] = {};
String RELAY_LABELS[RELAY_COUNT] = {};
