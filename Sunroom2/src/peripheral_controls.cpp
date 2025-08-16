#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include "time_helpers.h"
#include <ArduinoJson.h>
#include "automation_dsl/types.h"

// Forward declaration of processRelayRules from automation_dsl/bridge.cpp
void processRelayRules();

static Timer timer(1010);

/**
 * The pin corresponding to the overhead lights in the sunroom.
 * Also the pin for the barn lights to keep the code simpler.
 *
 * It would be nice to make this programmable in the future.
 */

 // todo: make this configurable
#define SUNROOM_LIGHTS_RELAY 0

void turnOffRelay(int relay)
{
    int pin = RUNTIME_RELAY_PINS[relay];
    digitalWrite(pin, HIGH);
    RELAY_VALUES[relay] = FORCE_OFF_AUTO_X;
}

void turnOnRelay(int relay)
{
    int pin = RUNTIME_RELAY_PINS[relay];
    digitalWrite(pin, LOW);
    RELAY_VALUES[relay] = FORCE_ON_AUTO_X;
}

void setupRelays()
{
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++)
    {
        int pin = RUNTIME_RELAY_PINS[i];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }
}

bool isRelayOn(RelayValue value)
{
    return value == FORCE_ON_AUTO_X || value == FORCE_ON_AUTO_ON || value == FORCE_ON_AUTO_OFF || value == FORCE_X_AUTO_ON;
}

/**
 * Write the relay values based on the stored variables. The rules aren't processed here.
 */
void relayRefresh()
{
    for (int i = 0; i < RUNTIME_RELAY_COUNT; i++)
    {
        bool isInverted = RUNTIME_RELAY_IS_INVERTED[i];
        bool value = isRelayOn(RELAY_VALUES[i]);
        bool writeValue = isInverted ? !value : value;
        digitalWrite(RUNTIME_RELAY_PINS[i], writeValue);
    }
}

void photoSensorSetup()
{
    if (PHOTO_SENSOR_PIN < 0)
    {
        return;
    }
    // analog for PHOTO_SENSOR_PIN
    pinMode(PHOTO_SENSOR_PIN, INPUT);

    // analog setup:
    adcAttachPin(PHOTO_SENSOR_PIN);

    // analog read, not sure if this is necessary???
    int value = analogRead(PHOTO_SENSOR_PIN);
}

void lightSwitchSetup()
{
    if (LIGHT_SWITCH_PIN < 0)
    {
        return;
    }
    pinMode(LIGHT_SWITCH_PIN, INPUT);
    IS_SWITCH_ON = digitalRead(LIGHT_SWITCH_PIN);
}

void peripheralControlsSetup()
{
    setupRelays();
    photoSensorSetup();
    lightSwitchSetup();
}

void lightSwitchLoop()
{
    if (LIGHT_SWITCH_PIN < 0)
    {
        return;
    }
    int switchV = digitalRead(LIGHT_SWITCH_PIN);
    if (switchV != IS_SWITCH_ON)
    {
        bool sunroomV = isRelayOn(RELAY_VALUES[SUNROOM_LIGHTS_RELAY]);
        if (switchV == 1 && sunroomV == 0)
        {
            turnOnRelay(SUNROOM_LIGHTS_RELAY);
        }
        else if (switchV == 0 && sunroomV == 1)
        {
            turnOffRelay(SUNROOM_LIGHTS_RELAY);
        }
        IS_SWITCH_ON = switchV;
    }
}

/**
 * Controls the peripherals based on the current temperature and humidity
 */
void controlPeripheralsLoop()
{
    lightSwitchLoop();
    relayRefresh();
    if (!timer.isIntervalPassed())
    {
        return;
    }
    if (PHOTO_SENSOR_PIN >= 0)
    {
        LIGHT_LEVEL = analogRead(PHOTO_SENSOR_PIN);
    }

    // Process the rules
    processRelayRules();
}
