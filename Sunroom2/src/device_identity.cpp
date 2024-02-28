#include <Arduino.h>
#include <soc/efuse_reg.h>
#include "definitions.h"

// board "node32s"
// e fuse isn't written! (adc vref value)

void checkDeviceIdentityOnSetup()
{
    CHIP_ID = ESP.getEfuseMac();
    if (CHIP_ID == DEVICE_1)
    {
        Serial.println("DEVICE_1");
    }
    else if (CHIP_ID == DEVICE_2)
    {
        Serial.println("DEVICE_2");
    }
    else if (CHIP_ID == DEVICE_3)
    {
        Serial.println("DEVICE_3");
    }
    else
    {
        Serial.println("DEVICE NOT RECOGNIZED: " + CHIP_ID);
    }
}