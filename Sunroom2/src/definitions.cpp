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

RelayValue RELAY_VALUES[RELAY_PINS.size()] = {FORCE_OFF_AUTO_X};
String RELAY_RULES[RELAY_PINS.size()] = {};
String RELAY_LABELS[RELAY_PINS.size()] = {};

// --- Board-specific GPIO capability helpers ---
static bool isClassicEsp32FlashPin(int pin) { return pin >= 6 && pin <= 11; }
static bool isClassicEsp32InputOnly(int pin) { return pin >= 34 && pin <= 39; }

const char *getBoardPinProfileName()
{
#if defined(PIN_PROFILE_ESP32_S3_DEVKIT)
    return "ESP32-S3-DEVKIT";
#elif defined(PIN_PROFILE_NODEMCU32S)
    return "NODEMCU-32S";
#else
#ifdef ESP32_S3
    return "ESP32-S3 (generic)";
#else
    return "ESP32 (classic)";
#endif
#endif
}

bool boardPinIsStrappingPin(int pin)
{
#if defined(PIN_PROFILE_ESP32_S3_DEVKIT)
    // S3 common strapping pins: 0, 3, 45, 46
    return pin == 0 || pin == 3 || pin == 45 || pin == 46;
#else
    // Classic ESP32: 0, 2, 4, 5, 12, 15
    return pin == 0 || pin == 2 || pin == 4 || pin == 5 || pin == 12 || pin == 15;
#endif
}

bool boardPinIsOutputAllowed(int pin)
{
#if defined(PIN_PROFILE_ESP32_S3_DEVKIT)
    // Disallow USB, JTAG, and any reserved per S3 docs: 19,20 (USB), 43,44 (UART), also keep 45/46 cautious
    if (pin == 19 || pin == 20 || pin == 43 || pin == 44) return false;
    // Many S3 pins are valid outputs; rely on IDF macro if present
    return GPIO_IS_VALID_OUTPUT_GPIO((gpio_num_t)pin);
#else
    if (isClassicEsp32FlashPin(pin) || isClassicEsp32InputOnly(pin)) return false;
    return GPIO_IS_VALID_OUTPUT_GPIO((gpio_num_t)pin);
#endif
}

bool boardPinIsInputAllowed(int pin)
{
#if defined(PIN_PROFILE_ESP32_S3_DEVKIT)
    // Disallow USB lines as inputs for app logic
    if (pin == 19 || pin == 20) return false;
    return GPIO_IS_VALID_GPIO((gpio_num_t)pin);
#else
    if (isClassicEsp32FlashPin(pin)) return false;
    return GPIO_IS_VALID_GPIO((gpio_num_t)pin);
#endif
}
