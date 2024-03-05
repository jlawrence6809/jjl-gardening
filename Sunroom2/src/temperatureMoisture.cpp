#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include "definitions.h"
#include "interval_timer.h"
#include <Adafruit_AHTX0.h>

static Adafruit_AHTX0 aht;

static Timer timer(30000);

bool initializeSensor()
{
    Serial.println("Connecting to ATH21...");

    if (!aht.begin())
    {
        Serial.println("Could not find AHT! Check wiring.");
        return false;
    }
    return true;
}

void temperatureMoistureLoop()
{
    if (!timer.isIntervalPassed())
    {
        return;
    }

    Serial.println("Main loop core: " + String(xPortGetCoreID()));

    INTERNAL_CHIP_TEMPERATURE = temperatureRead();

    Serial.println("Checking temperature and humidity...");

    // check aht status
    if (!initializeSensor())
    {
        CURRENT_TEMPERATURE = NULL_TEMPERATURE;
        CURRENT_HUMIDITY = NULL_TEMPERATURE;
        return;
    }
    sensors_event_t humidity, temp;

    // check that we can read from the sensor
    if (!aht.getEvent(&humidity, &temp))
    {
        Serial.println("Sensor read failed. Reconnecting...");
        if (!initializeSensor())
        {
            CURRENT_TEMPERATURE = NULL_TEMPERATURE;
            CURRENT_HUMIDITY = NULL_TEMPERATURE;
            return;
        }
        aht.getEvent(&humidity, &temp);
    }
    CURRENT_TEMPERATURE = temp.temperature;
    CURRENT_HUMIDITY = humidity.relative_humidity;

    Serial.println("Temperature: " + String(CURRENT_TEMPERATURE, 2) + "C");
    Serial.println("Humidity: " + String(CURRENT_HUMIDITY, 2) + "%");
}