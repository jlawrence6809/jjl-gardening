#include "pin_helpers.h"

bool boardPinIsOutputAllowed(int pin) {
    for (int valid : VALID_GPIO_PINS) {
        if (valid == pin) {
            return true;
        }
    }
    return false;
}

bool boardPinIsInputAllowed(int pin) {
    for (int valid : VALID_GPIO_PINS) {
        if (valid == pin) {
            return true;
        }
    }
    return false;
}
