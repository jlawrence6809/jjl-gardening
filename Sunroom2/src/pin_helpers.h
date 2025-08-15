#pragma once

#include <Arduino.h>
#include "definitions.h"

// Return true if pin is a valid GPIO for output per current board env
bool boardPinIsOutputAllowed(int pin);

// Return true if pin is a valid GPIO for input per current board env
bool boardPinIsInputAllowed(int pin);
