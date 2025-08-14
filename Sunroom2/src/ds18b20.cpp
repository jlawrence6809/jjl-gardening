#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "definitions.h"
#include "interval_timer.h"

static Timer timer(30020);

// NOTE: This code isn't tested yet. It had been initializing the probe in the global scope, which caused a crash if
// the pin was invalid. That was very hard to debug. I had chatgpt rewrite the code, which isn't used anyway, to
// initialize the probe in the setup.

// Lazily create OneWire and DallasTemperature only if the pin is valid.
static OneWire *g_oneWire = nullptr;
static DallasTemperature *g_sensors = nullptr;
static bool g_probeReady = false;

void temperatureProbeSetup(void)
{
    Serial.println("Setup temperature probe...");

    // Allow disabling via config
    if (DS18B20_PIN < 0)
    {
        Serial.println("DS18B20 disabled (pin < 0)");
        return;
    }

    // Validate GPIO capabilities for bidirectional OneWire
    if (!boardPinIsInputAllowed(DS18B20_PIN) || !boardPinIsOutputAllowed(DS18B20_PIN))
    {
        Serial.printf("DS18B20 pin %d not suitable for OneWire on this board profile\n", DS18B20_PIN);
        return;
    }

    // Create peripherals lazily (enable pull-up to keep bus idle-high)
    pinMode(DS18B20_PIN, INPUT_PULLUP);
    g_oneWire = new OneWire(DS18B20_PIN);
    g_sensors = new DallasTemperature(g_oneWire);
    g_sensors->begin();

    // Verify at least one device is present
    if (g_sensors->getDeviceCount() == 0)
    {
        Serial.println("No DS18B20 devices found on the bus; disabling probe");
        delete g_sensors; g_sensors = nullptr;
        delete g_oneWire; g_oneWire = nullptr;
        g_probeReady = false;
        return;
    }
    g_probeReady = true;

    Serial.printf("DS18B20 initialized on pin %d\n", DS18B20_PIN);
}

void temperatureProbeLoop(void)
{
    if (!g_probeReady)
    {
        return;
    }
    if (!timer.isIntervalPassed())
    {
        return;
    }

    Serial.println("Checking temperature probe...");
    g_sensors->requestTemperatures();
    float tempC = g_sensors->getTempCByIndex(0);
    CURRENT_PROBE_TEMPERATURE = tempC;
    Serial.println("Temperature: " + String(CURRENT_PROBE_TEMPERATURE));
}
