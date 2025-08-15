#include <Arduino.h>
#include "definitions.h"

uint64_t CHIP_ID = 0;
uint64_t DEVICE_1 = 0xa85627a4ae30;
uint64_t DEVICE_2 = 0xc8216c12cfa4;
uint64_t DEVICE_3 = 0xe0286c12cfa4;

String SSID = "";
String PASSWORD = "";

int RESET_COUNTER = 0;
int LAST_RESET_REASON = 0;

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

// Runtime relay configuration (defaults to 0 active relays)
int RUNTIME_RELAY_COUNT = 0;
int RUNTIME_RELAY_PINS[MAX_RELAYS] = {0};
bool RUNTIME_RELAY_IS_INVERTED[MAX_RELAYS] = {false};

RelayValue RELAY_VALUES[MAX_RELAYS] = {FORCE_OFF_AUTO_X};
String RELAY_RULES[MAX_RELAYS] = {};
String RELAY_LABELS[MAX_RELAYS] = {};

// --- Board-specific GPIO capability helpers ---
