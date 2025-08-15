#pragma once

#include <Arduino.h>
#include "definitions.h"

inline float cToF(float c)
{
    if (c == NULL_TEMPERATURE)
    {
        return NULL_TEMPERATURE;
    }
    return c * 9 / 5 + 32;
}


