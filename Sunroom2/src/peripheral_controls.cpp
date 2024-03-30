#include <Arduino.h>
#include <time.h>
#include "definitions.h"
#include "interval_timer.h"
#include "time_helpers.h"
#include <ArduinoJson.h>
#include "rule_helpers.h"

static Timer timer(30000);
int SUNROOM_LIGHTS_RELAY = 6;

void turnOffRelay(int relay)
{
    int pin = RELAY_PINS[relay];
    digitalWrite(pin, HIGH);
    RELAY_VALUES[relay] = FORCE_OFF_AUTO_X;
}

void turnOnRelay(int relay)
{
    int pin = RELAY_PINS[relay];
    digitalWrite(pin, LOW);
    RELAY_VALUES[relay] = FORCE_ON_AUTO_X;
}

void setupRelays()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        int pin = RELAY_PINS[i];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }
}

bool isRelayOn(RelayValue value)
{
    return value == FORCE_ON_AUTO_X || value == FORCE_ON_AUTO_ON || value == FORCE_ON_AUTO_OFF || value == FORCE_X_AUTO_ON;
}

void relayRefresh()
{
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        digitalWrite(RELAY_PINS[i], !isRelayOn(RELAY_VALUES[i]));
    }
}

void photoSensorSetup()
{
    // analog for PHOTO_SENSOR_PIN
    pinMode(PHOTO_SENSOR_PIN, INPUT);

    // analog setup:
    adcAttachPin(PHOTO_SENSOR_PIN);

    // analog read, not sure if this is necessary???
    int value = analogRead(PHOTO_SENSOR_PIN);
}

void lightSwitchSetup()
{
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

    Serial.println("Free heap:");
    FREE_HEAP = ESP.getFreeHeap();
    Serial.println(FREE_HEAP);
    LIGHT_LEVEL = analogRead(PHOTO_SENSOR_PIN);
    processRelayRules();
}
