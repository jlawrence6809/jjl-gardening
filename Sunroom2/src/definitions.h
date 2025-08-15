#include <Arduino.h>
#include <map>
#include <array>

// Ensures this file is only included once throughout the project to prevent duplicate definitions
#pragma once

constexpr long BAUD = 115200;

// DEFINES FROM BOARD LEVEL CONFIG HEADERS
constexpr int DS18B20_PIN = CFG_DS18B20_PIN; // Digital pin connected to the DHT sensor
constexpr int PHOTO_SENSOR_PIN = CFG_PHOTO_SENSOR_PIN;
constexpr int LIGHT_SWITCH_PIN = CFG_LIGHT_SWITCH_PIN;
constexpr auto RELAY_PINS = std::array{ CFG_RELAY_PINS };
// constexpr auto STRAPPING_PINS = std::array{ CFG_STRAPPING_PINS };
constexpr auto RELAY_IS_INVERTED = std::array{ CFG_RELAY_IS_INVERTED };
inline constexpr const char *WIFI_NAME = CFG_WIFI_NAME;

// Valid and reserved pins for the currently selected board profile
// These may be provided by the env_*.h headers for each board. Fallback to empty lists.
#ifdef CFG_VALID_GPIO_PINS
constexpr auto VALID_GPIO_PINS = std::array{ CFG_VALID_GPIO_PINS };
#else
constexpr std::array<int, 0> VALID_GPIO_PINS = {};
#endif

#ifdef CFG_RESERVED_PINS
constexpr auto RESERVED_PINS = std::array{ CFG_RESERVED_PINS };
#else
constexpr std::array<int, 0> RESERVED_PINS = {};
#endif

// Dynamic relay configuration
// We allow a runtime-configurable set of relays (starting from 0), up to
// the compile-time maximum capacity defined by RELAY_PINS.size().
constexpr int MAX_RELAYS = static_cast<int>(RELAY_PINS.size());
extern int RUNTIME_RELAY_COUNT; // number of active relays at runtime
extern int RUNTIME_RELAY_PINS[MAX_RELAYS];
extern bool RUNTIME_RELAY_IS_INVERTED[MAX_RELAYS];

// Utility: return true if pin is disabled via config (< 0)
inline constexpr bool pinIsDisabled(int pin) { return pin < 0; }

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
extern RelayValue RELAY_VALUES[MAX_RELAYS];

// array of rule string pointers
extern String RELAY_RULES[MAX_RELAYS];

extern String RELAY_LABELS[MAX_RELAYS];

// VARIABLES
// Note: must be longer than 8 characters
inline constexpr const char *AP_PASSWORD = "esp32iscool!";

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

// --- Board-specific GPIO capability helpers ---
