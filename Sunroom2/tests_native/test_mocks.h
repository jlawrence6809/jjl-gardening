#pragma once

// Mock sensor values for testing
extern float CURRENT_TEMPERATURE;
extern float CURRENT_HUMIDITY;
extern float LIGHT_LEVEL;
extern int IS_SWITCH_ON;

// Mock relay values for testing
enum RelayValue { OFF = 0, ON = 10, DONT_CARE = 20 };
extern RelayValue RELAY_VALUES[4];

// Mock setRelay function from bridge
void setRelay(int index, float value);
