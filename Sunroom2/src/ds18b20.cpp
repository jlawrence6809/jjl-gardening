#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "definitions.h"
#include "interval_timer.h"

static OneWire ds(DS18B20_PIN);
static Timer timer(30020);

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(DS18B20_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

int count = 0;

void temperatureProbeSetup(void)
{
    Serial.println("Setup temperature probe...");
    sensors.begin();
}

void temperatureProbeLoop(void)
{
    if (!timer.isIntervalPassed())
    {
        return;
    }
    Serial.println("Checking temperature probe...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    CURRENT_PROBE_TEMPERATURE = sensors.getTempCByIndex(0);
    Serial.println("Temperature: " + String(CURRENT_PROBE_TEMPERATURE));
}
