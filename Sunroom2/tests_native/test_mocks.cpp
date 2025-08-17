#include "test_mocks.h"

// Mock sensor values for testing (same as used in the actual bridge)
float CURRENT_TEMPERATURE = 27.5f;
float CURRENT_HUMIDITY = 65.0f;
float LIGHT_LEVEL = 750.0f;
int IS_SWITCH_ON = 1;

// Mock relay values for testing
RelayValue RELAY_VALUES[4] = {DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE};

// Mock setRelay function (same as in actual bridge)
void setRelay(int index, float value) {
    int intVal = static_cast<int>(value);
    int onesDigit = RELAY_VALUES[index] % 10;
    int newValue = (intVal * 10) + onesDigit;
    RELAY_VALUES[index] = static_cast<RelayValue>(newValue);
}
