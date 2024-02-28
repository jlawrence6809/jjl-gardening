#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include "definitions.h"
#include "interval_timer.h"
#include <Adafruit_AHTX0.h>

static Adafruit_AHTX0 aht;

static Timer timer(30000);

bool initializeSensor()
{
    // try to initialize!
    // int retries = 5;
    // Serial.println("Connecting to ATH21...");

    // while (!aht.begin() && retries > 0)
    // {
    //     Serial.println("Retrying...");
    //     retries--;
    //     delay(500);
    // }
    // if (retries == 0)
    // {
    //     Serial.println("ATH21 not found!");
    //     return false;
    // }

    // Serial.println("ATH21 found!");
    // return true;

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

    Serial.println("Checking temperature and humidity...");

    // check aht status
    if (!initializeSensor())
    {
        CURRENT_TEMPERATURE = -1;
        CURRENT_HUMIDITY = -1;
        return;
    }
    sensors_event_t humidity, temp;

    // check that we can read from the sensor
    if (!aht.getEvent(&humidity, &temp))
    {
        Serial.println("Sensor read failed. Reconnecting...");
        if (!initializeSensor())
        {
            CURRENT_TEMPERATURE = -1;
            CURRENT_HUMIDITY = -1;
            return;
        }
        aht.getEvent(&humidity, &temp);
    }
    CURRENT_TEMPERATURE = temp.temperature;
    CURRENT_HUMIDITY = humidity.relative_humidity;

    Serial.println("Temperature: " + String(CURRENT_TEMPERATURE, 2) + "C");
    Serial.println("Humidity: " + String(CURRENT_HUMIDITY, 2) + "%");
}